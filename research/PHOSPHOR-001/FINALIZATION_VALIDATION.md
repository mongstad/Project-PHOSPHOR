# Issue #1 finalization checks — 2026-11-01

The owner reviewed the native captures and decided **NO-GO for the tested
Ghostty config-only post-processing approach**. Documentation finalization
does not require repeating the rejected rendering experiment or completing
every planned interaction test.

The native work was preserved first in commit
`691593b199fd1989bb3df75a15b37a87c9da62f2`. All 116 files inventoried at takeover
match that commit byte-for-byte. Original evidence, prototype source, Phase 0
report and historical stop record remain intact; current documentation records
the later owner decision separately.

Executed focused checks:

- `bash -n prototypes/ghostty-molten-amber/run.sh`: passed.
- `python3 -B -m unittest discover -s prototypes/ghostty-molten-amber/checks -p 'test_*.py' -v`:
  **8 tests passed**, including early template isolation/cleanup and preservation
  of an existing config sentinel. These tests use a stub, not a native window.
- `python3 -B prototypes/ghostty-molten-amber/checks/compile_shader.py /tmp/phosphor-001-native/ghostty-1.4.0/src/renderer/shaders/shadertoy_prefix.glsl`:
  retained shader compiled to **11,344 bytes of SPIR-V**; invalid GLSL negative
  control rejected. No shader change or full renderer experiment was made.
- All five approved artwork hashes match the original manifest and accepted
  `main`; the accepted development plan is unchanged.

The [offline evidence audit](tools/check_evidence.py) checks the raw dataset,
report statistics, geometry, logs, snapshots, image files, local documentation
links, historical records, and existing build provenance without running
Ghostty or a native helper. Its exact output is retained in
[finalization-audit.txt](evidence/finalization-audit.txt).
The audit uses the already-present Pillow and PyNaCl libraries and the retained
local build/downloads for provenance checks; it installs nothing.
It passed: all 720 samples and twelve successful runs are consistent; all eight
summary rows recompute exactly; fourteen PNG files decode; four final capture
sizes match their geometry; twenty-six session-file hashes match metadata;
ten Python snapshots parse; and all 53 local Markdown/image links resolve.
The existing source signature and trusted comment reverify, and source,
compiler, local binary and fifty RPM hashes match the preserved provenance.

The complete original capture tree is covered by
[native.sha256](evidence/native.sha256): **137 files** match both the working
files and preservation commit byte-for-byte. Raw CSV files retain their original
CRLF records; raw logs retain captured spaces and blank lines. A whole-tree
`git diff --check` flags those captured bytes. They were deliberately preserved
rather than normalized. Documentation/source whitespace checks use:

```bash
git diff --cached --check -- . ':!research/PHOSPHOR-001/evidence/native/**'
```

No live rendering, display, GPU measurement, keyboard injection, TUI experiment,
dependency installation, upstream renderer modification, or system change was
performed during finalization. The remaining unverified tests are listed in
the [final report](REPORT.md); none is relabeled as passed. PR #2 is prepared
for review of the completed NO-GO investigation, not acceptance of the prototype.
