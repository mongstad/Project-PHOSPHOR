# PHOSPHOR-003 — fixed 16 pt GPU glyph-atlas experiment

Issue #5 performance feasibility only. A selectable CPU reference and GPU
composition path share the **accepted refined 16 pt** material and deterministic
fixture. No shell, PTY, parser, tabs, settings, new fonts, other themes or chrome.
See [report and owner gate](../../research/PHOSPHOR-003/REPORT.md).

## Build

C++20, CMake >=3.21, Qt >=6.6 Core/Gui/Quick, Noto Sans Mono. The tested setup
is Clang 20, Qt 6.11.2 and Wayland/OpenGL on NVIDIA. No private Qt APIs or custom
shader compiler are needed. The old CPU material source is compiled unchanged.

```sh
cmake -S prototypes/qt-gpu-atlas -B /tmp/phosphor-003-build \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build /tmp/phosphor-003-build -j4
ctest --test-dir /tmp/phosphor-003-build --output-on-failure
```

**GCC 16 optimized builds still fail the inherited material halo test.** Do not
use a failing build for visual comparisons; this experiment does not resolve
that pre-existing discrepancy. Run the tests before using another toolchain.
Local extracted-header build details are in the report's build record.

## Run matched paths

```sh
QT_QPA_PLATFORM=wayland QT_SCALE_FACTOR=1 XDG_CACHE_HOME=/tmp/phosphor-003-cache \
  /tmp/phosphor-003-build/phosphor-003 --path cpu --screen DP-1
# Same environment/arguments, replacing cpu with gpu:
QT_QPA_PLATFORM=wayland QT_SCALE_FACTOR=1 XDG_CACHE_HOME=/tmp/phosphor-003-cache \
  /tmp/phosphor-003-build/phosphor-003 --path gpu --screen DP-1
```

Both default to 1280×900 logical pixels and refined 16 pt. `--effects-off` uses
constant-color glyphs with identical masks/geometry. `--fullscreen` uses the
selected existing screen. `--width 3840 --height 2160` specifies a buffer/window,
not proof of a physical 4K display. No app option changes display modes.

`--capture /tmp/capture.png` saves the live unmodified client buffer at seven
seconds; `--metrics /tmp/metrics.json` writes timing/counter data on exit.
`--seconds 30 --update-after-ms 16000` starts synthetic dense changes at 16 ms
intervals after a warm idle period, then exits. `--fixed-size` locks the requested client size for measurement (the compositor
may still initially constrain oversized windows; always verify geometry).
It is incompatible with `--resize-test`. `--resize-test` goes to 800×600
and back to the initial size. `--help` lists all options. None executes commands.
Use a new output filename to preserve previous captures. Close the window or
let the duration expire; no daemon remains.

## Scope of GPU acceleration

Each atlas cell contains one reusable premultiplied glyph material, including
its accepted local glow. Font masks, distances and material evaluation remain
CPU work on cold cache/DPR changes. The GPU samples the resident atlas and blends
bounded batches of quads directly into the window. There is **no full-fixture texture** and
no whole-window CPU paint/upload in the GPU path. Warm updates replace geometry,
not the atlas. Glyph order is preserved for overlapping translucent halos.

The fixed fixture repertoire is finite and validated; this is not a dynamic
Unicode font-atlas manager. Fractional-scale behavior, backend compatibility,
cold cache cost and compiler limitations are reported candidly. Desktop output
and owner aesthetic acceptance are separate from byte comparisons.

Cleanup: close the window, then remove only disposable build/cache/output paths
you chose under `/tmp`. Preserve repository source, original artwork and all
research evidence. No install/uninstall, terminal-default change or system-wide
package installation is involved.

Validated visual parity is restricted to DPR 1. Fractional DPR 1.5 captures
differ materially from the CPU reference; see the report. Do not treat the
headless atlas texel test as proof of final scaled framebuffer parity.
