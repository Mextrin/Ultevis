#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_CONFIG="Debug"
EXE_PATH="${BUILD_DIR}/Airchestra_artefacts/${BUILD_CONFIG}/Airchestra"

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
        "./${EXE_PATH}"
        ;;

    compile-and-run)
        cmake --build "${BUILD_DIR}" --config "${BUILD_CONFIG}"
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
