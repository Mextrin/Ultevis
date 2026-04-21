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
# This automatically handles the Python PEP 668 lock without manual intervention
setup_and_activate_python() {
    if [ ! -d "${SCRIPT_DIR}/venv" ]; then
        echo "========================================================"
        echo "First-time setup: Creating Python Virtual Environment..."
        echo "========================================================"
        python3 -m venv "${SCRIPT_DIR}/venv"
        
        # Upgrade pip and install the required machine learning libraries
        "${SCRIPT_DIR}/venv/bin/pip" install --upgrade pip
        "${SCRIPT_DIR}/venv/bin/pip" install mediapipe opencv-python
        echo "Python environment ready!"
    fi

    # Activate it so the C++ app inherits the correct Python path
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
        # Automatically handle the Python environment before launching!
        setup_and_activate_python

        export ULTEVIS_LAUNCH_HAND_DETECTOR=1
        export ULTEVIS_HAND_DETECTOR_SCRIPT="${HAND_DETECTOR_SCRIPT}"
        "./${EXE_PATH}"
        ;;

    compile-and-run)
        cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"
        
        # Automatically handle the Python environment before launching!
        setup_and_activate_python

        export ULTEVIS_LAUNCH_HAND_DETECTOR=1
        export ULTEVIS_HAND_DETECTOR_SCRIPT="${HAND_DETECTOR_SCRIPT}"
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