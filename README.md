# Project PHOSPHOR

PHOSPHOR is investigating a native Linux terminal with the approved metallic,
illuminated typography. The four concept images in this repository remain the
visual specification. They are artwork, not screenshots of an implementation.

The project is in feasibility testing. See the [development plan](Development_Plan.md).

- [Issue #1 research and acceptance status](research/PHOSPHOR-001/REPORT.md)
- [Isolated Molten Amber shader experiment](prototypes/ghostty-molten-amber/README.md)

Issue #1's feasibility investigation is complete with an **owner-decided NO-GO
for the tested Ghostty config-only post-processing approach**. Native screenshots
and measurements establish the visual limitations; the approved design remains
unchanged. See the [final report](research/PHOSPHOR-001/REPORT.md) for evidence
and unverified testing limits. The [stop record](research/PHOSPHOR-001/STOP_RECORD.md)
is a historical snapshot from before review preparation. Any further investigation
or product implementation requires separate owner authorization.

Issue #3 evaluates an isolated C++20/Qt 6 glyph-aware alternative:
[prototype and run instructions](prototypes/qt-molten-amber/README.md),
[PHOSPHOR-002 evidence and recommendation](research/PHOSPHOR-002/REPORT.md).
This is a visual-only feasibility experiment, not a terminal implementation.

Issue #5 evaluates GPU composition of the accepted refined 16 pt material:
[isolated CPU/GPU atlas prototype](prototypes/qt-gpu-atlas/README.md),
[PHOSPHOR-003 measurements, screenshots and conditional recommendation](research/PHOSPHOR-003/REPORT.md).
Owner review is required; no full terminal development is authorized by this experiment.
