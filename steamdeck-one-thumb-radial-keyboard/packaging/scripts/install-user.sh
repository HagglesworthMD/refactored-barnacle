#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
INSTALL_DIR="$HOME/.local"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"
cmake --build "${BUILD_DIR}"
cmake --install "${BUILD_DIR}"

mkdir -p "$HOME/.config/systemd/user"
cp "${ROOT_DIR}/packaging/systemd/radialkb-ui.service" "$HOME/.config/systemd/user/"
cp "${ROOT_DIR}/packaging/systemd/radialkb-engine.service" "$HOME/.config/systemd/user/"

systemctl --user daemon-reload
systemctl --user enable --now radialkb-engine.service
systemctl --user enable --now radialkb-ui.service

printf "Installed to %s and started systemd user services.\n" "$INSTALL_DIR"
