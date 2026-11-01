# Issue #16 — Vertical scrollbar

Selected 2026-11-01 from owner project board 8: the **only Ready issue**, P2.
Issues #9–#15 were Backlog; there were no open PRs. Default branch `main` and
existing clean issue branch `16-ux-vertical-scrollbars-for-terminal-windows-and-scrollback`
both started at accepted `93ee665b7a32e3bb72ec27f4b2bb49ad4d361384` (PR #8).
No AGENTS.md was present. This work changes only the bounded terminal prototype
and its validation/docs; it does not authorize subsequent product work.

## Result

A ten-logical-pixel amber track and thumb live in each TerminalView's existing
24-pixel right gutter. Glyphs, shader, material generator and row/column formula
are unchanged. The thumb is proportional to visible / total retained rows,
with a 24-pixel minimum; its position maps oldest history to top and live output
to bottom. Dragging, track-page clicks, wheel and Shift+PageUp/Down share the
same clamped terminal offset. Clicking the bar restores terminal keyboard focus.
Qt owns mouse grabbing through the normal accepted press/release path, following
[QQuickItem's public event contract](https://doc.qt.io/qt-6/qquickitem.html).

Incoming output keeps the same historical rows visible until eviction; at the
1,000-line limit the oldest available row replaces evicted content. Returning
live follows new output. Resize preserves offset (including any rows libvterm
pushes into history), retaining the existing no-reflow/crop behavior. Empty or
short screen content with no history and alternate-screen programs have a
full-height dimmed inactive thumb. Exiting alternate screen returns live.
Selection is not implemented and remains issue #10; future selection must use
stable history identity and handle eviction explicitly. Future tabs/splits
constructed from TerminalView receive the same scrollbar without window-level
wiring; neither feature is implemented here.

## Executed validation

- System Clang **22.1.8**, Qt **6.11.2**, libvterm **0.3.3**; Release build passed.
  Existing parser/ANSI/Unicode/keyboard/PTY/process tests and new scrollbar
  geometry, drag, track, wheel, anchoring, resize, eviction and alternate-screen
  checks passed: **1/1 CTest suite**, 0 failures (0.59 s).
- UBSan trap-mode Release suite: **1/1 passed**, 0 failures (0.59 s). Covers our
  C++ code; system Qt/libvterm and GPU driver are not instrumented.
- Actual native Wayland/OpenGL window on DP-2, 1920×1080 display, DPR 1;
  1280×900 and 800×600 clients. No display modes or settings were changed.
  Input is **scripted Qt window events**, not a human hardware-mouse test.
- Final native snapshots: bottom offset 0; drag to 112; track to 137; wheel to
  140; keyboard to 165. Separate continuous-output captures retain byte-identical
  visible text while offset increases. Dragging down returns offset 0. Eviction
  clamps history and offset to 1,000; resize retains that offset and produces
  59×16 cells. Native nano remains at offset 0 under wheel input, preserves its
  screen, and restores normal live terminal on exit. Subsequent `echo FOCUS_OK`
  reaches the shell through the window's keyboard route.
- Matched accepted-main/current screenshot comparison: the **only different
  pixels** are in `(1266,20)–(1276,880)`, the scrollbar strip. All text, ANSI
  colors, backgrounds and cursor pixels outside that strip are identical.
  Both windows retain 97×26 cells at 1280×900. Source artwork and prior research
  and prototypes are unchanged.
- `tools/verify.py`: all retained native-state, text and pixel assertions pass.
  `git diff --check` passes.

## Actual application screenshots

Unedited client framebuffers from the native running application:

| State | Evidence |
| --- | --- |
| Short/no history | [short](evidence/short.png) |
| Long history, live bottom | [bottom](evidence/bottom.png) |
| Dragged partway into history | [drag](evidence/drag-middle.png) |
| Continuous output, anchored viewport | [before](evidence/anchor-before.png), [after](evidence/anchor-after.png) |
| Oldest retained history after eviction | [evicted](evidence/evicted.png) |
| Resized viewport | [resize](evidence/resize.png) |
| Alternate-screen nano | [nano](evidence/nano.png) |
| Shell input after scrollbar use | [focus](evidence/focus.png) |
| Matched material comparison | [accepted main](evidence/baseline-short.png), [current](evidence/current-short.png) |

Visual inspection found a separate, unobtrusive bar, readable unchanged glyphs,
visible cursor and no overlap with the text area. Owner aesthetic acceptance and
physical mouse feel remain for review. These are app framebuffers, not full
desktop/compositor-decoration captures or concept artwork.

## Bounded performance observation

Same machine, compiler, native screen and Python producer printing 2,000 lines
with 2 ms sleeps. One baseline and one current run, not a statistical benchmark.

| Observation | Accepted main | With scrollbar |
| --- | ---: | ---: |
| Sampled idle interval | 2.031 s | 2.000 s |
| Idle renders / uploads | 0 / 0 | 0 / 0 |
| Sampled output interval | 3.500 s | 3.301 s |
| Render calls | 190 | 178 |
| CPU render work | 28.266 ms | 26.470 ms |
| CPU work / render | 0.1488 ms | 0.1487 ms |
| Warm texture uploads | 0 | 0 |

Baseline's historical coarse-timer harness makes its interval slightly longer;
compare normalized work cautiously. Raw `/proc` process CPU ticks and metric
snapshots are retained. No speedup, GPU utilization, input latency or physical
4K claim is made. Rendering adds two solid rectangles, no glyph textures and no
animation/idle timer. Driver timing and whole-desktop GPU load were not measured.

## Reproduction

Build outside the repository using installed development dependencies:

```sh
cmake -S prototypes/qt-terminal -B /tmp/phosphor-016-build \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
cmake --build /tmp/phosphor-016-build -j4
ctest --test-dir /tmp/phosphor-016-build -V
```

For UBSan, use a separate `/tmp/phosphor-016-ubsan` build with
`-DCMAKE_CXX_FLAGS='-fsanitize=undefined -fsanitize-trap=all'`.
`tools/make-actions.py` and `tools/anchoring.py` generate machine-local absolute
capture paths under this issue's evidence directory. They overwrite their own
named action files; use a fresh checkout/output copy to preserve this evidence.
Run each with the matching `--test-script` path, e.g.:

```sh
QT_QPA_PLATFORM=wayland QSG_RHI_BACKEND=opengl QSG_RENDER_LOOP=basic \
QT_FORCE_STDERR_LOGGING=1 /tmp/phosphor-016-build/phosphor-terminal \
  --screen DP-2 --clean-bash --seconds 20 \
  --test-script research/PHOSPHOR-016/evidence/actions.json
python3 research/PHOSPHOR-016/tools/verify.py
```

Commands execute ordinary shell input, run bounded Python output and open nano
on an unsaved `/tmp` filename. They do not save an editor file or change user
configuration. `tools/compare.py` expects accepted-main source built separately
at `/tmp/phosphor-016-baseline`, runs both binaries, and records CPU samples.
Pillow is used only for evidence verification, not the terminal runtime.

The first pilot's drag did not move: coarse single-shot timers can reorder close
press/move/release actions. An instrumented run confirmed correct native dragging;
the harness now uses precise timers and timestamped input routed through the
window. Earlier pilot evidence/logs remain in `pilot/` and `timestamp-pilot/`;
final runs are `native-final.log` and `anchoring-final.log`. The portal app-ID
warning in logs is pre-existing desktop registration behavior, not a scrollbar
failure. No app installation is performed.

## Remaining acceptance / owner decision

Human physical mouse interaction and aesthetic approval remain unverified;
scripted native events and inspected captures are supporting evidence. No physical
4K, fractional-DPR, other GPU backend or accessibility conformance certification
is made. Previous GCC/DPR/Unicode limits remain unchanged. There is no selection,
clipboard, new theme, terminal engine, tab, split or release work. Review and merge
this single issue PR only if its behavior and visual result are acceptable.
