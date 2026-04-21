#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_CONFIG="Debug"
EXE_PATH="${BUILD_DIR}/Airchestra_artefacts/${BUILD_CONFIG}/Airchestra"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HAND_DETECTOR_SCRIPT="${SCRIPT_DIR}/src/mediapipe/hand_detector.py"

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        if command -v cygpath >/dev/null 2>&1; then
            HAND_DETECTOR_SCRIPT="$(cygpath -w "${HAND_DETECTOR_SCRIPT}")"
        fi
        ;;
esac

# --- NEW HELPER FUNCTION ---
setup_and_activate_python() {
    if [ ! -d "${SCRIPT_DIR}/venv" ]; then
        echo "========================================================"
        echo "First-time setup: Creating Python Virtual Environment..."
        echo "========================================================"
        python3 -m venv "${SCRIPT_DIR}/venv"
        
        "${SCRIPT_DIR}/venv/bin/pip" install --upgrade pip
        "${SCRIPT_DIR}/venv/bin/pip" install mediapipe opencv-python
        echo "Python environment ready!"
    fi

    # Activate it for the current bash session
    source "${SCRIPT_DIR}/venv/bin/activate"
}

case "${1:-}" in
    build-all)
        git submodule update --init --recursive

        if [ ! -d "./vcpkg" ]; then
            git clone https://github.com/microsoft/vcpkg ./vcpkg
            ./vcpkg/bootstrap-vcpkg.sh
        fi

        cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_CONFIG}"
        cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"
        ;;

    compile)
        cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"
        ;;

    run)
        setup_and_activate_python

        echo "Launching Python camera feed in a new Terminal window..."
        # Tell macOS to pop open a new Terminal and run the Python script independently
        osascript -e "tell application \"Terminal\" to do script \"cd \\\"${SCRIPT_DIR}\\\" && source venv/bin/activate && python src/mediapipe/hand_detector.py\""

        # Tell the C++ app NOT to launch a second hidden instance
        export ULTEVIS_LAUNCH_HAND_DETECTOR=0
        export ULTEVIS_HAND_DETECTOR_SCRIPT="${HAND_DETECTOR_SCRIPT}"
        
        # Run C++ app in the current terminal
        "./${EXE_PATH}"
        ;;

    compile-and-run)
        cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"
        
        setup_and_activate_python

        echo "Launching Python camera feed in a new Terminal window..."
        # Tell macOS to pop open a new Terminal and run the Python script independently
        osascript -e "tell application \"Terminal\" to do script \"cd \\\"${SCRIPT_DIR}\\\" && source venv/bin/activate && python src/mediapipe/hand_detector.py\""

        # Tell the C++ app NOT to launch a second hidden instance
        export ULTEVIS_LAUNCH_HAND_DETECTOR=0
        export ULTEVIS_HAND_DETECTOR_SCRIPT="${HAND_DETECTOR_SCRIPT}"
        
        # Run C++ app in the current terminal
        "./${EXE_PATH}"
        ;;

    clean)
        rm -rf "${BUILD_DIR}"
        ;;

    *)
        echo "Usage: ./build.sh {build-all|compile|run|compile-and-run|clean}"
        exit 1
        ;;
esac