#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_CONFIG="Release"
APP_ARTEFACT_DIR="${BUILD_DIR}/Airchestra_artefacts/${BUILD_CONFIG}"
EXE_PATH="${APP_ARTEFACT_DIR}/Airchestra"
APP_BUNDLE_EXE="${APP_ARTEFACT_DIR}/Airchestra.app/Contents/MacOS/Airchestra"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

build_python() {
    echo "[build-python] Building Python executable..."
    cd "${SCRIPT_DIR}/src/mediapipe"
    
    if [ ! -d "venv" ]; then
        python3 -m venv venv
    fi
    source venv/bin/activate
    
    pip install --upgrade pip
    pip install opencv-python mediapipe numpy psutil pyinstaller
    
    # Note: Mac uses colons (:) for data separation, unlike Windows!
    pyinstaller -y -D --collect-all mediapipe --add-data "face_landmarker.task:." --add-data "gesture_recognizer.task:." --add-data "hand_landmarker.task:." hand_detector.py
    
    deactivate
    cd "${SCRIPT_DIR}"
}

app_executable() {
    if [ -x "${APP_BUNDLE_EXE}" ]; then
        printf '%s\n' "${APP_BUNDLE_EXE}"
    else
        printf '%s\n' "${EXE_PATH}"
    fi
}

app_runtime_dir() {
    dirname "$(app_executable)"
}

copy_runtime_dir() {
    local source_dir="$1"
    local destination_dir="$2"
    local label="$3"

    if [ ! -d "${source_dir}" ]; then
        echo "[copy-runtime] Missing source directory for ${label}: ${source_dir}"
        return 1
    fi

    mkdir -p "${destination_dir}"
    if command -v rsync >/dev/null 2>&1; then
        rsync -a --delete --exclude '.git' --exclude '__pycache__' "${source_dir}/" "${destination_dir}/"
    else
        rm -rf "${destination_dir}"
        mkdir -p "${destination_dir}"
        cp -R "${source_dir}/." "${destination_dir}/"
        find "${destination_dir}" \( -name .git -o -name __pycache__ \) -prune -exec rm -rf {} +
    fi
}

copy_runtime_files() {
    local runtime_dir
    runtime_dir="$(app_runtime_dir)"

    copy_runtime_dir "${SCRIPT_DIR}/src/mediapipe/dist/hand_detector" "${runtime_dir}/mediapipe" "MediaPipe Engine"

    if [ -d "${SCRIPT_DIR}/Instruments" ]; then
        copy_runtime_dir "${SCRIPT_DIR}/Instruments" "${runtime_dir}/Instruments" "Instruments"
    else
        echo "[copy-runtime] Instruments folder not found at ${SCRIPT_DIR}/Instruments; skipping instrument copy."
    fi
}

build_app() {
    build_python

    cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"
    copy_runtime_files
}

case "${1:-}" in
    build-all)
        git submodule update --init --recursive

        if [ ! -d "./vcpkg" ]; then
            git clone https://github.com/microsoft/vcpkg ./vcpkg
            ./vcpkg/bootstrap-vcpkg.sh
        fi

        cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_CONFIG}"
        build_app
        ;;

    compile)
        build_app
        ;;

    run)
        
        # Run C++ app in the current terminal
        "$(app_executable)"
        ;;

    compile-and-run)
        build_app
        
        
        # Run C++ app in the current terminal
        "$(app_executable)"
        ;;

    clean)
        rm -rf "${BUILD_DIR}"
        ;;

    clean-build-run)
        rm -rf "${BUILD_DIR}"
        cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_CONFIG}"
        build_app

        "$(app_executable)"
        ;;

    *)
        echo "Usage: ./build.sh {build-all|compile|run|compile-and-run|clean|clean-build-run}"
        exit 1
        ;;
esac
