#!/usr/bin/env bash
# NASA NOSA 1.3; see LICENSE. Focused tests for actual QASP help code.
# Requires g++, python3, FLTK, Xvfb and xauth. No geometry-engine build needed.
# FLTK_CONFIG may point at the exact bundled FLTK build's fltk-config script.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
if [[ -z "${QASP_HELP_BUILD_DIR:-}" ]]; then
  CHECK_DIR="$(mktemp -d)"
  trap 'rm -rf "$CHECK_DIR"' EXIT
else
  CHECK_DIR="$QASP_HELP_BUILD_DIR"
  mkdir -p "$CHECK_DIR"
fi
CXX="${CXX:-g++}"
FLTK_CONFIG="${FLTK_CONFIG:-fltk-config}"

python3 tools/generate_qasp_help.py --check
python3 tools/check_qasp_help_copy.py

SANITIZERS=()
if [[ "${QASP_HELP_SANITIZE:-1}" == "1" ]]; then
  SANITIZERS=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"$CXX" -std=c++11 -Wall -Wextra -Werror -O1 -g "${SANITIZERS[@]}" \
  -Isrc/gui_and_draw tools/test_qasp_help.cpp src/gui_and_draw/QaspHelp.cpp \
  -o "$CHECK_DIR/help-tests"
"$CHECK_DIR/help-tests"

read -r -a FLTK_CXXFLAGS <<< "$("$FLTK_CONFIG" --cxxflags)"
read -r -a FLTK_LDFLAGS <<< "$("$FLTK_CONFIG" --ldflags)"
echo "Testing actual guide component with FLTK $("$FLTK_CONFIG" --version)"
"$CXX" -std=c++11 -Wall -Wextra -Werror -O1 "${FLTK_CXXFLAGS[@]}" \
  -Isrc/gui_and_draw tools/qasp_field_guide_test.cpp \
  src/gui_and_draw/QaspHelp.cpp src/gui_and_draw/QaspLessonDiagram.cpp \
  src/gui_and_draw/QaspFieldGuide.cpp src/gui_and_draw/QaspTheme.cpp \
  "${FLTK_LDFLAGS[@]}" -o "$CHECK_DIR/field-guide"
xvfb-run -a -s '-screen 0 1440x1000x24' "$CHECK_DIR/field-guide" --self-test
