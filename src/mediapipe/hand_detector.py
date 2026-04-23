import os
import cv2
import mediapipe as mp
import socket
import json

# Import instrument modules
from theremin import detect_hands as theremin_detect, recognizer as theremin_recognizer
from drums import drum_detect, get_drum_hit_coordinates as draw_drums, recognizer as drum_recognizer

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
CAMERA_MODE = os.environ.get("ULTEVIS_CAMERA_MODE", "theremin").strip().lower()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 1. Wrapper functions to standardize outputs for every instrument
def process_theremin(result):
    payload = theremin_detect(result)
    return payload, None

def process_drums(result):
    payload, active_zones, hand_positions = drum_detect(result)
    return payload, {"zones": active_zones, "positions": hand_positions}

# 2. Wrapper functions to standardize drawing for every instrument
def render_theremin(frame, h, w, extra_data):
    pass # No specific drawings for theremin

def render_drums(frame, h, w, extra_data):
    draw_drums(frame, h, w, extra_data["zones"], extra_data["positions"])

# 3. Instrument Registry (Add new instruments here)
INSTRUMENT_REGISTRY = {
    "theremin": {
        "recognizer": theremin_recognizer,
        "title": "MediaPipe Theremin Tracker",
        "is_video_mode": True, # True if using detect_for_video()
        "process": process_theremin,
        "render": render_theremin
    },
    "drums": {
        "recognizer": drum_recognizer,
        "title": "MediaPipe Drum Zone Tracker",
        "is_video_mode": False, # False if using recognize()
        "process": process_drums,
        "render": render_drums
    }
}

# Fallback to theremin if environment variable is unrecognized
current_instrument = INSTRUMENT_REGISTRY.get(CAMERA_MODE, INSTRUMENT_REGISTRY["theremin"])

recognizer = current_instrument["recognizer"]
window_title = current_instrument["title"]

# Video feed logic
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    raise RuntimeError("Unable to open webcam at index 0. Check camera permissions.")

cap.set(cv2.CAP_PROP_FRAME_WIDTH, 10000)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 10000)
cv2.namedWindow(window_title, cv2.WINDOW_NORMAL | cv2.WINDOW_KEEPRATIO)

frame_counter = 0

try:
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        frame_counter += 1
        image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
        
        # Abstracted model execution
        if current_instrument["is_video_mode"]: 
            recognition_result = recognizer.detect_for_video(image, frame_counter)
        else: 
            recognition_result = recognizer.recognize(image)

        # Abstracted payload processing
        payload, extra_data = current_instrument["process"](recognition_result)
            
        sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))
        
        display_frame = cv2.flip(frame, 1)       
        frame_height, frame_width = display_frame.shape[:2]

        # Abstracted visual rendering
        current_instrument["render"](display_frame, frame_height, frame_width, extra_data)

        # Universal coordinate and gesture display for ALL instruments
        left_gesture_text = "None"
        right_gesture_text = "None"
        left_coords = "(X: 0.00, Y: 0.00)"
        right_coords = "(X: 0.00, Y: 0.00)"

        if hasattr(recognition_result, 'hand_landmarks') and recognition_result.hand_landmarks and hasattr(recognition_result, 'handedness'):
            has_gestures = hasattr(recognition_result, 'gestures') and recognition_result.gestures

            for i in range(len(recognition_result.hand_landmarks)):
                landmarks = recognition_result.hand_landmarks[i]
                hand_info = recognition_result.handedness[i]
                hand_label = hand_info[0].category_name
                
                gesture_name = "None"
                if has_gestures and i < len(recognition_result.gestures):
                    gesture_name = recognition_result.gestures[i][0].category_name
                    
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

        cv2.imshow(window_title, display_frame)
        
        if cv2.waitKey(1) == ord('q'):
            break
finally:
    cap.release()
    cv2.destroyAllWindows()
    sock.sendto(json.dumps({"cameraOff": True}).encode(), (UDP_IP, UDP_PORT))
    sock.close()