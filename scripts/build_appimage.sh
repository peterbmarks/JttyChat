#!/usr/bin/env bash
# Builds JttyChat and packages it as a Linux AppImage.
#
# Usage: scripts/build_appimage.sh
#
# Requires a Qt6 development environment (qt6-base-dev, cmake, g++) to
# build the app. Downloads linuxdeploy + its Qt plugin (cached under
# tools/) to do the AppImage packaging itself.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
APPDIR="${BUILD_DIR}/AppDir"
TOOLS_DIR="${PROJECT_ROOT}/tools"
DIST_DIR="${PROJECT_ROOT}/dist"
ARCH="$(uname -m)"

log() { printf '==> %s\n' "$1"; }

fetch() {
    local url="$1" out="$2"
    if command -v wget >/dev/null 2>&1; then
        wget -q --show-progress -O "${out}" "${url}"
    else
        curl -fL --progress-bar -o "${out}" "${url}"
    fi
}

log "Configuring (CMake, Release)"
cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

log "Building"
cmake --build "${BUILD_DIR}" --config Release -j"$(nproc)"

log "Installing into AppDir"
rm -rf "${APPDIR}"
cmake --install "${BUILD_DIR}" --prefix "${APPDIR}/usr"

mkdir -p "${TOOLS_DIR}"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage"
LINUXDEPLOY_QT_PLUGIN="${TOOLS_DIR}/linuxdeploy-plugin-qt-${ARCH}.AppImage"

if [ ! -x "${LINUXDEPLOY}" ]; then
    log "Downloading linuxdeploy"
    fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage" "${LINUXDEPLOY}"
    chmod +x "${LINUXDEPLOY}"
fi

if [ ! -x "${LINUXDEPLOY_QT_PLUGIN}" ]; then
    log "Downloading linuxdeploy-plugin-qt"
    fetch "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage" "${LINUXDEPLOY_QT_PLUGIN}"
    chmod +x "${LINUXDEPLOY_QT_PLUGIN}"
fi

# linuxdeploy needs FUSE to run its AppImage directly; fall back to
# extract-and-run if FUSE isn't available (common in containers/CI).
run_tool() {
    local tool="$1"; shift
    if "${tool}" --appimage-help >/dev/null 2>&1; then
        "${tool}" "$@"
    else
        "${tool}" --appimage-extract-and-run "$@"
    fi
}

mkdir -p "${DIST_DIR}"

log "Running linuxdeploy with the Qt plugin"
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH:-}"
export QML_SOURCES_PATHS="${PROJECT_ROOT}/src"
export OUTPUT="${DIST_DIR}/JttyChat-${ARCH}.AppImage"
export PATH="${TOOLS_DIR}:${PATH}"

# Some systems alias `qmake` to Qt5 (e.g. via qtchooser) even though a Qt6
# dev environment is installed; the Qt plugin shells out to `qmake` to find
# the Qt install, so point it explicitly at the Qt6 one if present.
if command -v qmake6 >/dev/null 2>&1; then
    export QMAKE="$(command -v qmake6)"
fi

run_tool "${LINUXDEPLOY}" \
    --appdir "${APPDIR}" \
    --executable "${APPDIR}/usr/bin/JttyChat" \
    --desktop-file "${PROJECT_ROOT}/packaging/jttychat.desktop" \
    --icon-file "${PROJECT_ROOT}/packaging/jttychat.png" \
    --plugin qt \
    --output appimage

# linuxdeploy honours $OUTPUT and writes the AppImage there directly; older
# versions instead drop it in the current directory, so sweep that up too.
if ! [ -f "${OUTPUT}" ]; then
    mv -f "${PROJECT_ROOT}"/JttyChat-*.AppImage "${OUTPUT}"
fi

log "Done: ${OUTPUT}"
