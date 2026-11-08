# MODIFICATIONS — QASP

**QASP** is an **unofficial fork** of [OpenVSP](https://github.com/OpenVSP/openvsp),
which is developed by NASA and released under the
**NASA Open Source Agreement (NOSA) version 1.3**. See `LICENSE`.

QASP is **not** produced, endorsed, sponsored, or supported by NASA, nor by the
OpenVSP development team. Do not report QASP bugs to them.

Baseline: OpenVSP **3.56.0** (upstream commit `ca4958f2`).

---

## Summary of changes from upstream

### 1. Rebranding / product identity

- `src/vsp/main.h.in`: `VSPVERSION1/2/3` display strings changed to
  "QASP : Aircraft Design Studio". `VSPVERSION4` is **intentionally unchanged**:
  the public version API and CAD metadata retain their existing format.
- `src/CMakeLists.txt`: package name, description and homepage updated.
- `README.md`, `qasp-logo.png`: QASP identity, download instructions and limits.

### 2. Portable build and releases

- `.github/workflows/qasp-release.yml`: portable Windows x64 ZIP build, package
  verification, checksums, artifacts and release publication.
- Beginner-help content and focused native component checks now gate the Windows
  build. The checks use the exact bundled FLTK source snapshot.
- **No installer or single-file bundler.** Existing `vsp.exe`, solver tools,
  neighbouring resources and runtime DLLs remain together in the portable folder.

### 3. Native file dialogs

- `ScreenMgr.cpp`, `PreferencesScreen.cpp`: native OS file dialogs selected by
  default, with the original in-app chooser still available as a fallback.

### 4. Hover explanations

- `GuiDevice.h/.cpp`: generic tooltip support across the widgets backing a
  device. Automatic help now follows group/name/description changes when a
  control is rebound; an invalid binding clears stale automatic help.
- `QaspHelp.h/.cpp`, `QaspHelpData.inc`: **135 context-specific authored entries**,
  explaining meaning, aircraft effect, example and units. The same name in
  different groups is not assumed to mean the same thing.
- Advanced controls without authored beginner copy retain their existing
  descriptions labelled `Reference:`. This is not claimed as complete coverage.
- `QaspTheme.cpp`: readable hover-card font, timing, width and colours.

### 5. In-app Field Guide

- `QaspFieldGuide.h/.cpp`: Help-menu window, revised after user testing of the
  first release. Long plain-text pages replaced with **23 short lessons**,
  structured sections, search, previous/next navigation and resize handling.
- `QaspLessonDiagram.h/.cpp`: native, offline explanatory diagrams. These are
  illustrations, not simulation results or performance claims.
- `MainVSPScreen.h/.cpp`: Help menu entry; original reference documentation remains.
- `src/help/qasp-help.json`: editable source for authored tooltip and lesson text,
  embedded at build time. No runtime JSON parser or external service added.
- Prior overstatements about undo, mandatory analysis order and direct ceiling
  predictions have been removed from the beginner guide. Tutorial menu paths
  have been checked against the source.

### 6. Flat theme

- `QaspTheme.h/.cpp`, `ScreenMgr.cpp`: cool neutral palette, hairline borders and
  flat replacements for beveled box drawing. Geometry/solver code is not changed.

### 7. Validation and documentation

- `tools/generate_qasp_help.py`, `check_qasp_help_copy.py`: content validation and
  generated-source consistency, plus a check that authored parameter names exist.
- `tools/test_qasp_help.cpp`: context, formatting, units-sensitive examples,
  rebinding, explicit overrides, search and content bounds.
- `tools/qasp_field_guide_test.cpp`, `run_qasp_help_checks.sh`: actual FLTK
  component tests, independent of the full geometry-engine build.
- `tools/capture_qasp_field_guide.py`: real native component screenshots on X11;
  no generated or mock UI is represented as a running QASP application.
- `docs/QASP-HELP-REVISION.md`: scope, test limits and labelled component screenshots.
- `src/gui_and_draw/CMakeLists.txt`: new QASP modules registered.

---

## Retained from upstream, unmodified

All NASA/NOSA copyright headers, the `LICENSE` file, and all third-party
licenses and notices are retained in full. QASP claims no ownership over the
upstream work.

## Third-party components

OpenVSP bundles or links numerous third-party libraries (FLTK, libxml2, Eigen3,
Clipper2, AngelScript, cminpack, delabella, GLM, GLEW, gl2ps, Code-Eli, libIGES,
OpenABF, exprparse and others). Their licenses still apply. See `LICENSE`,
`Libraries/`, and individual source directories under `src/external/`.

This help revision adds no third-party icon font, image asset pack or service.

## Trademark note

"NASA", the NASA logo, "OpenVSP" and "Vehicle Sketch Pad" are used only to
describe origin and lineage, as required for attribution. No endorsement is
implied.
