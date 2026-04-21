import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import socket
import json
from pathlib import Path

UDP_IP = "127.0.0.1"
UDP_PORT = 5005

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Initialize GestureRecognizer
MODEL_PATH = Path(__file__).with_name("gesture_recognizer.task")
if not MODEL_PATH.exists():
    raise FileNotFoundError(f"MediaPipe gesture recognizer model not found: {MODEL_PATH}")

base_options = python.BaseOptions(model_asset_path=str(MODEL_PATH))
options = vision.GestureRecognizerOptions(base_options=base_options, num_hands=2)
recognizer = vision.GestureRecognizer.create_from_options(options)

cap = cv2.VideoCapture(0)
if not cap.isOpened():
    raise RuntimeError("Unable to open webcam at index 0. Check camera permissions.")

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    # Convert to MediaPipe format
    image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
    
    recognition_result = recognizer.recognize(image)

    payload = {
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
        "rightGesture": "None",
        "leftGesture": "None"
    }

    current_gesture = "None"

    if recognition_result.handedness:
        for i, (hand_landmarks, handedness, gestures) in enumerate(zip(
            recognition_result.hand_landmarks, 
            recognition_result.handedness,
            recognition_result.gestures
        )):
            label = handedness[0].category_name  # "Left" or "Right"
            thumb_tip = hand_landmarks[4]  # Thumb tip
            index_tip = hand_landmarks[8]  # Index finger tip
            
            # Get default gesture from the model
            gesture_name = gestures[0].category_name if gestures else "None"
            
            # Calculate distance for custom Pinch gesture
            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05
            
            # Override model gesture if pinch is detected
            if distance < PINCH_THRESHOLD:
                gesture_name = "Pinch"
                
            current_gesture = gesture_name 

            X_RIGHT_MIN = 0.0 
            X_RIGHT_MAX = 0.67 

            if label == "Right":
                constrained_x_right = max(X_RIGHT_MIN, min(index_tip.x, X_RIGHT_MAX))
                payload["rightHandVisible"] = True
                payload["rightHandX"] = constrained_x_right
                payload["rightHandY"] = index_tip.y
                payload["rightGesture"] = gesture_name
            else:
                payload["leftHandVisible"] = True
                payload["leftHandX"] = index_tip.x
                payload["leftHandY"] = index_tip.y
                payload["leftGesture"] = gesture_name

    sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))

    display_frame = cv2.flip(frame, 1)
    frame_height, frame_width = display_frame.shape[:2]
    
    # Draw circles on index finger tips
    if recognition_result.handedness:
        for hand_landmarks in recognition_result.hand_landmarks:
            index_tip = hand_landmarks[8]
            center_x = int((1 - index_tip.x) * frame_width)  
            center_y = int(index_tip.y * frame_height)
            cv2.circle(display_frame, center=(center_x, center_y), radius=10, color=(0, 255, 0), thickness=-1)
    
    cv2.putText(display_frame,
        f"L: {payload['leftHandVisible']}, LeftX: {payload['leftHandX']:.2f}, LeftY: {payload['leftHandY']:.2f}, Gest: {payload['leftGesture']}",
        (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2
    )
    cv2.putText(display_frame,
        f"R: {payload['rightHandVisible']}, RightX: {payload['rightHandX']:.2f}, RightY: {payload['rightHandY']:.2f}, Gest: {payload['rightGesture']}",
        (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2
    )

    cv2.imshow("MediaPipe Gesture Tracker", display_frame)

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()