#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_root}/build"

cmake -S "${repo_root}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release

cmake --build "${build_dir}" -j"$(nproc)"
cmake --install "${build_dir}" --prefix "${HOME}/.local"

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
if restart_if_exists "radialkb-engine.service"; then
  restarted=true
fi
if restart_if_exists "radialkb-ui.service"; then
  restarted=true
fi

if [[ "${restarted}" == "false" ]]; then
  echo "No radialkb systemd units found. Available matching units:"
  if command -v rg >/dev/null 2>&1; then
    systemctl --user list-units --all | rg -i radialkb || true
  else
    systemctl --user list-units --all | grep -i radialkb || true
  fi
fi
