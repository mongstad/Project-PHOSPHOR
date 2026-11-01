# PHOSPHOR-003 build / measurement reproduction

Accepted base: `54b0260` on `main`, merged PR #4. New branch:
`issue-5-gpu-glyph-atlas`. No accepted prototype or research file is edited.
The new executable compiles the accepted `qt-molten-amber/renderer.cpp` verbatim;
its refined 16 pt parameters are fixed in both paths. The fixture is extracted
unchanged from the old app; explicit image comparison checks this extraction.

No package install/download is needed in this issue. Reuse the compiler, Qt
headers/runtime and temporary OpenGL/Vulkan header prefix from
[PHOSPHOR-002 BUILD](../PHOSPHOR-002/BUILD.md). That record identifies official
Fedora RPMs, signature verification and local extraction. New builds are under
`/tmp/phosphor-003`, not in the repository. The normal package-based build is in
[the prototype README](../../prototypes/qt-gpu-atlas/README.md).

From this checkout, reproduce the exact local configurations with:

```sh
bash research/PHOSPHOR-003/tools/build-local.sh clang
bash research/PHOSPHOR-003/tools/build-local.sh ubsan
bash research/PHOSPHOR-003/tools/build-local.sh gcc
```

Clang 20 release and Clang UBSan trap builds/tests pass. GCC 16.2.1 release builds
but fails the retained rectangular-halo regression check, just as the accepted
research reported. This issue does not silently repair, suppress, or reclassify
that discrepancy. GCC is unsuitable for this material until separately resolved.
UBSan trap mode is used because the installed Clang sanitizer runtime archive
is unavailable. Native GPU UBSan coverage is recorded separately, in addition
to the headless atlas tests. Neither checks proprietary driver internals.

An initial build error used `slots` as a field name, colliding with Qt's keyword
macro; it was renamed to `entries` before successful builds and captures. Logs
preserve the failed attempt. The local `-L` flag is unused at compile time but
used at link time; its Clang warning is harmless. No material formula was changed.

## Native measurements

`tools/measure.py` uses native Wayland windows, basic Qt Quick render loop,
DPR 1, the same Clang binary, fixed refined 16 pt and shared fixture. It refuses
to overwrite a previous dataset; select a fresh `--out` for a reproduction.
It runs two repetitions per resolution with CPU/GPU order alternating. Each
process has a cold glyph cache and an already reusable Qt shader/pipeline cache.
Thus “cold” here means cold **glyph/material** cache, not an erased driver cache.

- Physical 1080p: fullscreen on DP-2, 1920×1080 at approximately 144 Hz.
- 4K buffer: 3840×2160 client on DP-1, whose physical mode remains 2560×1440
  at 144 Hz. The native window extends beyond the physical desktop. It is not
  physical 4K presentation, refresh or readability validation.
- The owner says HDMI is unavailable and directs DP use. Neither active DP
  advertises 4K. No HDMI enabling, display-mode/scale change or user-default
  change is performed. Before/after KScreen output state is recorded.

A temporary KWin script targets only `org.phosphor.Spike003`, sets the oversized
window geometry where required, focuses it for Spectacle, records geometry and
unloads itself. No persistent window rules are written. At seven seconds the
app captures its live client framebuffer. Six one-second process CPU/RSS and
whole-GPU samples follow for idle; the app switches to 16 ms synthetic updates
around 16 seconds. After two seconds of warm update, eight more samples follow.
Each process exits around 30 seconds. Capture/geometry operations occur outside
the measured update interval. The 112 rows are authoritative for actual elapsed
intervals; CPU is percent of one logical core based on `/proc/PID/stat`.
GPU utilization/power/memory cover the whole device, including unrelated apps.

`tools/validate-native.py` runs effects-off pairs, 1280×900 refined pairs, resize,
process-local DPR 1.5, an instrumented UBSan native run, threaded-loop smoke,
and short scene-graph profiling runs. Its static pairs share geometry and
fixture; profiling runs deliberately update and are not pixel-comparison pairs.
No dataset should be overwritten; preserve existing outputs before a new run.

`--metrics` records cold cache wall duration, warm CPU paint or GPU node-update
wall duration, builds/calls, frameSwapped and update counts. All durations use
QElapsedTimer; only the external `/proc` samples are CPU time. GPU node update
runs at the scene-graph synchronization boundary while GUI state is blocked.
Qt's own profiling logs supplement it with renderer synchronization/render
stage timings; these are not per-glyph GPU timestamps or key-to-pixel latency.
Texture byte counts are application-requested/full-surface replacement sizes,
not a PCIe traffic measurement. Geometry bytes count submitted vertices, not
actual driver bus traffic. The GPU atlas remains one upload across warm updates
and ordinary resize; DPR changes recreate it. Whole-screen GPU drawing still
occurs; the improvement is removing CPU rasterization and image transfer.

The scene-graph node owns its texture/material/geometry on the render thread;
Qt destroys it when invalidated. Metrics are atomic across render/GUI threads.
The fixture state is read only during the synchronized node-update callback.
Default-backend RHI is required; the software backend is rejected for the GPU path.
Only the recorded OpenGL/Qt/NVIDIA setup is performance-validated; no Vulkan,
Metal, other GPU or production Unicode atlas claim follows.

Cleanup: close sample windows or let timed runs exit. Temporary caches are under
`/tmp/phosphor-003/cache`; builds/download roots are local and removable when no
longer needed. Keep original artwork and all repository evidence. No installed
application, background service, terminal replacement or system package needs
uninstalling. Qt portal warnings arise because this disposable app has no
installed desktop entry; capture/render success is checked independently.

## Corrections and excluded diagnostics

The first 4K GPU mesh exceeded a 16-bit vertex/index batch range (74,058
vertices). Native captures showed repeated blending and missing geometry.
The final path uses at most 4,096 glyphs / 24,576 vertices per QSGGeometryNode,
sharing the same atlas, preserving glyph order and allowing Qt to batch safely.
A compile-time bound prevents reintroducing an oversized batch. This is a
geometry correctness fix; no material parameter or visual design changed.
The corrected real 4K capture differs only by one-channel-level rounding in a
small subset of pixels; the original large-mesh capture is not accepted evidence.

One initial CPU 4K run resized to 2560×1356 before its update interval. Its
measurements are invalid as 4K results. All initial timings/captures are retained
under `evidence/diagnostic-native/`, excluded from final tables. The complete
paired dataset is rerun under `evidence/final-native/` after the geometry fix,
with fixed-size measurement constraints and explicit phase/exit size checks.
KWin can initially constrain oversized windows regardless of requested size;
the helper forces the 4K client and measured geometry—not the CLI request—is
what establishes a 4K buffer. That window spans the DP desktop, and Qt may
associate it with DP-2 despite requesting DP-1. Both outputs remain at DPR 1;
neither is a physical 4K display.
