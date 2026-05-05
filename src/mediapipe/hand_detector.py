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
import subprocess

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
requested_mode = os.environ.get("ULTEVIS_CAMERA_MODE", "none")

CAMERA_INDEX_ENV = "ULTEVIS_CAMERA_INDEX"

# ==============================================================================
# Helper Functions
# ==============================================================================

def safe_replace(src, dst, retries=10, delay=0.01):
    """Safely overwrites the image file so QML doesn't read a half-written file."""
    for _ in range(retries):
        try:
            os.replace(src, dst)
            return True
        except PermissionError:
            time.sleep(delay)
    return False

def log(message):
    """Prints camera diagnostics immediately so QProcess forwarding shows them."""
    print(f"[hand_detector] {message}", flush=True)

def log_available_cameras():
    """Logs camera-like devices reported by the OS before OpenCV probing starts."""
    log(f"OpenCV {cv2.__version__} camera diagnostics starting.")

    if sys.platform != "win32":
        log("OS camera device listing is only implemented for Windows; OpenCV probing will follow.")
        return

    commands = [
        [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-Command",
            (
                "$devices = Get-PnpDevice | "
                "Where-Object { "
                "$_.Class -eq 'Camera' -or "
                "$_.Class -eq 'Image' -or "
                "$_.FriendlyName -match 'camera|webcam|usb video|integrated|ir' "
                "} | "
                "Sort-Object Class,FriendlyName | "
                "Select-Object Status,Class,FriendlyName,InstanceId; "
                "if ($devices) { $devices | Format-Table -AutoSize | Out-String -Width 240 }"
            ),
        ],
        [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-Command",
            (
                "$devices = Get-CimInstance Win32_PnPEntity | "
                "Where-Object { "
                "$_.PNPClass -eq 'Camera' -or "
                "$_.PNPClass -eq 'Image' -or "
                "$_.Name -match 'camera|webcam|usb video|integrated|ir' "
                "} | "
                "Sort-Object PNPClass,Name | "
                "Select-Object Status,PNPClass,Name,DeviceID; "
                "if ($devices) { $devices | Format-Table -AutoSize | Out-String -Width 240 }"
            ),
        ],
    ]

    errors = []
    for command in commands:
        try:
            result = subprocess.run(
                command,
                capture_output=True,
                text=True,
                timeout=5,
                check=False,
            )
        except Exception as exc:
            errors.append(str(exc))
            continue

        if result.returncode != 0:
            errors.append((result.stderr or result.stdout or "unknown error").strip())
            continue

        device_list = result.stdout.strip()
        if device_list:
            log("Windows camera-like devices:")
            for line in device_list.splitlines():
                if line.strip():
                    log(f"  {line}")
            return

    detail = "; ".join(error for error in errors if error)
    if detail:
        log(f"Unable to list Windows camera devices: {detail}")
    else:
        log("Windows camera-like devices: none found.")

def camera_candidates():
    """Returns camera index/backend pairs to try, preferring explicit user config."""
    if sys.platform == "win32":
        backends = [
            ("DirectShow", cv2.CAP_DSHOW),
            ("Media Foundation", cv2.CAP_MSMF),
            ("Any", cv2.CAP_ANY),
        ]
    else:
        backends = [("Any", cv2.CAP_ANY)]

    configured_index = os.environ.get(CAMERA_INDEX_ENV)
    if configured_index is not None:
        try:
            indexes = [int(configured_index)]
        except ValueError:
            log(f"Ignoring invalid {CAMERA_INDEX_ENV}={configured_index!r}; using auto-detection.")
            indexes = list(range(4))
    else:
        indexes = list(range(4))

    log("OpenCV camera probe candidates:")
    for index in indexes:
        for backend_name, _ in backends:
            log(f"  index {index} via {backend_name}")

    for index in indexes:
        for backend_name, backend in backends:
            yield index, backend_name, backend

def open_camera_safely():
    """Opens the camera with platform-specific fallbacks."""
    failures = []

    for index, backend_name, backend in camera_candidates():
        log(f"Trying camera index {index} via {backend_name}.")
        capture = cv2.VideoCapture(index, backend)
        capture.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        capture.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

        if not capture.isOpened():
            failures.append(f"index {index} / {backend_name}: open failed")
            capture.release()
            continue

        # Some Windows drivers report opened before a frame can actually be read.
        ok, frame = capture.read()
        if ok and frame is not None:
            width = int(capture.get(cv2.CAP_PROP_FRAME_WIDTH))
            height = int(capture.get(cv2.CAP_PROP_FRAME_HEIGHT))
            log(f"Opened camera index {index} via {backend_name} at {width}x{height}.")
            return capture

        failures.append(f"index {index} / {backend_name}: no frame")
        capture.release()

    details = "; ".join(failures) if failures else "no candidates tried"
    raise RuntimeError(
        "Unable to open a webcam. "
        f"Tried {details}. "
        f"Set {CAMERA_INDEX_ENV}=1 (or another index) before launch to force a camera."
    )

def command_listener():
    """Listens on a background thread for instrument change commands from C++."""
    global requested_mode
    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listen_sock.bind(("127.0.0.1", CMD_PORT))
    log(f"Listening for camera mode commands on UDP port {CMD_PORT}.")
    
    valid_modes = ["theremin", "drums", "keyboard", "guitar", "none"]
    while True:
        try:
            data, _ = listen_sock.recvfrom(1024)
            cmd = json.loads(data.decode())
            if "mode" in cmd and cmd["mode"] in valid_modes:
                requested_mode = cmd["mode"]
                log(f"Requested camera mode: {requested_mode}.")
        except Exception:
            pass

def send_mute_payload(sock):
    """Sends a completely blank payload to mute all instruments."""
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
    """Writes a black frame to disk when the camera is off."""
    black_frame = np.zeros((480, 640, 3), dtype=np.uint8)
    cv2.imwrite(TEMP_FRAME_PATH, black_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
    safe_replace(TEMP_FRAME_PATH, FINAL_FRAME_PATH)

# ==============================================================================
# Main Loop
# ==============================================================================

def main():
    global CAMERA_MODE
    
    log_available_cameras()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # Start command listener thread
    threading.Thread(target=command_listener, daemon=True).start()

    cap = None
    recognizer = None
    last_timestamp_ms = -1

    try:
        while True:
            # ---------------------------------------------------------
            # 1: Camera Mode & Hardware
            # ---------------------------------------------------------
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
                        
                    # Map the correct MediaPipe model to the instrument
                    if CAMERA_MODE in ["keyboard", "guitar"]:
                        recognizer = keyboard_recognizer
                    elif CAMERA_MODE == "drums":
                        recognizer = drum_recognizer
                    else:
                        recognizer = theremin_recognizer

            # Skip loop if camera is meant to be off
            if CAMERA_MODE == "none" or cap is None:
                time.sleep(0.05)
                continue

            # ---------------------------------------------------------
            # 2: Frame Capture 
            # ---------------------------------------------------------
            ret, frame = cap.read()
            if not ret or frame is None:
                continue

            current_timestamp_ms = int(time.time() * 1000)
            if current_timestamp_ms <= last_timestamp_ms:
                continue
            last_timestamp_ms = current_timestamp_ms

            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

            clean_frame = np.ascontiguousarray(rgb_frame.copy())
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=clean_frame)

            # ---------------------------------------------------------
            # 3: MediaPipe
            # ---------------------------------------------------------
            if CAMERA_MODE in ["keyboard", "guitar", "theremin"]:
                # High-speed video tracking mode
                if CAMERA_MODE == "theremin":
                    recognition_result = recognizer.detect_for_video(mp_image, current_timestamp_ms)
                else:
                    recognition_result = recognizer.recognize_for_video(mp_image, current_timestamp_ms)
            else:
                # Standard image mode (Drums)
                recognition_result = recognizer.recognize(mp_image)

            # ---------------------------------------------------------
            # 4: Payload
            # ---------------------------------------------------------
            payload = {}
            active_zone_names = set()
            displayed_hand_positions = {}

            if CAMERA_MODE == "keyboard":
                payload, active_zone_names, displayed_hand_positions = detect_key_strokes(recognition_result)
                gesture_payload = detect_keyboard_hands(recognition_result)
                payload.update(gesture_payload) # Merge gesture coords into main payload

            elif CAMERA_MODE == "guitar":
                payload, active_zone_names, displayed_hand_positions = detect_guitar_hands(recognition_result)

            elif CAMERA_MODE == "drums":
                payload, active_zone_names, displayed_hand_positions = drum_detect(recognition_result, mp_image)

            elif CAMERA_MODE == "theremin":
                payload = detect_hands(recognition_result)

            # Transmit data to C++ backend
            sock.sendto(json.dumps(payload).encode(), (UDP_IP, UDP_PORT))

            # ---------------------------------------------------------
            # 5: Visual Feedback & UI Drawing
            # ---------------------------------------------------------
            display_frame = cv2.flip(frame, 1) # Mirror for UI
            frame_height, frame_width = display_frame.shape[:2]

            if CAMERA_MODE == "keyboard":
                draw_keyboard_zones(display_frame, active_zone_names)
                for label, pos in displayed_hand_positions.items():
                    cx, cy = int(pos[0] * frame_width), int(pos[1] * frame_height)
                    cv2.circle(display_frame, (cx, cy), 10, (0, 180, 255), -1)

            elif CAMERA_MODE == "guitar":
                for label, pos in displayed_hand_positions.items():
                    cx, cy = int(pos[0] * frame_width), int(pos[1] * frame_height)
                    
                    # Guitar specific feedback colors
                    color = (0, 255, 0) # Green default
                    if label == "Right" and payload.get("guitarStrumHit", False):
                        color = (0, 0, 255) # Red on strum
                    elif label == "Left" and payload.get("leftPinch", False):
                        color = (0, 165, 255) # Orange on pinch
                        
                    cv2.circle(display_frame, (cx, cy), 15, color, -1)

            elif CAMERA_MODE == "drums":
                get_drum_hit_coordinates(display_frame, frame_height, frame_width, active_zone_names, displayed_hand_positions)

            elif CAMERA_MODE == "theremin":
                draw_circle(display_frame, frame_height, frame_width, recognition_result)

            # Write finalized frame to disk for QML to pick up
            resized_frame = cv2.resize(display_frame, (640, 480))
            cv2.imwrite(TEMP_FRAME_PATH, resized_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
            safe_replace(TEMP_FRAME_PATH, FINAL_FRAME_PATH)

            del mp_image
            del recognition_result

    finally:
        if cap is not None:
            cap.release()
        sock.sendto(json.dumps({"cameraOff": True}).encode(), (UDP_IP, UDP_PORT))
        sock.close()

if __name__ == "__main__":
    main()
