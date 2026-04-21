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
        export ULTEVIS_LAUNCH_HAND_DETECTOR=1
        export ULTEVIS_HAND_DETECTOR_SCRIPT="${HAND_DETECTOR_SCRIPT}"
        "./${EXE_PATH}"
        ;;

    compile-and-run)
        cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"
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
