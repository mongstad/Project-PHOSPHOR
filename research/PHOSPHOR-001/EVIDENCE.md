# PHOSPHOR-001 evidence index

The [final report](REPORT.md) records the owner's completed NO-GO decision for
the tested Ghostty config-only shader. Original captures and raw measurements
are retained without editing, trimming, line-ending normalization or removal.

| Record | Role and interpretation |
| --- | --- |
| [Final screenshots](evidence/native/screenshots/) | Four original native Spectacle PNGs, plus geometry records: baseline/prototype at 1920×1080 and 3840×2160 client sizes. PNG sizes include native decorations. 4K uses an oversized, partly clipped Wayland window, not an enabled physical 4K display. |
| [Final measurements](evidence/native/measurements/) | Twelve completed runs and 720 samples, progress/exit records, per-run geometry and logs, PTY states, source/config/launcher snapshots and their input hashes, and the original summary JSON. Only this dataset supplies the final report's performance table. |
| [Capture logs](evidence/native/) | Baseline/prototype capture logs, Ghostty version, build logs, signed-source provenance, signature, Zig URL/checksum and extracted Fedora RPM hashes. Failed build attempts are retained separately from successful native runs. See [BUILD.md](BUILD.md). |
| [Preflight](evidence/native/preflight/) | Original images and interrupted measurements from the earlier color-space configuration. `diagnostic-runs/` preserves the complete interrupted dataset. These do not contribute to final performance results. |
| [Session archive](evidence/native/session/) | Original local session files, including duplicate original PNGs, intermediate captures, logs, script snapshots, the initially generated template, and synthetic workload preparations. `metadata.json` records hashes, sizes and original modification times for the 26 session files copied during finalization. |
| [Validation tool snapshots](tools/) | The benchmark, PTY workload and temporary compositor helper as used. They are historical reproduction code with fixed local paths, not product code or an instruction to resume testing. |
| [Phase 0 report](PHASE0_REPORT.md) and [validation record](evidence/validation.txt) | Preserved initial research and checks from before native execution. Statements that Ghostty was missing or native tests had not run describe that earlier phase, not current status. |
| [Stop record](STOP_RECORD.md) | Unchanged historical record from when the owner stopped testing. Its local/unpublished status describes that point in time, before the later instruction to finalize and push PR #2. |
| [Artwork hashes](evidence/artwork.sha256) | All five approved original artwork files remain unchanged. The development plan is also unchanged. |

`session/show.sh` ran the fixture and fastfetch **without a logo** for the final
native captures. `fastfetch-amber.vt` was a separately generated stream; no
amber-logo native capture was completed. `terminal-sample.txt` was prepared for
an additional TUI check that did not run. Synthetic `.vt` files and unused code
paths are not native screenshot, interaction, or performance evidence.

The early `baseline.log` belongs to the tarball build whose version defaulted
to `1.4.0-dev+0000000`. Final capture/measurement logs use the local stable
`1.4.0` build. Intermediate geometry can reflect a pending asynchronous resize;
the final screenshot geometry records and benchmark checks establish the sizes
used in the final report.

The existing native work was first committed unchanged as preservation commit
`691593b199fd1989bb3df75a15b37a87c9da62f2`. Final documentation records the owner's
decision separately. [Native capture checksums](evidence/native.sha256) cover
every preserved native evidence file, including duplicates and diagnostics.
The external source archives, compiler and local build tree remain outside the
repository under `/tmp/phosphor-001-native`; none was deleted or rebuilt during
finalization. The PR preserves provenance rather than distributing those binaries.

No rendering experiment, dependency installation, system configuration change,
shader edit or alternate architecture work was performed during finalization.
