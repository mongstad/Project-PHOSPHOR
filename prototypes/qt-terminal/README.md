# PHOSPHOR basic interactive terminal (issue #7)

Linux-only C++20 / Qt 6 milestone: a real PTY-backed interactive shell with the
accepted refined 16 pt Molten Amber GPU glyph material. This is not a production
terminal or a Konsole replacement. Previous prototypes and artwork are unchanged.

## Build and run

Dependencies: CMake 3.21+, C++20 compiler (**Clang recommended**), Qt 6.6+ Core,
Gui, Quick and ShaderTools development tools, and **libvterm >= 0.3.3** development
files/pkg-config. Tested: Qt 6.11.2, libvterm 0.3.3, Clang 20, Fedora 44 Wayland.
No private Qt API is used. ShaderTools/qsb runs at build time only.

```sh
cmake -S prototypes/qt-terminal -B /tmp/phosphor-terminal-build \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build /tmp/phosphor-terminal-build
ctest --test-dir /tmp/phosphor-terminal-build --output-on-failure
/tmp/phosphor-terminal-build/phosphor-terminal --screen DP-1
```

Use an actual connected screen name (`--screen`, default DP-1). `--width` and
`--height` set initial client dimensions; `--fullscreen` is optional. The app
opens a normal native window, launches the account's configured shell from
`getpwuid()` with `-i`, and falls back to `/bin/bash` if its executable is invalid.
It inherits the working directory, environment and normal user privileges.
`--shell /absolute/path` selects another interactive shell explicitly.
`--clean-bash` uses `/bin/bash --noprofile --norc -i` for reproducible tests.
No shell profiles, desktop settings or default terminal are modified.

An existing extracted upstream libvterm tree can instead be specified with
`-DLIBVTERM_SOURCE_DIR=/absolute/path/libvterm-0.3.3`. CMake never downloads it.
[Machine-local reproduction](../../research/PHOSPHOR-004/BUILD.md) records the
exact local toolchains and source archive; no privileged install is required.

## Behavior and boundaries

- libvterm owns UTF-8 decoding, ANSI/VT state, cursor addressing/movement,
  LF/CR/BS/TAB, erase, autowrap, scrolling regions, SGR indexed/bright/truecolor,
  alternate screen and terminal-mode-dependent keyboard encoding. The tested
  subset is documented in the report; advertising `TERM=xterm-256color` permits
  ordinary readline/curses use but is **not** a conformance claim.
- Printable text, Return, Backspace, Tab, Escape, Ctrl combinations, arrows,
  Home/End/Insert/Delete/Page keys and F1–F12 go to the PTY. Shift+PageUp/Down
  and wheel navigate up to **1,000** history lines; ordinary typing returns live.
  History is unavailable in the alternate screen. Resize preserves/crops cells
  using libvterm without reflow or pulling old history into the resized screen.
- Default foreground is the unchanged amber material. Explicit SGR foreground
  colors tint its per-glyph intensity; backgrounds are separate opaque cell
  rectangles. Reverse, conceal, underline and strike are handled. Bold does not
  change the approved DemiBold font; italic/blink/double-size text are not styled.
  The steady underline cursor honors visibility, but not requested blink/shape.
- Supported presentation is fixed 16 pt at **DPR 1**. The issue #5 fractional-DPR
  discrepancy remains unresolved; no new parity claim is made. The optimized
  GCC material issue remains unresolved; use Clang.
- BMP single-cell characters use the accepted glyph generator. NFC-composable
  accents are normalized. Wide characters, non-BMP/emoji and combining clusters
  that do not reduce to one BMP character show U+FFFD, preserving their grid
  advance. This is explicitly limited Unicode presentation, not full shaping.
- Up to 512 glyphs are cached in 128-glyph GPU pages. ASCII/prior fixture glyphs
  are prewarmed; at most two additional glyphs are generated per frame. New
  glyphs temporarily show U+FFFD, and exceeding the cache cap keeps that fallback.
  Only changed pages upload, never the whole terminal surface. Cold generation
  remains about one second. Changed screen geometry is rebuilt, with each batch
  capped at 4,096 glyphs.
- OSC/DCS and unsupported controls are consumed by libvterm. No clipboard,
  hyperlink opening, title changes, image protocols, command execution callbacks
  or desktop actions are installed. Commands execute only through shell input.
  There is no clipboard paste/selection UI, IME integration or mouse reporting.
- PTY reads are nonblocking and budgeted at 64 KiB per activation; redraws are
  coalesced by a 16 ms timer. Writes queue partial data up to 1 MiB; overflow is
  rejected and logged. EOF, exec failure and shell exit are handled. Normal
  close sends HUP, then kills remaining processes in this PTY session and reaps
  the shell. Explicitly daemonized/new-session processes are outside that
  ownership; a forced app kill/crash cannot provide the normal cleanup guarantee.

## Tests and evidence

[PHOSPHOR-004 report](../../research/PHOSPHOR-004/REPORT.md) includes real running
application screenshots, pixel comparison, native checks, performance and
limitations. The tests run real unprivileged Bash processes; use a normal Linux
environment with `/dev/ptmx` access.

`--test-script FILE` is a local validation-only JSON array of timed Qt key events,
resize, framebuffer capture and text-dump actions after the first frame. It is
not a remote protocol. Inspect scripts before running: typed text executes in
the shell exactly as user input does. `--seconds` and `--metrics` provide bounded
runs and measurements. Evidence captures are unedited; derived crops are labeled.

Close the window or exit the shell to stop. No install, background service,
Konsole configuration or package release is produced by this milestone.
