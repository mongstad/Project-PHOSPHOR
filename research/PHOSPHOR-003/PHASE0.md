# Issue #5 architecture assessment

Baseline: accepted main `54b0260` (PR #4 merged). The owner explicitly prefers
`--points 16 --refine-16`, not the earlier original material. The exact reference
is `research/PHOSPHOR-002/refinement-16pt/evidence/refined-client.png`.
Previous reports/artwork/source remain unchanged. No repository AGENTS.md was
found. Issue #5 authorizes this experiment over the older Ghostty-only phases
in Development_Plan.md. No full terminal work is authorized.

The accepted path constructs glyph masks at 3× physical pixel resolution,
computes distance-based normals, reflections, bevel/rim and halo on CPU, and
caches the resulting premultiplied glyph images. QQuickPaintedItem paints every
visible glyph into a whole-window image on every update, then Qt uploads that
surface. Existing measurements show approximately 1 s cold material construction
and substantial dense 4K CPU painting cost; these are separate costs.

Bounded plan: preserve that exact material constructor and upload a compact
**per-glyph RGBA atlas**, then render one textured triangle list with supported
QSGGeometry/QSGTextureMaterial. Cache the atlas until DPR/material changes;
update only quads for text changes and resize. Preserve painter row/glyph order
for overlapping translucent glow. Snap quad positions to the same physical
pixel raster placement as the CPU reference, then verify actual captures.
This is GPU glyph composition, not GPU material generation or a new SDF shader.
It uses individual reusable glyphs, never an image of a line or complete fixture.
A shader rewrite of the metallic formula is unnecessary for this bottleneck and
would risk the owner's accepted appearance. Cold CPU cache cost remains explicit.

The new isolated executable offers matched CPU and GPU paths and effects-off,
with one shared deterministic fixture at fixed 16 pt. Instrument cold glyph
construction, CPU paint/layout, scene-graph node update and texture/geometry byte
counts separately. Bytes are application-side requested/logical amounts, not
measured PCIe traffic. FrameSwapped counts are not FPS or latency claims.

Use two paired repetitions per physical resolution, with order alternated;
measure warm idle and 16 ms dense updates for each path. Capture static fixtures
separately so content matches. Include cold startup, resize and effects-off.
Record process CPU/RSS and whole-device GPU telemetry; distinguish buffer size,
physical mode, refresh rate and DPR. Physical HDMI 4K requires explicit owner
approval; otherwise label oversized buffer results. Stop if this small atlas
path cannot preserve the design or demonstrate an advantage.

Public Qt APIs checked on 2026-11-01:
- https://doc.qt.io/qt-6/qsgtexturematerial.html — textured premultiplied-alpha
  geometry, default scene-graph backend; no software-backend assumption.
- https://doc.qt.io/qt-6/qsggeometry.html — textured vertices/triangle buffers.
- https://doc.qt.io/qt-6/qquickitem.html#updatePaintNode — render-thread node
  ownership and GUI synchronization boundary.

Live docs are 6.12; local compile/runtime are Qt 6.11.2. No private API,
new framework, shader-tool dependency or third-party code is required. Qt
Core/Gui/Quick licensing and proposed MIT source licensing remain as documented
in PHOSPHOR-002; no license is selected/changed. Clang release, GCC release and
UBSan trap tests will be attempted with the existing local toolchains. The old
optimized-GCC discrepancy must remain a failure unless new evidence resolves it.

Owner clarification: HDMI is not available; use DP. DP-2 supports 1920×1080 and
DP-1 supports 2560×1440, neither exposes 3840×2160. Therefore use physical 1080p
on DP-2 and an oversized 3840×2160 native client/buffer on DP-1. Do not enable
HDMI or change any desktop modes. Physical 4K acceptance remains unverified.

Implementation correction: native 4K validation exposed a large-mesh batching
limit. The final candidate uses multiple bounded triangle lists sharing one
atlas (4,096 glyphs per node), rather than the initially planned single list.
See BUILD.md for excluded diagnostics and corrected measurements.
