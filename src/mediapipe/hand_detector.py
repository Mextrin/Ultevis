import os
import sys
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import numpy as np
import socket
import json
import tempfile
import threading
import time

from theremin import detect_hands, draw_circle, add_theremin_text, recognizer as theremin_recognizer
from drums import drum_detect, get_drum_hit_coordinates, recognizer as drum_recognizer
from keyboard import detect_key_strokes, draw_keyboard_zones, detect_keyboard_hands, recognizer as keyboard_recognizer

UDP_IP = "127.0.0.1"
UDP_PORT = 5005

CAMERA_MODE = "none"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
payload = {}

TEMP_DIR = tempfile.gettempdir()
temp_frame_path = os.path.join(TEMP_DIR, "airchestra_frame.tmp.jpg")
final_frame_path = os.path.join(TEMP_DIR, "airchestra_frame.jpg")

requested_mode = "none"

def safe_replace(src, dst, retries=10, delay=0.01):
    for _ in range(retries):
        try:
            os.replace(src, dst)
            return True
        except PermissionError:
            time.sleep(delay)
    return False

def open_camera_safely():
    if sys.platform == "win32":
        capture = cv2.VideoCapture(0, cv2.CAP_DSHOW)
        capture.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        capture.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    else:
        capture = cv2.VideoCapture(0)
        capture.set(cv2.CAP_PROP_FRAME_WIDTH, 10000)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, 10000)
        
    if not capture.isOpened():
        raise RuntimeError("Unable to open webcam at index 0.")
        
    return capture

def command_listener():
    global requested_mode
    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listen_sock.bind(("127.0.0.1", 5006))
    while True:
        try:
            data, _ = listen_sock.recvfrom(1024)
            cmd = json.loads(data.decode())
            # Added "none" and "keyboard" to the valid modes
            if "mode" in cmd and cmd["mode"] in ["theremin", "drums", "keyboard", "guitar", "none"]:
                requested_mode = cmd["mode"]
        except Exception as e:
            pass

listener_thread = threading.Thread(target=command_listener, daemon=True)
listener_thread.start()

INSTRUMENT = 1 if CAMERA_MODE == "drums" else 0
recognizer = drum_recognizer if INSTRUMENT == 1 else theremin_recognizer

cap = open_camera_safely() if CAMERA_MODE != "none" else None

last_timestamp_ms = -1

try:
    while True:
        if CAMERA_MODE != requested_mode:
            CAMERA_MODE = requested_mode
            
            if CAMERA_MODE == "none":
                # Physically turn off the webcam
                if cap is not None:
                    cap.release()
                    cap = None
                
                # Send one final packet to C++ to tell the audio engine to mute everything
                mute_payload = {
                    "instrument": "none",
                    "leftHandVisible": False, 
                    "rightHandVisible": False, 
                    "leftHandX": 0.0,
                    "leftHandY": 0.0,
                    "rightHandX": 0.0,
                    "rightHandY": 0.0,
                    "leftPinch": False,
                    "rightPinch": False,
                    "leftDrumHit": False, 
                    "rightDrumHit": False,
                    "mouthKickHit": False
                }
                sock.sendto(json.dumps(mute_payload).encode(), (UDP_IP, UDP_PORT))

                black_frame = np.zeros((480, 640, 3), dtype=np.uint8)
                cv2.imwrite(temp_frame_path, black_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
                safe_replace(temp_frame_path, final_frame_path)
                
            else:
                if cap is None:
                    cap = open_camera_safely()
                    
                INSTRUMENT = 1 if CAMERA_MODE == "drums" else 0
                if CAMERA_MODE == "keyboard":
                    recognizer = keyboard_recognizer
                else:
                    recognizer = drum_recognizer if INSTRUMENT == 1 else theremin_recognizer

        if CAMERA_MODE == "none" or cap is None:
            time.sleep(0.05)
            continue

        ret, frame = cap.read()
        if not ret or frame is None:
            continue

        current_timestamp_ms = int(time.time() * 1000)

        if current_timestamp_ms <= last_timestamp_ms:
            continue
        last_timestamp_ms = current_timestamp_ms

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame.copy())
        
        if CAMERA_MODE == "keyboard":
            recognition_result = recognizer.recognize_for_video(image, current_timestamp_ms)
        elif INSTRUMENT == 0:
            recognition_result = recognizer.detect_for_video(image, current_timestamp_ms)
        else:
            recognition_result = recognizer.recognize(image)

        active_zone_names: set[str] = set()
        displayed_hand_positions: dict[str, tuple[float, float]] = {}
        detected_labels = set()

        if CAMERA_MODE == "keyboard":
            # Get the main payload and drawing info from detect_key_strokes
            payload, active_zone_names, displayed_hand_positions = detect_key_strokes(recognition_result)
            
            # Get display-space hand positions plus gesture information for the QML overlay.
            gesture_payload = detect_keyboard_hands(recognition_result)
            
            # Add the hand/gesture info to the main payload, else it will get overwritten
            # in the next frame and cause missed QML hit-tests.
            for key in (
                "rightHandVisible",
                "leftHandVisible",
                "rightHandX",
                "rightHandY",
                "leftHandX",
                "leftHandY",
                "rightPinch",
                "leftPinch",
                "rightThumbUp",
                "rightThumbDown",
                "leftThumbUp",
                "leftThumbDown",
            ):
                payload[key] = gesture_payload.get(key, payload.get(key, False))

        elif INSTRUMENT == 0:
            payload = detect_hands(recognition_result)
        else:
            payload, active_zone_names, displayed_hand_positions = drum_detect(recognition_result, image)
            
        sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))
        display_frame = cv2.flip(frame, 1)       
        
        h, w = display_frame.shape[:2]
        frame_height, frame_width = h, w

        if CAMERA_MODE == "keyboard":
            # Draw the keyboard zones
            draw_keyboard_zones(display_frame, active_zone_names)
            # Draw circles for hand positions
            for label, position in displayed_hand_positions.items():
                center_x = int(position[0] * frame_width)
                center_y = int(position[1] * frame_height)
                cv2.circle(display_frame, center=(center_x, center_y), radius=10, color=(0, 180, 255), thickness=-1)
        elif INSTRUMENT == 0:
            draw_circle(display_frame, frame_height, frame_width, recognition_result)
        else:
            get_drum_hit_coordinates(display_frame, frame_height, frame_width, active_zone_names, displayed_hand_positions)

        resized_frame = cv2.resize(display_frame, (640, 480))
        
        cv2.imwrite(temp_frame_path, resized_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
        safe_replace(temp_frame_path, final_frame_path)

finally:
    if cap is not None:
        cap.release()
    sock.sendto(json.dumps({"cameraOff": True}).encode(), (UDP_IP, UDP_PORT))
    sock.close()
