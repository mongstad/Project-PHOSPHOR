# Molten Amber composited-frame experiment

For [PHOSPHOR-001](https://github.com/mongstad/Project-PHOSPHOR/issues/1).
**Executed in a locally built Ghostty 1.4.0 on Fedora/KDE/Wayland; rejected by
the owner with a NO-GO outcome.** See the
[final native findings](../../research/PHOSPHOR-001/REPORT.md). This retained experiment tested
whether a small local filter produces useful cores, contours, and glow at normal
font sizes. It does not establish a metallic glyph material or concept fidelity.
Read the [research report](../../research/PHOSPHOR-001/REPORT.md) first.

## Prerequisite and isolation

Use an owner-approved Ghostty binary, preferably **1.4.0**, with its matching
resources. Installation and dependency changes require the owner's approval.
The launcher never installs software. An external binary can be supplied with
`GHOSTTY=/absolute/path/to/ghostty`.

Run from this directory:

```bash
bash run.sh baseline
bash run.sh prototype
```

Each command starts a separate process with a temporary XDG config directory,
default config files disabled,
the same checked-in config, and a clean Bash shell. Only prototype mode adds
the shader. Existing Ghostty instances do not receive these settings. Konsole
profiles, the default terminal, and display modes are not changed. The shell
does not load user startup files, and its Bash history goes to `/dev/null`.
The temporary config directory is removed when Ghostty exits. This also
contains the template Ghostty creates before parsing its CLI config flags.
The white cursor and blue selection are experimental visibility controls;
they are not claims that the approved amber cursor/selection design is met.

The shader uses 17 input samples on its full path, a four-physical-pixel halo,
and no time input. The shared config disables shader animation. Keep the same
font, scale, geometry, content, and focus state when comparing modes.
The experiment uses `alpha-blending=linear-corrected` and converts its material
colors to linear space. A native run exposed a lifted background with the
earlier `native` setting; the diagnostic captures are retained separately.

## Archived native validation procedure

The runbook below preserves the planned procedure for reproducibility. The
[final report](../../research/PHOSPHOR-001/REPORT.md) records which parts ran and
which remain unverified. It is not an instruction to resume testing, install
software, or begin another experiment after the owner's NO-GO decision.

1. Record `ghostty +version`, GPU/driver, monitor refresh, KDE/Wayland versions,
   scale and exact **terminal surface pixel dimensions**, plus the prototype
   commit and shader/config hashes. Inspect Ghostty logs for shader failures;
   `+validate-config` alone does not establish that the shader loaded.
2. Use 12 pt first. In each clean shell run `python3 fixtures.py`, then
   `fastfetch`. Inspect punctuation, ANSI/truecolor, inverse text, the amber
   background/solid-block counterexample, Unicode/fallbacks and box drawing.
   Run the actual normal shell/prompt separately as an explicitly recorded
   variation; this clean Bash prompt is not the user's configured prompt.
3. Select the fixture line; copy and paste it, inspect both focused and unfocused
   states. Check block, bar and underline cursors (e.g. `printf '\e[5 q'` for a
   blinking bar, `printf '\e[0 q'` to restore the terminal default). Check that
   hiding the cursor in a TUI does not leave an old cursor exclusion patch.
4. Run `python3 fixtures.py scroll --seconds 30`. Resize while scrolling;
   check clipping, shimmer, disappearing strokes, background contamination and
   behavior at all four surface edges. Then run
   `python3 fixtures.py fullscreen --seconds 30`; resize, press `q` to exit,
   and verify the normal screen/cursor returns. This small curses program is
   a test workload, not a terminal emulator. Also try an available real TUI.
5. Repeat baseline/prototype at 1920×1080 and 3840×2160 **surface pixels**
   (2560×1440 optional). A fullscreen 1080p monitor does not provide a 1080p
   terminal surface when decorations consume pixels. Confirm the surface size
   from renderer instrumentation or a precise content-area measurement.
   Do not change desktop display modes without owner approval. A virtual or
   offscreen 4K run must be labeled as such, not native 4K display validation.
6. For each mode/workload, warm up 10 s, then sample idle, typing, and scroll
   for 30 s, three repetitions, alternating mode order. Retain raw samples.
   Observe process CPU with `pidstat -p PID 1 30` **if already installed** and
   whole-GPU utilization/power with:

   ```bash
   nvidia-smi --query-gpu=timestamp,utilization.gpu,power.draw,memory.used --format=csv -l 1
   ```

   Stop the sampler with Ctrl-C after the observation window. These are
   whole-GPU observations and include other applications/compositing; they
   are not per-process shader timings. Record idle redraw behavior with the
   cursor blink controlled equally in both modes. Record typing latency and
   dropped frames separately; CPU/GPU percentages alone do not prove
   responsiveness. If frame timing tools are unavailable, leave timing
   unverified. Do not install measurement tools without owner approval.
7. Save **unedited screenshots of the actual Ghostty window**, including its
   native chrome, for baseline and prototype at each resolution. Keep original
   files, timestamps, and checksums. Place reference/baseline/prototype images
   adjacent in the review document; any crops or zooms are supplemental and
   must be labeled. Compare against the repository's Molten Amber artwork.
   Never use this artwork or synthetic test output as implementation evidence.

Reject a post-process-only solution if it produces merely orange text, glowing
cell rectangles, lost syntax colors, or illegible normal-size glyphs. Record
chrome gaps separately. The owner decides the feasibility gate.

## Automated checks

Python's standard library and Bash are sufficient for launcher checks:

```bash
bash -n run.sh
python3 -B -m unittest discover -s checks -p 'test_*.py' -v
```

An optional compilation check uses an **existing** `libshaderc_shared` and the
unchanged prefix from Ghostty commit
`83fb95624b0439ee660c2679b43b38c8b63769cd` (v1.4.0):

```bash
python3 -B checks/compile_shader.py /path/to/ghostty/src/renderer/shaders/shadertoy_prefix.glsl
```

The check verifies the prefix hash, compiles GLSL to SPIR-V, and rejects an
intentionally invalid negative control. It does not exercise Ghostty's complete
compiler pipeline, OpenGL driver, terminal rendering, or performance.

## Cleanup

Exit the disposable shell/window; the launcher cleans its temporary XDG config.
No user config needs restoring. The prototype is
entirely in this directory; remove a disposable checkout if no longer needed.
Preserve all screenshots and measurement records before removing anything.
If an owner-approved binary was installed separately, handle its removal
separately; this launcher creates no package, desktop entry, or default handler.
