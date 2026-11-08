# QASP — risks, limitations and no-warranty notice

**Please read this before relying on a model, a calculation or an exported file.**

QASP is an unofficial modification of OpenVSP. It is a conceptual aircraft
geometry and engineering-analysis tool, **not a certificate that an aircraft is
safe, airworthy, buildable, stable, controllable or legal to operate**. It is not
endorsed, approved or supported by NASA or the OpenVSP team.

## No promise of crash-free operation or preserved work

QASP is provided **AS IS**. Its maintainers and contributors make no additional
promise that it will be free from defects, crashes, freezes, incorrect results,
security vulnerabilities, incompatible changes or data loss. There is no
promise of uninterrupted availability, a particular level of performance,
fitness for a particular purpose, support response or future maintenance.

A successful build, a passed test, a checksum, an attractive 3D model, a smooth
plot, or a solver reporting that it finished does **not** establish that the
software or the result is correct or safe for your use.

The existing **NASA Open Source Agreement (NOSA) 1.3**, including its warranty
and liability provisions, remains the governing licence; see [LICENSE](LICENSE),
especially section 4. This document explains practical risks. It does not
replace the licence, create a new warranty, or remove rights or responsibilities
that applicable law does not allow to be excluded.

## What QASP does not guarantee

| It does not guarantee… | Why this matters |
|---|---|
| **No crashes, hangs or graphics failures** | Large models, memory exhaustion, drivers, native libraries, thread interactions and operating-system changes can fail. A computer that launches one model may fail on another. |
| **No lost, incomplete or corrupted files** | A crash, power failure, full disk, permission error, removable drive, network share, sync conflict or security-software intervention can interrupt a write. Staged saves reduce some risks; they are not a universal power-loss or backup guarantee. |
| **Recovery of unsaved changes** | Project-tab snapshots are working copies, not a promised autosave, version history or crash-recovery system. Temporary storage may be deleted. Save explicit project files and keep separate backups. |
| **A self-contained project file** | Textures, airfoils, custom components, scripts and solver files may refer to other paths. Moving a `.vsp3` file alone can break those references. |
| **Identical results on every machine or version** | Floating-point arithmetic, solver options, thread scheduling, library versions, geometry changes and driver behaviour can change results or rendering. |
| **Correct dimensions, units or mass** | An apparently sensible number may use the wrong units. Missing parts, incorrect material assumptions, shell/volume double counting and omitted payload or fuel can invalidate the result. |
| **A correct aerodynamic or structural answer** | Approximations, invalid geometry, insufficient resolution, wrong loads, missing constraints, inappropriate flow models or unconverged calculations can yield plausible but wrong answers. |
| **A particular top speed, flight ceiling or climb rate** | These require suitable mass, aerodynamic and propulsion data across the relevant conditions. An altitude input is not a calculated ceiling. This revision does not add a validated automatic aircraft-performance calculator. |
| **Safe stall, spin, trim, stability or handling behaviour** | A shape or a coefficient curve is not a demonstrated safe flight envelope. Simplified methods may not capture separated flow, stall progression or dynamic behaviour adequately. |
| **Structural strength, fatigue life or flutter clearance** | Creating a mesh is not the same as solving and validating an appropriate structural or aeroelastic model. Materials, joints, loads, boundary conditions and test evidence matter. |
| **A watertight, manufacturable or interchangeable CAD export** | Mesh resolution, tolerances, intersections, normals, trimming, format limitations and unit conventions can alter the receiving model. Inspect and validate exports in their destination tools. |
| **Regulatory approval or suitability for safety-critical use** | QASP is not a certification authority, a flight-control system, an autopilot, an operational dispatch tool or a substitute for applicable approval processes. |
| **A secure sandbox for files, scripts or external tools** | Native code and custom scripts/tools can access files and may perform other actions with your account's permissions. A project is not automatically trustworthy because it opens successfully. |
| **Compatibility with every file, plugin, script or later OpenVSP release** | Modified interfaces and saved formats can behave differently. Keep originals and test conversions on copies. |

## Analysis-specific traps

- **Geometry is not a real aircraft.** A closed exterior can still omit engines,
  fasteners, payload, fuel systems, control systems, clearances and internal structure.
- **Choose a method for the intended conditions.** Simplified lifting-surface,
  empirical drag, panel, rotor and structural methods have different assumptions.
  Do not extrapolate a method into stall, severe separation, transonic flow or
  another regime simply because it returns a number.
- **Convergence is necessary, not sufficient.** Check numerical resolution,
  iteration histories, physical assumptions and sensitivity to inputs. Agreement
  between two runs with the same wrong assumption is not independent validation.
- **Reference values matter.** Coefficients from different reference areas,
  lengths, coordinate systems or sign conventions are not directly comparable.
- **Avoid missing or double-counted effects.** Combining drag estimates or mass
  contributions without understanding their definitions can count an effect twice
  or omit it entirely.
- **A mesh is preparation.** CFD Mesh is not, by itself, a validated flow solution;
  an FEA mesh is not, by itself, a demonstrated strength or safety margin.
- **Human-readable help is explanatory, not authoritative design data.** Examples
  and diagrams simplify a topic and do not recommend a safe aircraft configuration.

## New workspace and project-tab limitations

The workspace UI is a modified interface and must be treated as experimental
until tested on the models and platform you intend to use.

- Only **one engineering model is active in the underlying engine at a time**.
  Tabs are not a promise of simultaneous, independent background analyses.
- Switching projects preserves the model and the settings supported by `.vsp3`
  working snapshots. It is not a promise to preserve every transient application
  object, generated mesh, plotted result, console history, clipboard or undo history.
- Project switching must be refused while a solver or mesh operation is running.
  Do not force-close a process or modify its working files to bypass that guard.
- Save or export results you need before switching projects. Verify the project,
  included components, reference values and output path before a new analysis.
- Asterisk/unsaved indicators are a convenience, not a backup system or a promise
  that every possible script/API-side change has been detected instantly.
- Auto-fit changes the camera framing. It does not correct scale, geometry,
  overlapping components, hidden parts or an engineering error.
- Collapsing a sidebar changes visibility and layout only. It does not stop an
  analysis, save a model or disable a component in the calculation.

Known restrictions and the actual tested scope belong in release notes. A broad
no-warranty statement is **not** a substitute for reporting a known defect.

## Files, privacy and security

1. Open models, scripts and custom components only from sources you trust. Treat
   `.vspscript`, `.vsppart`, plugins, native executables and DLLs as code, not
   harmless pictures. Do not run an unfamiliar project as an administrator.
2. Keep the portable folder together. Do not mix executables or DLLs from
   different builds or download replacement DLLs from arbitrary websites.
3. A SHA-256 checksum checks whether downloaded bytes match the published file.
   It is **not** proof that those bytes are harmless, bug-free or independently audited.
4. Do not disable antivirus or bypass a specific malware detection merely because
   this is an open-source project. Investigate the warning and its source.
5. Working snapshots, exports, logs, screenshots and external solver files can
   contain proprietary geometry, file paths or other sensitive information. They
   may remain in local temporary storage after a crash. Review what you share.
6. Do not assume that all scripts, third-party tools, browser links or integrations
   are offline or have the same privacy behaviour. Review them separately.
7. Native dependencies and the operating system have their own defects and
   security updates. QASP does not promise to detect or repair them automatically.

## Before relying on an output

- Keep the original model and at least one separate, verified backup. Do not test
  a new build on the only copy of important work.
- Save deliberately and confirm the intended filename and project tab. Reopen a
  copy to check it, particularly after conversion or a major edit.
- Check units, scale, symmetry, included sets, mass, CG, atmosphere, reference
  quantities, propulsion assumptions, materials, constraints and output paths.
- Inspect geometry, solver warnings and convergence. Compare with suitable
  benchmarks, measurements and independent methods.
- Have appropriately qualified engineers review any design proposed for real
  manufacture or operation. Obtain the required testing and approvals. Do not
  construct, fly or place people at risk on the strength of QASP output alone.

## Reporting a problem

Stop relying on an affected result. Preserve a copy rather than repeatedly
saving over it. Report the QASP version/commit, platform, exact error and steps
that reproduce the issue. Share a minimal non-sensitive model if possible.
Report QASP-specific changes to this repository, **not** as an official OpenVSP
or NASA support request. Reporting a problem does not guarantee a fix or a timetable.
