# Native validation tool snapshots

These are the small Python scripts executed for this issue on the owner's
Fedora/KDE/Wayland machine. Paths intentionally identify this checkout and
`/tmp/phosphor-001-native`; they are evidence/reproduction tools, not product
code or a general terminal test framework. Python D-Bus/GLib and NVIDIA tools
were already installed. Do not run them against unrelated windows.

- `kwin.py` loads an ephemeral KWin script, selects only application class
  `org.phosphor.Spike001`, optionally sets its frame geometry/focus, reports
  client/buffer geometry through a temporary D-Bus service, and unloads the
  script. It does not save window rules or change display modes.
- `workload.py` runs inside Ghostty's PTY. A control file chooses idle or rapid
  scroll; scroll emits 32 lines per approximately 1/60 second. It records PTY
  geometry, output counts and receipt of a known paste marker. It does not
  record arbitrary keyboard input. Its alternate-screen mode is supplemental;
  the separate existing curses fixture is also used for native TUI inspection.
- `bench.py` runs baseline/prototype separately at exact 1920×1080 and
  3840×2160 client sizes. Each resolution has three repetitions, with mode
  order alternating. Each process gets 10 seconds to warm up, followed by
  30 one-second idle samples and 30 scroll samples. Configuration and shader
  snapshots/hashes accompany the final dataset. Source files must stay fixed
  throughout a run. The driver obtains CPU time from the Ghostty process's
  `/proc/PID/stat`, and GPU telemetry from `nvidia-smi`. The CPU result excludes
  the workload child and is expressed as a percentage of one logical core.

Whole-GPU readings include the desktop compositor and other applications;
these are neither per-process GPU costs nor GPU frame timings. The oversized
4K window is rendered by the real Wayland application on NVIDIA, but is partly
clipped by the active 1440p/1080p desktop. No physical 4K display mode is enabled.
The read-only compositor queries verify the allocated client/buffer dimensions.

The raw samples and per-run logs are authoritative for which runs completed.
An interrupted diagnostic dataset from the old color-space setting is retained
under `evidence/native/preflight/` and is excluded from final summaries.
