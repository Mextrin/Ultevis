import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import socket
import json

UDP_IP = "127.0.0.1"
UDP_PORT = 5005

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Create hand detector with new API
base_options = python.BaseOptions(model_asset_path='hand_landmarker.task')
options = vision.HandLandmarkerOptions(base_options=base_options, num_hands=2)
detector = vision.HandLandmarker.create_from_options(options)

cap = cv2.VideoCapture(0)

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    # Convert to MediaPipe format
    image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
    detection_result = detector.detect(image)

    payload = {
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
    }

    if detection_result.handedness:
        for i, (hand_landmarks, handedness) in enumerate(zip(
            detection_result.hand_landmarks, detection_result.handedness
        )):
            label = handedness[0].category_name  # "Left" or "Right"
            thumb_tip = hand_landmarks[4]  # Thumb tip is landmark 4
            index_tip = hand_landmarks[8]  # Index finger tip is landmark 8

            # Calculate distance between thumb and index tips
            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05  # Adjust this value to tune sensitivity

            # Only record position if thumb and index are touching (pinching)
            if distance < PINCH_THRESHOLD:
                X_RIGHT_MIN = 0.0 ### X right hand limit
                X_RIGHT_MAX = 0.67 ###

                if label == "Right":
                    # Constrain the value between X_MIN and X_MAX
                    constrained_x_right = max(X_RIGHT_MIN, min(index_tip.x, X_RIGHT_MAX))

                    payload["rightHandVisible"] = True
                    payload["rightHandX"] = constrained_x_right
                    payload["rightHandY"] = index_tip.y
                else:
                    payload["leftHandVisible"] = True
                    payload["leftHandX"] = index_tip.x
                    payload["leftHandY"] = index_tip.y

    sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))

    display_frame = cv2.flip(frame, 1)
    frame_height, frame_width = display_frame.shape[:2]
    
    # Draw circles after display_frame is created
    if detection_result.handedness:
        for i, (hand_landmarks, handedness) in enumerate(zip(
            detection_result.hand_landmarks, detection_result.handedness
        )):
            label = handedness[0].category_name
            thumb_tip = hand_landmarks[4]
            index_tip = hand_landmarks[8]

            distance = ((thumb_tip.x - index_tip.x)**2 + (thumb_tip.y - index_tip.y)**2)**0.5
            PINCH_THRESHOLD = 0.05

            if distance < PINCH_THRESHOLD:
                center_x = int((1 - index_tip.x) * frame_width)  # Flip x coordinate
                center_y = int(index_tip.y * frame_height)
                cv2.circle(display_frame, center=(center_x, center_y), radius=20, color=(0, 255, 0), thickness=-1)
    
    cv2.putText(display_frame,
        f"L: {payload['leftHandVisible']}, LeftX: {payload['leftHandX']:.2f}, LeftY: {payload['leftHandY']:.2f}",
        (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2
    )
    cv2.putText(display_frame,
        f"R: {payload['rightHandVisible']}, RightX: {payload['rightHandX']:.2f}, RightY: {payload['rightHandY']:.2f}",
        (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2
    )
    
    cv2.imshow("MediaPipe Hand Tracker", display_frame)

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()