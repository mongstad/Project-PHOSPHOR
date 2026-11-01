# Issue #16 / PR #17 — Molten Amber scrollbar styling

Owner-requested visual pass on the existing issue branch, based on `7e83611`.
The owner approved drag, track navigation and resize anchoring. This follow-up
preserves those behaviors, hit areas, scrollbar dimensions and terminal columns.
No glyph material, shader, terminal-state or PTY code changes.

The thumb now has a narrow illuminated gold edge, a gold-to-copper beveled face,
subtle top/bottom highlights and two low-intensity halo steps. The halo extends
only two pixels laterally, inside the existing right gutter. The track has quiet
warm edge lines. Hover brightens the thumb, pressing/dragging strengthens the
highlight, and release/ungrab clears the pressed state. Empty history and
alternate screen retain a subdued, inactive full-height thumb.

All styling uses solid scene-graph geometry with opaque colors preblended for
the existing dark gutter; it introduces no blur pass, texture upload, animation
or dependency. Hover redraws only when crossing track/thumb state boundaries.
The test-only input harness adds a no-buttons `mouse: hover` action.

## Actual running application captures

Native Wayland/OpenGL on the existing DP-2 1920×1080 display, DPR 1; no display
configuration changes. These are unedited client framebuffers, not concepts.

- [At rest](evidence/rest.png)
- [Hover](evidence/hover.png)
- [Pressed](evidence/pressed.png)
- [During drag](evidence/dragging.png)
- [Released](evidence/released.png)
- [Resized](evidence/resize.png)
- [Alternate-screen nano](evidence/nano.png)

Visual inspection confirms a slim warm metallic highlight, clear interaction
states and no text overlap. Owner judgment of the final appearance remains the
acceptance gate.

## Executed checks

- Clang Release CTest: 1/1 suite passed; existing parser, ANSI, PTY, keyboard,
  lifecycle and scrollbar geometry/navigation regressions remain passing.
- UBSan trap-mode CTest: 1/1 suite passed. System libraries/driver are not
  instrumented.
- Repeated native scripted window events confirm track/wheel/keyboard navigation,
  drag, live return, continuous-output anchoring, 1,000-line eviction, resize,
  nano alternate-screen isolation and subsequent shell input.
- Pixel comparison of the same bottom-of-history view against the retained
  pre-styling image confirms all pixels left of the right gutter are identical.
  Terminal columns remain 97 at 1280×900 and 59 at 800×600.
- Separate screenshot assertions verify that rest → hover → pressed changes
  pixels only within the scrollbar area; dragging changes offset and release
  keeps that offset. See `evidence/verification.log` and `tools/verify.py`.
- The initial hover capture revealed that first entry over the thumb needed the
  same update as hover movement. `hoverEnterEvent` now handles it; earlier
  evidence is preserved in `evidence/hover-pilot/`.

Prior issue evidence is untouched. New evidence, including checksums and action
files, is isolated under this directory. Scripts use the same installed toolchain
and `/tmp/phosphor-016-build` as the parent report. Generate actions with
`tools/states.py`, `tools/make-actions.py` or `tools/anchoring.py`; launch with
`--clean-bash --screen DP-2 --seconds 20 --test-script <generated-actions.json>`
and `QT_QPA_PLATFORM=wayland QSG_RHI_BACKEND=opengl QSG_RENDER_LOOP=basic`.
Generators/captures overwrite their own named styling outputs; use a separate
checkout/copy to preserve these records. `tools/verify.py` requires Pillow only
for image comparison. Raw fixed-width screen dumps retain trailing spaces.

This pass uses scripted pointer/key events, not new human mouse testing. No new
4K, fractional-DPR or GPU-performance measurement is claimed. Existing platform
and Unicode limits remain. No merge, release, theme expansion or unrelated work.
