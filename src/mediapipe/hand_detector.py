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
            wrist = hand_landmarks[0]  # Wrist is first landmark

            X_RIGHT_MIN = 0.0 ### X right hand limit
            X_RIGHT_MAX = 0.67 ###

            if label == "Right":
                # Constrain the value between X_MIN and X_MAX
                constrained_x_right = max(X_RIGHT_MIN, min(wrist.x, X_RIGHT_MAX)) ###

                payload["rightHandVisible"] = True
                payload["rightHandX"] = constrained_x_right
                payload["rightHandY"] = wrist.y
            else:
                payload["leftHandVisible"] = True
                payload["leftHandX"] = wrist.x
                payload["leftHandY"] = wrist.y

    sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))

    display_frame = cv2.flip(frame, 1)
    
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