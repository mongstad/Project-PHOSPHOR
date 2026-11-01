# Final validation, 2026-11-01

- Clang 20 release build completed; focused glyph suite passed all nine
  point-size/DPR combinations, including non-flat opaque interiors and separate
  edge/glow controls. Final CTest runtime: 10.37 s.
- Clang undefined-behavior sanitizer in trap mode: same suite passed in 11.81 s.
  Normal sanitizer runtime was unavailable; the failed attempt is preserved.
- Native window: 14 capture cases, 1080p and actual independent HDMI 4K at 30 Hz,
  12/14/16 pt, same-geometry baseline/material, glow off, and DPR 1.5/resizing.
  Eight exploratory measurement cases yielded 80 approximately one-second rows.
- Final native smoke confirms NVIDIA OpenGL ES 3.2 / Qt Quick OpenGL QRhi and
  normal shutdown. A portal registration warning is expected without an installed
  desktop entry; this sample intentionally does not install one.
- Offline `python3 research/PHOSPHOR-002/tools/audit.py`: passes original artwork
  hashes, unchanged historical paths, geometry, sample counts, crop pixel equality,
  document links and exact original/final KScreen output JSON equality.
- Five approved PNG SHA256 values match. No PHOSPHOR-001 file or prototype,
  development plan, user terminal/default setting or application setting changed.
- Source/docs/helper `git diff --cached --check` passes. Raw CSV retains Python's
  CRLF record endings and raw CMake failure logs retain upstream whitespace;
  those evidence-only whitespace warnings are intentionally not normalized.
- GCC optimized rendering failed the halo check; that is an explicit unresolved
  limitation, not reported as passing validation. See BUILD.md and diagnostics.

Only the new prototype/research directories and README links are part of this
change. No full terminal work, other theme, automatic continuation, merge,
issue closure or release was performed. Recommendation: NO-GO; owner review.
