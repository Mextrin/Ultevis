import os
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import socket
import json
import tempfile
import threading  # <-- NEW: For the background listener

from theremin import detect_hands, draw_circle, add_theremin_text, recognizer as theremin_recognizer
from drums import drum_detect, get_drum_hit_coordinates, recognizer as drum_recognizer

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
CAMERA_MODE = os.environ.get("ULTEVIS_CAMERA_MODE", "theremin").strip().lower()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
payload = {}

# Set up the cross-platform Temp Directory paths
TEMP_DIR = tempfile.gettempdir()
temp_frame_path = os.path.join(TEMP_DIR, "airchestra_frame.tmp.jpg")
final_frame_path = os.path.join(TEMP_DIR, "airchestra_frame.jpg")

# --- THE FIX: BACKGROUND COMMAND LISTENER ---
requested_mode = CAMERA_MODE

def command_listener():
    global requested_mode
    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listen_sock.bind(("127.0.0.1", 5006))
    while True:
        try:
            data, _ = listen_sock.recvfrom(1024)
            cmd = json.loads(data.decode())
            if "mode" in cmd and cmd["mode"] in ["theremin", "drums", "none"]:
                requested_mode = cmd["mode"]
        except Exception as e:
            pass

# Start the listener in the background (daemon=True means it dies when the app dies)
listener_thread = threading.Thread(target=command_listener, daemon=True)
listener_thread.start()
# --------------------------------------------

if CAMERA_MODE == "drums":
    INSTRUMENT = 1
else:
    INSTRUMENT = 0

def draw_frame():
    frame_height, frame_width = display_frame.shape[:2]
    return frame_height, frame_width

if INSTRUMENT == 0:
    recognizer = theremin_recognizer
else:
    recognizer = drum_recognizer

cap = cv2.VideoCapture(0)
if not cap.isOpened():
    raise RuntimeError("Unable to open webcam at index 0.")

cap.set(cv2.CAP_PROP_FRAME_WIDTH, 10000)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 10000)

frame_counter = 0

try:
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret or frame is None:
            continue

        # --- THE FIX: HOT-SWAP INSTRUMENTS ---
        # If C++ asked for a change, swap the AI models at the start of the next frame
        if CAMERA_MODE != requested_mode:
            CAMERA_MODE = requested_mode
            if CAMERA_MODE == "none":
                break # Quit Python if "none" is sent
                
            INSTRUMENT = 1 if CAMERA_MODE == "drums" else 0
            recognizer = drum_recognizer if INSTRUMENT == 1 else theremin_recognizer
        # -------------------------------------

        frame_counter += 1

        image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
        
        if INSTRUMENT == 0: 
            recognition_result = recognizer.detect_for_video(image, frame_counter)
        else: 
            recognition_result = recognizer.recognize(image)

        active_zone_names: set[str] = set()
        displayed_hand_positions: dict[str, tuple[float, float]] = {}
        detected_labels = set()

        if INSTRUMENT == 0:
            payload = detect_hands(recognition_result)
        else:
            payload, active_zones, hand_positions = drum_detect(recognition_result)
            
        sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))
        display_frame = cv2.flip(frame, 1)       
        frame_height, frame_width = draw_frame()

        if INSTRUMENT == 0:
            draw_circle(display_frame, frame_height, frame_width, recognition_result)
            add_theremin_text(display_frame, payload)
        else:
            get_drum_hit_coordinates(display_frame, frame_height, frame_width, active_zones, hand_positions)

        left_gesture_text = "None"
        right_gesture_text = "None"
        left_coords = "(X: 0.00, Y: 0.00)"
        right_coords = "(X: 0.00, Y: 0.00)"

        if hasattr(recognition_result, 'gestures') and recognition_result.gestures and hasattr(recognition_result, 'hand_landmarks'):
            for gesture_info, hand_info, landmarks in zip(recognition_result.gestures, recognition_result.handedness, recognition_result.hand_landmarks):
                if gesture_info and hand_info and landmarks:
                    gesture_name = gesture_info[0].category_name
                    hand_label = hand_info[0].category_name
                    norm_x = 1.0 - landmarks[9].x
                    norm_y = landmarks[9].y
                    coord_text = f"(X: {norm_x:.2f}, Y: {norm_y:.2f})"
                    
                    if hand_label == "Left":
                        left_gesture_text = gesture_name
                        left_coords = coord_text
                    elif hand_label == "Right":
                        right_gesture_text = gesture_name
                        right_coords = coord_text

        cv2.putText(display_frame, f"Left Hand: {left_gesture_text} {left_coords}", (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 3, cv2.LINE_AA)
        cv2.putText(display_frame, f"Right Hand: {right_gesture_text} {right_coords}", (20, 90), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 3, cv2.LINE_AA)

        resized_frame = cv2.resize(display_frame, (640, 480))
        cv2.imwrite(temp_frame_path, resized_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
        os.replace(temp_frame_path, final_frame_path)

finally:
    cap.release()
    sock.sendto(json.dumps({"cameraOff": True}).encode(), (UDP_IP, UDP_PORT))
    sock.close()