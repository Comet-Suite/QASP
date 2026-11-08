# QASP beginner-help checks

This change responds to actual user testing of the first portable release.
Hover help must explain a term, its effect and an example; a parameter name or
technical description alone is not counted as beginner help.

## Content source

Edit `src/help/qasp-help.json`, then run:

```sh
python3 tools/generate_qasp_help.py
```

The generated `QaspHelpData.inc` is compiled into the application. There is no
runtime JSON dependency, network request or new resource folder to lose when
extracting the portable package.

- **135 context-specific parameter entries** (129 distinct parameter names).
- **23 focused lessons**, each limited to 190 words of body copy, with a native
  diagram and the sections meaning / aircraft effect / example / try / limit.
- Some advanced parameters still use their original descriptions, now explicitly
  labelled `Reference:`. This is not claimed as full beginner-help coverage.
- Keys include the parameter group, not just its name. Ambiguous `Design/Length`
  entries also require a matching description.

## Run tests

On Linux with g++, Python, FLTK, Xvfb and xauth:

```sh
bash tools/run_qasp_help_checks.sh
```

To use the exact FLTK 1.4.2 snapshot bundled by QASP:

```sh
FLTK_CONFIG=/path/to/fltk-build/fltk-config bash tools/run_qasp_help_checks.sh
```

The pure C++ tests run with address/undefined-behaviour sanitizers by default.
Set `QASP_HELP_SANITIZE=0` if your compiler cannot use them.

Tests cover context matching, unknown fields, units-sensitive examples, stale
help when a control is reused, explicit overrides, search and bounds. The native
component tests load all lessons and exercise navigation, empty results, clear,
minimum size, expansion, close and reopen.

These are focused tests. They do **not** replace a full QASP build or a Windows
runtime test. The Windows portable build remains the integration gate.

## Real component screenshots

Keep the test binary by setting `QASP_HELP_BUILD_DIR=/tmp/qasp-help-checks`.
Then, with Pillow and xdotool installed:

```sh
xvfb-run -a -s '-screen 0 1440x1000x24' \
  python3 tools/capture_qasp_field_guide.py /tmp/qasp-help-checks/field-guide \
  --lesson thrust --output field-guide-thrust.png
```

These are pixels from the actual compiled FLTK guide in a Linux component-test
harness. They must not be described as screenshots of the complete Windows app.
The lesson illustrations themselves are explanatory drawings, not analysis output.

## Copy rules

- Define the term before explaining a consequence.
- Avoid universal claims such as "more span gives more lift" or "dihedral makes
  an aircraft self-levelling"; state the conditions of a comparison.
- State units or the unit convention; do not silently choose metres for a
  unitless geometry model. Propeller mean chord `C/R` is a ratio, not a length.
- Distinguish an assumed input from a calculated or validated outcome.
- Never call an altitude input a calculated maximum flight altitude.
- Tutorials must use real menu labels and must not promise that every operation
  is undoable. Retain NASA/NOSA and third-party notices.

Packaging remains an extracted portable folder with its existing executables
and supporting DLLs. No single-file bundler or installer has been added.
