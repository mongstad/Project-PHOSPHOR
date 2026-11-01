# PHOSPHOR-002: isolated Qt molten-glyph experiment

Visual sample only: C++20, Qt Quick, native resizable window, CPU glyph materials.
No shell, PTY, parser, terminal engine, production settings or other themes.
The report and owner gate are in [PHOSPHOR-002](../../research/PHOSPHOR-002/REPORT.md).

## Build and run

Requires CMake >=3.21, a C++20 compiler, Qt 6 >=6.6 development packages for
Core/Gui/Quick, Qt Wayland at runtime, and Noto Sans Mono (or a substituted font,
which changes the result). The tested working compiler is Clang 20. The local
GCC 16 optimized build produced incorrect glow; **run CTest before trusting a
new compiler build**. This portability defect remains unresolved.

```sh
cmake -S prototypes/qt-molten-amber -B /tmp/phosphor-002-build \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build /tmp/phosphor-002-build -j4
QT_FORCE_STDERR_LOGGING=1 ctest --test-dir /tmp/phosphor-002-build --output-on-failure
XDG_CACHE_HOME=/tmp/phosphor-002-cache QT_QPA_PLATFORM=wayland \
  /tmp/phosphor-002-build/phosphor-002
```

Repeat with `--baseline` for constant-color text using identical geometry.
`--points 12` / `14` / `16`, `--glow 0..2` and `--edge 0..2` are disposable
experiment controls. Default font: Noto Sans Mono DemiBold, 14 pt. Effects-off
also uses DemiBold; the title has the same size as ordinary text.

`--update-ms 16` replaces synthetic dense rows on a 16 ms timer; it executes
nothing. `--seconds 24` quits automatically. `--capture /tmp/sample.png` saves
the unmodified live client framebuffer at seven seconds (after any blocked
initial rendering). `--resize-test` resizes to 800×600 then 1280×900.
`--screen DP-2 --fullscreen` uses a named existing display; it does not configure
monitors. `QT_SCALE_FACTOR=1.5` provides a process-local scaling test.
`--width`/`--height` control window size, not physical display resolution.
Use `--help` for the complete CLI.

Close the sample window to stop it. Remove only your chosen disposable build
and cache directories when no longer needed. Repository source/artwork/evidence
is retained. No installation, desktop registration, Konsole configuration,
default application change or persistent app settings is needed.

## Components

- `renderer.*`: independent CPU glyph mask, signed-distance bevel/reflection,
  emissive rim, exterior glow and DPR-specific cache.
- `main.cpp`: Qt Quick window, synthetic fixed-grid sample, measurement/capture
  hooks. This shell has no terminal semantics.
- `tests.cpp`: checks geometry parity, mask coverage conservation without glow,
  visible material difference, independent glow, transparent borders/spaces,
  and monospaced advance at 12/14/16 pt × DPR 1/1.5/2.

Cache rebuilding is synchronous and expensive; whole-surface image uploads are
not a production renderer. Unicode support is intentionally limited to BMP
code points in the fixture, without cluster shaping/width logic. See the report
for measured performance, visual gaps, compiler limits and the gate decision.

## Bounded 16 pt refinement

`--points 16 --refine-16` opts into a single stronger interior/bevel/rim candidate.
Omit `--refine-16` to reproduce the original material; its default appearance is
unchanged. The flag rejects other point sizes and `--baseline`. Glow and geometry
are shared. See the [matched native comparison and stop recommendation](../../research/PHOSPHOR-002/refinement-16pt/REPORT.md).
