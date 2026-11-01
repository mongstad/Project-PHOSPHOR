# PHOSPHOR-002 — minimum glyph-aware technique

Issue #3 explicitly authorizes this alternative-renderer feasibility experiment;
`Development_Plan.md` remains unchanged historical guidance. No AGENTS.md exists
in the project or its parent workspace. Branch `issue-3-qt-molten-glyph` starts
at fetched accepted main `3adc5a451cd46ef92423304ff55a995a44ee44db`, the merged
PR #2. PHOSPHOR-001 records and all five original PNGs remain immutable.

## Why this pipeline

The previous Ghostty shader could not distinguish matching text/background
pixels in a composited frame. Here a C++ glyph cache rasterizes **only font
glyphs**, at three samples per physical pixel. A truncated signed distance from
each glyph's coverage boundary supplies bevel normals. A warm reflection band,
dark trough and lower copper reflection are modulated by directional lighting;
a thin emissive rim and independent exterior glow complete the material.
The result is downsampled once to the actual DPR, cached and drawn at fixed
monospaced cell positions. Baseline uses identical masks, placement and sampling,
with a constant foreground. No concept image is loaded by the executable.

The simplest Qt Quick adapter is a `QQuickPaintedItem` inside `QQuickWindow`.
It uploads a CPU-painted image to the scene graph; this is deliberately a CPU
mask experiment, not a GPU SDF implementation. The cache avoids recomputing
material during synthetic text updates. Whole-window paint/upload remains a
known cost. Idle has no animation timer. A future terminal core would supply
cells to an independent renderer; none is present here.

Official API references consulted on 2026-11-01:

- [Qt Quick scene graph](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html): custom QQuickItem geometry/material via updatePaintNode, QRhi-backed presentation.
- [QQuickPaintedItem](https://doc.qt.io/qt-6/qquickpainteditem.html): QPainter image-to-texture path and its upload cost.
- [QSGSimpleTextureNode](https://doc.qt.io/qt-6/qsgsimpletexturenode.html): texture-node backend limits and QSGImageNode alternative.

The live Qt documentation redirects to 6.12; the tested installed Qt is 6.11.2.
The actual compile and native runtime validate the subset used. Qt Quick's
internal text distance-field atlas is not accessed through private APIs.
A custom QSGMaterial/ShaderEffect atlas could reduce upload cost, but its
engineering is not presumed necessary or authorized beyond this gate.

## Explicit limits before testing

At 12 pt a normal glyph stroke has very few physical pixels: metallic depth
may collapse into alternating bright/dark pixels. Supersampling cannot create
extra displayed detail. Bloom can improve luminous appearance but cannot prove
metallic structure. The comparison must therefore include normal-size captures.

The sample iterates BMP QChars on a fixed grid. It covers selected Greek,
accented and box glyphs, not shaping, combining clusters, emoji, bidi, CJK width,
font fallback correctness or terminal Unicode conformance. Font/DPR changes
rebuild the cache synchronously. No cursor, selection, ANSI parser, PTY, shell,
settings system, other themes, or custom window decoration is implemented.

## Dependencies and proposed licensing

C++20, CMake >=3.21, Qt >=6.6 Core/Gui/Quick, a monospaced font; runtime uses
Qt Wayland and the host graphics drivers. Noto Sans Mono is the tested font.
No bundled fonts, Qt binaries or third-party code are committed. [Qt modules](https://doc.qt.io/qt-6/licensing.html)
have open-source LGPLv3/GPL options (and commercial licensing); distribution
would require a proper module/license and dynamic-linking compliance review.
Noto fonts use [SIL OFL](https://github.com/notofonts/latin-greek-cyrillic/blob/main/OFL.txt). CMake and the compiler are build tools.

Propose MIT for original prototype/product source, subject to owner decision;
no LICENSE is silently selected and the artwork receives no new license here.
A full product's terminal engine and its license are deliberately undecided.
