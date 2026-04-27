import os
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import numpy as np
import socket
import json
import tempfile
import threading
import time # Needed to put the CPU to sleep

from theremin import detect_hands, draw_circle, add_theremin_text, recognizer as theremin_recognizer
from drums import drum_detect, get_drum_hit_coordinates, recognizer as drum_recognizer
from keyboard import detect_keyboard_hands

UDP_IP = "127.0.0.1"
UDP_PORT = 5005

# Force the app to start with the camera off, ignoring any old environment variables
CAMERA_MODE = "none"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
payload = {}

TEMP_DIR = tempfile.gettempdir()
temp_frame_path = os.path.join(TEMP_DIR, "airchestra_frame.tmp.jpg")
final_frame_path = os.path.join(TEMP_DIR, "airchestra_frame.jpg")

requested_mode = "none"

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

# Only open the camera if we aren't starting in 'none' mode
cap = cv2.VideoCapture(0) if CAMERA_MODE != "none" else None
if cap and not cap.isOpened():
    raise RuntimeError("Unable to open webcam at index 0.")
elif cap:
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 10000)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 10000)

frame_counter = 0

try:
    # --- THE FIX: Change to `while True` so the script stays alive even when the camera is off ---
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
                os.replace(temp_frame_path, final_frame_path)
                
            else:
                # Turn the webcam back on!
                if cap is None:
                    cap = cv2.VideoCapture(0)
                    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 10000)
                    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 10000)
                    
                INSTRUMENT = 1 if CAMERA_MODE == "drums" else 0
                recognizer = drum_recognizer if INSTRUMENT == 1 else theremin_recognizer

        # If we are in 'none' mode, put the CPU to sleep for a fraction of a second and skip the AI
        if CAMERA_MODE == "none" or cap is None:
            time.sleep(0.05)
            continue

        ret, frame = cap.read()
        if not ret or frame is None:
            continue

        current_timestamp_ms = int(time.time() * 1000)

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
        
        if INSTRUMENT == 0: 
            recognition_result = recognizer.detect_for_video(image, current_timestamp_ms)
        else: 
            recognition_result = recognizer.recognize(image)

        active_zone_names: set[str] = set()
        displayed_hand_positions: dict[str, tuple[float, float]] = {}
        detected_labels = set()

        if CAMERA_MODE == "keyboard":
            payload = detect_keyboard_hands(recognition_result)
        elif INSTRUMENT == 0:
            payload = detect_hands(recognition_result)
        else:
            payload, active_zones, hand_positions = drum_detect(recognition_result, image)
            
        sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))
        display_frame = cv2.flip(frame, 1)       
        
        h, w = display_frame.shape[:2]
        frame_height, frame_width = h, w

        if CAMERA_MODE == "keyboard":
            pass
        elif INSTRUMENT == 0:
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
    if cap is not None:
        cap.release()
    sock.sendto(json.dumps({"cameraOff": True}).encode(), (UDP_IP, UDP_PORT))
    sock.close()
