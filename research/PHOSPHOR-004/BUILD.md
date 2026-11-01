# Issue #7 build, architecture and reproduction

Base: `2109428144c075b5e0ebc89f8ea0ef85dfda3a29`, accepted `main` merge of PR #6.
Selected from project board 8: issue #7 **Ready / P0**, the only open issue; no
open PRs. Branch `issue-7-basic-terminal`. No project AGENTS.md was present.
Issue #7 expressly authorizes the basic terminal milestone, superseding the
old plan's “no PTY” restrictions for prior disposable experiments only. It does
not authorize full product implementation or unrelated issues.

## Foundation and dependencies

Use [Paul Evans's libvterm](https://www.leonerd.org.uk/code/libvterm/) for terminal
state and escape parsing, rather than implement a new parser. Its API separates
screen state and keyboard encoding from UI and I/O. Upstream version 0.3.3 is
MIT licensed; retain its copyright/license when redistributing compiled code.
The upstream license is retained in `evidence/libvterm-LICENSE.txt`. This task
uses its unmodified C sources, not the unrelated similarly named libraries.

Archive: `https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz`, downloaded
over HTTPS into `/tmp/phosphor-004/`, extracted there without installation.
`evidence/libvterm-source.sha256` records the downloaded bytes; this is provenance,
not an upstream signature verification claim. Normal builds can use Fedora's
libvterm-devel package instead; CMake does not fetch dependencies.

Qt Core/Gui/Quick obligations remain as recorded in prior research; no project
license is changed. The small original shader adds Qt's **build-time** qsb tool
via ShaderTools. [Qt's licensing page](https://doc.qt.io/qt-6/qtshadertools-index.html)
identifies qsb as GPLv3/commercial and the runtime library as LGPLv3/GPLv2/commercial
since Qt 6.3. This app does not call or link QShaderBaker/ShaderTools at runtime;
qsb processes our GLSL into resources. Shipping Qt/tool binaries still needs the
applicable notices/source/relinking compliance review. No distribution or release
is authorized here. [Qt custom-material documentation](https://doc.qt.io/qt-6/qtquick-scenegraph-custommaterial-example.html)
was consulted for the public QSGMaterialShader synchronization contract.

## Local toolchain

Reuse the already-extracted Qt 6.11.2 / CMake and compiler headers described in
PHOSPHOR-002 and PHOSPHOR-003. No package install or system changes:

```sh
curl -fL https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz \
  -o /tmp/phosphor-004/libvterm-0.3.3.tar.gz
tar -xzf /tmp/phosphor-004/libvterm-0.3.3.tar.gz -C /tmp/phosphor-004
bash research/PHOSPHOR-004/tools/build-local.sh clang
bash research/PHOSPHOR-004/tools/build-local.sh ubsan
```

The script uses Clang 20 and the prior temporary Fedora libstdc++ 16 headers;
UBSan is trap mode because that Clang runtime archive is unavailable locally.
Unlike the previous issue, the sanitizer covers the compiled libvterm C sources
as well as our C++ adapter. It does not instrument Qt or the proprietary driver.
The retained optimized-GCC material failure was **not resolved or reclassified**;
new native evidence uses Clang. Historical GCC evidence remains in PHOSPHOR-003.

Final regression tests also cover EOF while the shell remains alive (bounded
cleanup after 250 ms) and propagation of a wide cell’s background to its
continuation cell. Earlier failing assertions are retained; final Clang and
UBSan logs are `clang-final-validated.log` / `ubsan-final-validated.log`, with
individual checks in `clang-test-details.log` / `ubsan-test-details.log`.

The initial build passed, but native validation exposed a missing QSB batchable
variant/texture-commit step. The initial failed pilot is retained. A strengthened
Ctrl+C test initially expected a CRLF adjacent to output, overlooking Bash's
bracketed-paste control bytes; raw transcript confirmed actual interruption.
The assertion now requires the standalone output marker, and readline insertion
requires the actual `hello` result. Earlier failed logs are retained; final test
logs and screenshots are identified in REPORT.md.

## Architecture

`terminal.*`: libvterm adapter, bounded scrollback and immutable-by-value visible
cell snapshots. No PTY or GPU dependency. Unknown protocol payloads have no
application-side command/clipboard/desktop handlers.

`pty.*`: Linux master/slave allocation, initial termios inherited from the PTY,
new controlling session, explicit exec environment, nonblocking Qt socket
notifiers, partial write queue, EOF/waitpid, TIOCSWINSZ and owned-session cleanup.
Exec argv/environment are prepared before fork; child uses descriptor/session
setup then execve, closing inherited descriptors with Linux close_range.
Read work is bounded per activation; output rendering is coalesced in main.

`view.*`: synchronized scene-graph callback reads a coherent screen snapshot,
reuses the original material constructor, copies premultiplied glyph pixels into
paged atlases and submits bounded ordered triangles. One shader flag preserves
accepted RGBA exactly for default text; explicit ANSI colors use its brightest
channel times the requested RGB. Backgrounds/cursor use separate color geometry.
Textures/geometry are render-thread-owned; old geometry is removed before page
replacement. Atomic metrics bridge render/GUI threads. Both basic-loop validation
and prior engine constraints remain explicit; this issue does not validate other
RHI backends or a production threaded rendering configuration.

## Native reproduction

`tools/native-checks.py` records exact Qt key actions, commands, real framebuffer
and Spectacle captures. It refuses to overwrite named action files. It covers a
matched prior fixture via actual `cat` output, commands, errors, colors, Ctrl+C,
line editing, Ctrl+D/L, 1,200 output lines, history, resize, nano entry/exit,
fastfetch, the configured shell and a native UBSan run. It writes only test files
under `/tmp/phosphor-004` and the evidence directory. The temporary KWin helper
focuses **only** this app and records geometry; it unloads itself, writing no
persistent rule. No desktop output modes/scales are changed.

`tools/measure.py` performs two runs each of this terminal and the unchanged
issue #5 GPU fixture at physical DP-2 1920×1080. It records five one-second idle
and six one-second output samples per run. The terminal producer writes 20 lines
of 140 ASCII columns per 16 ms batch for ten seconds, through real Python/Bash/PTY
I/O. The fixture instead changes its deterministic text every 16 ms. These are
**different workloads**: the fixture is context for added terminal overhead,
not a matched speedup experiment. Process CPU excludes shell/producer children;
GPU/power values cover the entire shared desktop. Snapshot deltas separate warm
parse/render time and glyph texture replacement. Final terminal capture occurs after the measured output interval; the fixture
does not capture during sampling. The first dataset included a fixture capture
during idle and is retained under `evidence/performance/`, excluded from final
tables; all four runs are repeated under `evidence/performance-final/`. No FPS/latency claim follows.
Physical 4K remains unavailable on the DP outputs and is not newly measured.
