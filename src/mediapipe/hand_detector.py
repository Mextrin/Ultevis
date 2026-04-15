import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import socket
import json
import sys
import time

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
IS_WINDOWS = sys.platform.startswith("win")


def create_detector() -> vision.HandLandmarker:
    base_options = python.BaseOptions(model_asset_path="hand_landmarker.task")

    if IS_WINDOWS:
        options = vision.HandLandmarkerOptions(
            base_options=base_options,
            num_hands=2,
            running_mode=vision.RunningMode.VIDEO,
        )
    else:
        options = vision.HandLandmarkerOptions(
            base_options=base_options,
            num_hands=2,
        )

    return vision.HandLandmarker.create_from_options(options)


def create_socket() -> socket.socket:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if IS_WINDOWS:
        sock.connect((UDP_IP, UDP_PORT))
    return sock


def open_capture() -> cv2.VideoCapture:
    if not IS_WINDOWS:
        return cv2.VideoCapture(0)

    for backend in (cv2.CAP_DSHOW, None):
        cap = cv2.VideoCapture(0, backend) if backend is not None else cv2.VideoCapture(0)
        if cap.isOpened():
            break
        cap.release()
    else:
        raise RuntimeError("Could not open camera 0")

    capture_hints = [
        (cv2.CAP_PROP_BUFFERSIZE, 1),
        (cv2.CAP_PROP_FRAME_WIDTH, 640),
        (cv2.CAP_PROP_FRAME_HEIGHT, 480),
        (cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"MJPG")),
    ]

    for prop, value in capture_hints:
        try:
            cap.set(prop, value)
        except cv2.error:
            pass

    return cap


sock = create_socket()
detector = create_detector()
cap = open_capture()
frame_id = 0

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))

    if IS_WINDOWS:
        timestamp_ms = time.monotonic_ns() // 1_000_000
        detection_result = detector.detect_for_video(image, timestamp_ms)
    else:
        detection_result = detector.detect(image)

    payload = {
        "rightHandVisible": False,
        "leftHandVisible": False,
        "rightHandX": 0.0,
        "rightHandY": 0.0,
        "leftHandX": 0.0,
        "leftHandY": 0.0,
    }

    if IS_WINDOWS:
        payload["frameId"] = frame_id
        payload["sentAtMs"] = timestamp_ms

    if detection_result.handedness:
        for hand_landmarks, handedness in zip(
            detection_result.hand_landmarks, detection_result.handedness
        ):
            label = handedness[0].category_name
            wrist = hand_landmarks[0]

            if label == "Right":
                payload["rightHandVisible"] = True
                payload["rightHandX"] = wrist.x
                payload["rightHandY"] = wrist.y
            else:
                payload["leftHandVisible"] = True
                payload["leftHandX"] = wrist.x
                payload["leftHandY"] = wrist.y

    encoded_payload = json.dumps(payload, separators=(",", ":")).encode() if IS_WINDOWS else json.dumps(payload).encode()
    if IS_WINDOWS:
        sock.send(encoded_payload)
    else:
        sock.sendto(encoded_payload, (UDP_IP, UDP_PORT))

    display_frame = cv2.flip(frame, 1)

    cv2.putText(display_frame,
        f"R: {payload['rightHandVisible']}  L: {payload['leftHandVisible']}",
        (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2
    )
    cv2.imshow("MediaPipe Hand Tracker", display_frame)

    frame_id += 1

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
sock.close()
detector.close()
cv2.destroyAllWindows()
