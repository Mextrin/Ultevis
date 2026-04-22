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
        payload = drum_detect(recognition_result)
        
    # Draw frame so that text or circles are drawn on the displayed frame
    sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))
    display_frame = cv2.flip(frame, 1)       
    frame_height, frame_width = draw_frame()

    if INSTRUMENT == 0:
        draw_circle(display_frame, frame_height, frame_width, recognition_result)
        add_theremin_text(display_frame, payload)
    else:
        get_drum_hit_coordinates(display_frame, frame_height, frame_width)   

    # Show the frame in the adaptive, resizable window
    cv2.imshow(window_title, display_frame)

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()