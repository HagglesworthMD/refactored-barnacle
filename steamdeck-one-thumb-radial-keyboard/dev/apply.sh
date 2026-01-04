#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_root}/build"

cmake -S "${repo_root}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release

cmake --build "${build_dir}" -j"$(nproc)"
cmake --install "${build_dir}" --prefix "${HOME}/.local"

systemd_user_available() {
  command -v systemctl >/dev/null 2>&1 || return 1
  systemctl --user show-environment >/dev/null 2>&1
}

restart_if_exists() {
  local unit="$1"
  if command -v rg >/dev/null 2>&1; then
    list_cmd=(rg -q "^${unit}")
  else
    list_cmd=(grep -q "^${unit}")
  fi
  if systemctl --user list-unit-files | "${list_cmd[@]}"; then
    systemctl --user restart "${unit}"
    return 0
  fi
  return 1
}

restarted=false
if systemd_user_available; then
  systemctl --user daemon-reload
  if restart_if_exists "radialkb-engine.service"; then
    restarted=true
  fi
  if restart_if_exists "radialkb-ui.service"; then
    restarted=true
  fi
else
  echo "Skipping systemd user actions (systemctl --user unavailable)."
fi

if systemd_user_available && [[ "${restarted}" == "false" ]]; then
  echo "No radialkb systemd units found. Available matching units:"
  if command -v rg >/dev/null 2>&1; then
    systemctl --user list-units --all | rg -i radialkb || true
  else
    systemctl --user list-units --all | grep -i radialkb || true
  fi
fi

sleep 1

echo "==> Verifying running binaries"
pgrep -af radialkb || true

echo "==> Verifying executable paths"
engine_pid="$(pgrep -n radialkb-engine || true)"
ui_pid="$(pgrep -n radialkb-ui || true)"

if [[ -n "${engine_pid}" ]]; then
  readlink -f "/proc/${engine_pid}/exe"
fi

if [[ -n "${ui_pid}" ]]; then
  readlink -f "/proc/${ui_pid}/exe"
fi

if systemd_user_available; then
  echo "==> Recent engine logs (FSM + input)"
  journalctl --user -u radialkb-engine.service --since "2 min ago" --no-pager | tail -100

  echo "==> Recent UI logs (QML / DBus)"
  journalctl --user -u radialkb-ui.service --since "2 min ago" --no-pager | tail -100
fi
