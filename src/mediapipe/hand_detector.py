import os
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import socket
import json
from theremin import detect_hands, draw_circle, add_theremin_text, recognizer as theremin_recognizer
from drums import drum_detect, get_drum_hit_coordinates, recognizer as drum_recognizer

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
CAMERA_MODE = os.environ.get("ULTEVIS_CAMERA_MODE", "theremin").strip().lower()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
payload = {}

# Determine the instrument index from the camera mode
if CAMERA_MODE == "drums":
    INSTRUMENT = 1
else:
    INSTRUMENT = 0

def draw_frame():
    frame_height, frame_width = display_frame.shape[:2]
    return frame_height, frame_width

recognizer = None

# Set recognizer and window title based on Instrument
if INSTRUMENT == 0:
    recognizer = theremin_recognizer
    window_title = "MediaPipe Theremin Tracker"
else:
    recognizer = drum_recognizer
    window_title = "MediaPipe Drum Zone Tracker"

# Video feed logic
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    raise RuntimeError("Unable to open webcam at index 0. Check camera permissions.")

# Request the highest resolution from the camera hardware (10000 is the resolution)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 10000)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 10000)

# Create a resizable window that maintains the aspect ratio
cv2.namedWindow(window_title, cv2.WINDOW_NORMAL | cv2.WINDOW_KEEPRATIO)

# Initialize frame_counter before the loop
frame_counter = 0

# Video feed loop
while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    # Increment the frame counter
    frame_counter += 1

    # Convert to MediaPipe format
    image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
    
    if INSTRUMENT == 0: 
        # Theremin uses HandLandmarker in VIDEO mode
        recognition_result = recognizer.detect_for_video(image, frame_counter)
    else: 
        # Drums use GestureRecognizer
        recognition_result = recognizer.recognize(image)

    active_zone_names: set[str] = set()
    displayed_hand_positions: dict[str, tuple[float, float]] = {}
    detected_labels = set()

    if INSTRUMENT == 0:
        payload = detect_hands(recognition_result)
    else:
        # Unpack the three return values from the updated function
        payload, active_zones, hand_positions = drum_detect(recognition_result)
        
    # Draw frame so that text or circles are drawn on the displayed frame
    sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))
    display_frame = cv2.flip(frame, 1)       
    frame_height, frame_width = draw_frame()

    if INSTRUMENT == 0:
        draw_circle(display_frame, frame_height, frame_width, recognition_result)
        add_theremin_text(display_frame, payload)
    else:
        # Pass the visual state to the drawing function
        get_drum_hit_coordinates(display_frame, frame_height, frame_width, active_zones, hand_positions)

    # Show the frame in the adaptive, resizable window
    cv2.imshow(window_title, display_frame)

    # Initialize default text and coordinates for both hands
    left_gesture_text = "None"
    right_gesture_text = "None"
    left_coords = "(X:0, Y:0)"
    right_coords = "(X:0, Y:0)"

    # Check if the model returned data about gestures, handedness, and landmarks
    if hasattr(recognition_result, 'gestures') and recognition_result.gestures and hasattr(recognition_result, 'hand_landmarks'):
        
        # Loop combining gesture data, hand side, and anatomical landmarks
        for gesture_info, hand_info, landmarks in zip(recognition_result.gestures, recognition_result.handedness, recognition_result.hand_landmarks):
            if gesture_info and hand_info and landmarks:
                gesture_name = gesture_info[0].category_name
                hand_label = hand_info[0].category_name
                
                # Get coordinates for landmark 9 (base of the middle finger)
                # X axis is inverted because display_frame was flipped horizontally earlier
                x_px = frame_width - int(landmarks[9].x * frame_width)
                y_px = int(landmarks[9].y * frame_height)
                coord_text = f"(X:{x_px}, Y:{y_px})"
                
                # Assign data to the corresponding hand
                if hand_label == "Left":
                    left_gesture_text = gesture_name
                    left_coords = coord_text
                elif hand_label == "Right":
                    right_gesture_text = gesture_name
                    right_coords = coord_text

    # Draw the text permanently on the screen
    cv2.putText(display_frame, f"Left Hand: {left_gesture_text} {left_coords}", (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 3, cv2.LINE_AA)
    cv2.putText(display_frame, f"Right Hand: {right_gesture_text} {right_coords}", (20, 90), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 3, cv2.LINE_AA)

    # Show the frame in the adaptive, resizable window
    cv2.imshow(window_title, display_frame)

    # Check for quit command
    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()