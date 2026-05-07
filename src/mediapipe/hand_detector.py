import os
import sys
import cv2
import mediapipe as mp
import numpy as np
import socket
import json
import tempfile
import threading
import time
import gc

# --- Instrument Modules ---
from theremin import detect_hands, draw_circle, recognizer as theremin_recognizer
from drums import drum_detect, get_drum_hit_coordinates, recognizer as drum_recognizer
from keyboard import detect_key_strokes, draw_keyboard_zones, detect_keyboard_hands, recognizer as keyboard_recognizer
from guitar import detect_guitar_hands

# --- Configuration & Network ---
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
CMD_PORT = 5006

TEMP_DIR = tempfile.gettempdir()
TEMP_FRAME_PATH = os.path.join(TEMP_DIR, "airchestra_frame.tmp.jpg")
FINAL_FRAME_PATH = os.path.join(TEMP_DIR, "airchestra_frame.jpg")

# --- Global State ---
CAMERA_MODE = "none"
requested_mode = "none"

# ==============================================================================
# Helper Functions
# ==============================================================================

def safe_replace(src, dst, retries=10, delay=0.01):
    for _ in range(retries):
        try:
            os.replace(src, dst)
            return True
        except PermissionError:
            time.sleep(delay)
    return False

def camera_candidates():
    """Returns camera index/backend pairs to try, respecting ENV variables."""
    configured_index = os.environ.get("ULTEVIS_CAMERA_INDEX")
    indexes = [int(configured_index)] if configured_index is not None else [0, 1, 2, 3]

    if sys.platform == "win32":
        backends = [("DirectShow", cv2.CAP_DSHOW), ("Media Foundation", cv2.CAP_MSMF), ("Any", cv2.CAP_ANY)]
    else:
        backends = [("Any", cv2.CAP_ANY)]

    for index in indexes:
        for backend_name, backend in backends:
            yield index, backend_name, backend

def open_camera_safely():
    """Opens the camera using driver fallbacks while forcing HD resolution."""
    for index, backend_name, backend in camera_candidates():
        print(f"[hand_detector] Trying camera {index} via {backend_name}...")
        cap = cv2.VideoCapture(index, backend)
        
        if sys.platform == "win32":
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
        else:
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)

        if not cap.isOpened():
            cap.release()
            continue

        ret, frame = cap.read()
        if ret and frame is not None:
            w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            print(f"[hand_detector] Success! Opened camera {index} via {backend_name} at {w}x{h}.")
            return cap
            
        cap.release()

    raise RuntimeError("Unable to open any webcam. Please check your hardware.")

def command_listener():
    global requested_mode
    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listen_sock.bind(("127.0.0.1", CMD_PORT))
    
    valid_modes = ["theremin", "drums", "keyboard", "guitar", "none"]
    while True:
        try:
            data, _ = listen_sock.recvfrom(1024)
            cmd = json.loads(data.decode())
            if "mode" in cmd and cmd["mode"] in valid_modes:
                requested_mode = cmd["mode"]
        except Exception:
            pass

def send_mute_payload(sock):
    mute_payload = {
        "instrument": "none",
        "leftHandVisible": False, "rightHandVisible": False, 
        "leftHandX": 0.0, "leftHandY": 0.0,
        "rightHandX": 0.0, "rightHandY": 0.0,
        "leftPinch": False, "rightPinch": False,
        "leftDrumHit": False, "rightDrumHit": False,
        "mouthKickHit": False, "guitarStrumHit": False
    }
    sock.sendto(json.dumps(mute_payload).encode(), (UDP_IP, UDP_PORT))

def write_black_frame():
    black_frame = np.zeros((360, 640, 3), dtype=np.uint8)
    cv2.imwrite(TEMP_FRAME_PATH, black_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
    safe_replace(TEMP_FRAME_PATH, FINAL_FRAME_PATH)

# ==============================================================================
# Main Loop
# ==============================================================================

def main():
    global CAMERA_MODE
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    threading.Thread(target=command_listener, daemon=True).start()

    cap = None
    recognizer = None
    last_timestamp_ms = -1

    try:
        while True:
            if CAMERA_MODE != requested_mode:
                CAMERA_MODE = requested_mode
                
                if CAMERA_MODE == "none":
                    if cap is not None:
                        cap.release()
                        cap = None
                    send_mute_payload(sock)
                    write_black_frame()
                else:
                    if cap is None:
                        cap = open_camera_safely()
                        
                    if CAMERA_MODE in ["keyboard", "guitar"]:
                        recognizer = keyboard_recognizer
                    elif CAMERA_MODE == "drums":
                        recognizer = drum_recognizer
                    else:
                        recognizer = theremin_recognizer

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
            clean_frame = np.ascontiguousarray(rgb_frame.copy())
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=clean_frame.copy())

            if CAMERA_MODE in ["keyboard", "guitar", "theremin", "drums"]:
                if CAMERA_MODE == "theremin":
                    recognition_result = recognizer.detect_for_video(mp_image, current_timestamp_ms)
                else:
                    recognition_result = recognizer.recognize_for_video(mp_image, current_timestamp_ms)
            else:
                recognition_result = recognizer.recognize(mp_image)

            payload = {}
            active_zone_names = set()
            displayed_hand_positions = {}

            if CAMERA_MODE == "keyboard":
                payload, active_zone_names, displayed_hand_positions = detect_key_strokes(recognition_result)
                gesture_payload = detect_keyboard_hands(recognition_result)
                payload.update(gesture_payload) 

            elif CAMERA_MODE == "guitar":
                payload, active_zone_names, displayed_hand_positions = detect_guitar_hands(recognition_result)

            elif CAMERA_MODE == "drums":
                payload, active_zone_names, displayed_hand_positions = drum_detect(recognition_result, mp_image, current_timestamp_ms)

            elif CAMERA_MODE == "theremin":
                payload = detect_hands(recognition_result)

            sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))

            # 5: Visual Feedback & UI Drawing
            display_frame = cv2.flip(frame, 1) 
            frame_height, frame_width = display_frame.shape[:2]

            if CAMERA_MODE == "keyboard":
                draw_keyboard_zones(display_frame, active_zone_names)
                for label, pos in displayed_hand_positions.items():
                    cx, cy = int(pos[0] * frame_width), int(pos[1] * frame_height)
                    cv2.circle(display_frame, (cx, cy), 10, (0, 180, 255), -1)

            elif CAMERA_MODE == "guitar":
                pass

            elif CAMERA_MODE == "drums":
                get_drum_hit_coordinates(display_frame, frame_height, frame_width, active_zone_names, displayed_hand_positions)

            elif CAMERA_MODE == "theremin":
                pass

            resized_frame = cv2.resize(display_frame, (640, 360))
            cv2.imwrite(TEMP_FRAME_PATH, resized_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
            safe_replace(TEMP_FRAME_PATH, FINAL_FRAME_PATH)

            del mp_image
            if 'recognition_result' in locals():
                del recognition_result

    finally:
        if cap is not None:
            cap.release()
        sock.sendto(json.dumps({"cameraOff": True}).encode(), (UDP_IP, UDP_PORT))
        sock.close()

if __name__ == "__main__":
    main()