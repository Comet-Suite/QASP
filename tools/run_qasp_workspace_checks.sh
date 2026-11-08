#!/usr/bin/env bash
# NOSA 1.3; see LICENSE. Focused checks, not full application/Windows validation.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
CHECK_DIR=$(mktemp -d)
trap 'rm -rf "$CHECK_DIR"' EXIT
CXX=${CXX:-g++}
FLTK_CONFIG=${FLTK_CONFIG:-fltk-config}
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc/gui_and_draw \
  src/gui_and_draw/QaspProjectSession.cpp tools/test_qasp_projects.cpp \
  -o "$CHECK_DIR/projects"
ASAN_OPTIONS=detect_leaks=1 "$CHECK_DIR/projects"
read -r -a CFLAGS <<< "$("$FLTK_CONFIG" --cxxflags)"
read -r -a LDFLAGS <<< "$("$FLTK_CONFIG" --ldflags)"
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O1 "${CFLAGS[@]}" \
  -Isrc/gui_and_draw src/gui_and_draw/QaspWorkspace.cpp tools/qasp_workspace_test.cpp \
  "${LDFLAGS[@]}" -o "$CHECK_DIR/workspace"
xvfb-run -a -s '-screen 0 1600x1100x24' "$CHECK_DIR/workspace"
