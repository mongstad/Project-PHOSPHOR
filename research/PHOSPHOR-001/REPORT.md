# PHOSPHOR-001 — Ghostty Molten Amber feasibility

**Date:** 2026-11-01. **Status:** Phase 0 researched; isolated experiment
prepared; Phase 1 native validation blocked. **Issue:** [#1](https://github.com/mongstad/Project-PHOSPHOR/issues/1).

**Recommendation: NO-GO for advancing to product implementation on the present
evidence.** This is an evidence stop, **not** an empirical rejection of Ghostty.
The foundation's GO / CONDITIONAL GO / NO-GO gate remains undecided: there are
no real Ghostty screenshots or baseline/prototype performance measurements.
Do not accept the shader's successful compilation as concept fidelity.

The smallest next action is owner approval of a Ghostty installation/build or
provision of an existing approved binary, then the native experiment below.
No installation, default-terminal change, Konsole edit, renderer fork, or full
product implementation was performed.

## Basis and local availability

The sole Ready item was #1, P0, on the
[project board](https://github.com/users/mongstad/projects/8). There were no
previous PRs or issue comments. The existing issue branch was clean and exactly
at accepted `main`, `ba63f6f54075f80005f5dfde2fcf1f08823b49d5`, after fetching.
The repository had the development plan and five artwork files, with no README
or AGENTS.md. The accepted plan and all artwork are unchanged.

`ghostty` was absent from PATH, RPM installation, the installed Flatpak list,
and the inspected local binary directories. No installed version can be
reported. The source research targets **Ghostty 1.4.0**, released 2026-10-27
according to the [official release notes](https://ghostty.org/docs/install/release-notes/1-4-0),
tag `v1.4.0`, resolved commit
`83fb95624b0439ee660c2679b43b38c8b63769cd`. An unbuilt source checkout was
downloaded for inspection; no dependencies were installed.

Host checks found Fedora 44 KDE, Plasma 6.7.5, a Wayland session, Noto Sans Mono,
and an NVIDIA RTX 3070 Ti (8192 MiB), driver 615.78.08. Active outputs were
1920×1080 at 143.98 Hz and 2560×1440 at 144 Hz, both scale 1. The connected
4K-capable output was **disabled**; its listed 3840×2160 mode is 30 Hz.
No display settings were changed. Sandboxed NVIDIA/display probes failed;
read-only host probes succeeded. This is an access distinction, not evidence
of a broken host driver. See [environment evidence](evidence/environment.txt).

## Verified source capabilities

All source links in this section are pinned to the inspected release.

| Area | Source-backed finding and implication |
| --- | --- |
| Linux renderer | [OpenGL.zig](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/renderer/OpenGL.zig) requires OpenGL 4.3, binds EGL/OpenGL explicitly, uses three frame slots and +Y-up custom-shader coordinates. The 1.4 pipeline exports frames to GTK using DMABUF, with a CPU presentation fallback. This changes the Linux performance baseline from older GtkGLArea releases. |
| Native application | [GTK render surface](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/apprt/gtk/class/render_surface.zig) imports rendered frames into GTK. Ghostty supports Wayland; actual KDE/NVIDIA/DMABUF behavior on this host is untested. A GTK application is not proof of KDE-native chrome fidelity. |
| Shader API | [Prefix](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/renderer/shaders/shadertoy_prefix.glsl): GLSL `mainImage(out vec4, in vec2)`, `iResolution`, one `sampler2D iChannel0`, cursor information, palette and global colors. Channels 1–3 are unused. There is no exported glyph atlas, coverage/distance field, per-cell foreground/background, selection geometry or per-character coordinate input. |
| Shader input | [generic.zig drawFrame](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/renderer/generic.zig) draws backgrounds, text and graphics before the custom pass. It binds only the composited texture to that pass. Shader chains use front/back intermediate textures, not an author-controlled persistent history/buffer API. |
| Sampling | [samplerOptions](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/renderer/OpenGL.zig) uses linear filtering and clamp-to-edge. `iResolution` measures physical output pixels. Pixel offsets must account for scale and boundaries. |
| Loading | [shadertoy.zig](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/renderer/shadertoy.zig) prepends the GLSL prefix, compiles through glslang to Vulkan 1.2 / SPIR-V 1.5, then uses SPIRV-Cross to emit target GLSL for Linux. Independent front-end compilation does not verify the whole path. |
| Config and redraw | [Config.zig](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/config/Config.zig) provides `custom-shader`, `custom-shader-animation=false`, CLI-only `config-default-files=false`, and `gtk-single-instance=false`. The latter two isolate a CLI experiment. Animation off still permits redraws for output, cursor blink, selection and other terminal updates. Shader compile errors appear in renderer logs, not just config validation. |
| Cursor and selection | Cursor visibility/style are **int**, despite the online reference documenting some as vec4. The source is authoritative. [State/frame uniform updates](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/renderer/generic.zig) populate cursor geometry/visibility and selection colors, but not a selection mask. Check visibility before using stored cursor coordinates. |
| Glyph-aware extension point | The existing [text fragment shader](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/src/renderer/shaders/glsl/cell_text.f.glsl) has atlas coverage and foreground/background inputs before composition. This is a plausible investigation point for real per-glyph materials; it is not currently accessible through a config-only custom shader. |

## Feasibility inference before prototype work

**Credible narrow experiment:** with opaque near-black background and a known
amber foreground, project each pixel onto the foreground/background color line
to estimate coverage. Sample neighbors for a narrow edge, directional highlight
and a small glow. Keep the original antialiasing footprint, avoid animation,
and leave colors outside the heuristic mostly unchanged. This can test whether
there is useful visual depth at 12 pt without writing a terminal emulator.

**Fundamental limitation:** compositing discards identity. An amber pixel can
come from a glyph, an ANSI/truecolor background, selection, cursor or image.
Even a perfect color classifier cannot distinguish identical input pixels with
different origins. A uniform amber area also lacks glyph-interior geometry.
Changing background colors invalidates the inferred coverage. Simple image
gradients cannot reliably recover a per-glyph bevel or surface normal, and
thin strokes may have no space for distinct core/contour layers.

These are source-based inferences, not observations of the unrun shader. They
rule out claiming generally correct text-only material effects from this API;
they do **not** prove that no visually useful restricted effect is possible.
The fixture deliberately includes matching amber cell backgrounds and solid
block characters to expose the ambiguity rather than hiding it. ANSI colors
close to amber can still be modified. Selection and arbitrary TUI backgrounds
are especially important falsification cases.

Window decorations, GTK tabs/buttons/icons and KDE frame accents are outside
the terminal texture. A custom shader cannot deliver the complete artwork's
illuminated chrome. This remains a separate unsolved requirement.

## Isolated experiment and options

[Prototype sources and full runbook](../../prototypes/ghostty-molten-amber/README.md)
include a shared baseline config, a single 17-sample static shader, a launcher
which adds that shader only in prototype mode, and static/scroll/curses workloads.
It starts with standard Noto Sans Mono at 12 pt and an opaque background.
Cursor and selection use contrasting controls for visibility, not the approved
amber treatment. There is no font project, new application shell, four-theme
system or renderer modification.

| Option | Assessment and bounded next step |
| --- | --- |
| Config-only post-process | Smallest experiment; no Ghostty maintenance fork. Execute this trial after binary approval. Reject it if the distinctive material is absent or normal terminal use produces objectionable artifacts. Do not accept simple amber recoloring as equivalent. |
| Glyph-aware Ghostty change | Potential **CONDITIONAL GO only after owner authorization and a separate demonstrated result**. A bounded study could modify only grayscale text shading and emit a dedicated mask for glow, leaving VT/input untouched. Scope includes per-instance glyph coordinates, cursor/selection exclusion and render-target plumbing; colored glyphs need an explicit bypass. Assess all affected render paths and atlas/background interactions. This is substantially more work than the single shader, with recurring rebase, performance and regression costs each upstream renderer release. No implementation or effort estimate has been validated. |
| Broad renderer/emulator rewrite | Outside this issue and unauthorized. The current evidence does not justify starting one. |

Ghostty itself is [MIT licensed](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/LICENSE).
A distributed fork must retain its notice. That license is not a blanket
statement about every bundled dependency or font: the exact build's notices
must be reviewed before distribution. This spike distributes neither Ghostty
nor fonts, and the external prefix is not vendored. The pinned
[packaging guide](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/PACKAGING.md)
describes the release tarball/build process. Its examples and the live build
documentation can lag: this release's
[build.zig.zon](https://github.com/ghostty-org/ghostty/blob/83fb95624b0439ee660c2679b43b38c8b63769cd/build.zig.zon)
declares minimum Zig **0.17.0**. No claim is made that an arbitrary older Zig
builds 1.4.0.

The [official binary guide](https://ghostty.org/docs/install/binary#fedora)
lists Fedora COPR/other community options, not a Ghostty-produced Linux binary.
Do not enable a repository automatically. Owner choices are to supply an
approved binary, approve a specific community package, or approve a local
build and its missing dependencies. Package provenance/version must be
recorded with the native run.

## 4K cost model — estimates, not benchmark results

The full shader path samples 17 texels per output pixel. At 1920×1080 that is
35,251,200 texture-sample instructions per frame; at 3840×2160 it is
141,004,800, four times as many. At a hypothetical 60 redraws/s, 4K means
8.46 billion sample instructions/s. Cache reuse and hardware execution make
these counts unsuitable for predicting actual bandwidth or frame time.

Two RGBA8 intermediates at 4K occupy about 63.3 MiB per frame slot. With all
three slots allocated as in the source, these intermediates alone are about
189.8 MiB, excluding presentation targets, atlases, GTK/compositor resources
and driver allocation overhead. This is a source-based storage estimate, not
measured GPU memory. DMABUF import failure and CPU-copy fallback are additional
risks to measure on NVIDIA. Halo radius is fixed in physical pixels; display
scaling can alter apparent glow width and thin-stroke quality.

| Workload | 1080p baseline / prototype | 4K baseline / prototype |
| --- | --- | --- |
| Idle redraw, CPU/GPU/power | Not measured / not measured | Not measured / not measured |
| Typing latency/responsiveness | Not measured / not measured | Not measured / not measured |
| Scroll and resize | Not measured / not measured | Not measured / not measured |

The runbook defines repeatable paired workloads and flags whole-GPU telemetry
as confounded by the compositor and other apps. Native 4K display validation
cannot use the current active monitor modes; enabling/changing a display mode
also needs owner approval. An offscreen/virtual 4K run would be a separate,
explicitly labeled measurement, not proof of native 4K behavior.

## Visual gap table and evidence

Molten references are [the numbered amber concept](../../ChatGPT%20Image%20Nov%201,%202026,%2004_25_52%20PM-1.png)
and [Molten Fedora Terminal Glow](../../Molten%20Fedora%20Terminal%20Glow.png).
The other numbered images are Emerald, Arctic Cyan and Neon Magenta and are
preserved, without implementation in this spike. The reference images were
inspected directly and all five original hashes are recorded in
[artwork.sha256](evidence/artwork.sha256). Text printed *inside* the artwork,
including its claimed 4K resolution, is not machine/environment evidence.

| Approved target | Actual baseline / prototype evidence | Unresolved gap |
| --- | --- | --- |
| Metallic cores and thin luminous glyph contours | No screenshots captured | Per-character metallic appearance at ordinary size unverified; inferred mask has semantic ambiguity. |
| Controlled glow and crisp small text | No screenshots captured | Halo quality, clipping, stroke detail and scale behavior unverified. |
| ANSI/truecolor, Unicode, selection, cursors, TUI usability | Workload generators prepared; no Ghostty execution | Color contamination, selection rectangles, cursor artifacts, font fallback and layout all unverified. |
| Rectangular native geometry, illuminated controls and borders | No running-window screenshot | Normal native decorations requested; shader does not style chrome. |
| Acceptable 1080p and native 4K performance | No renderer benchmarks | All CPU/GPU, redraw and responsiveness criteria open. |

There is no honest reference/baseline/prototype screenshot comparison yet.
No generated, edited or synthetic render is supplied as implementation evidence.

## Checks executed and owner decision

[Validation record](evidence/validation.txt): Bash syntax passed; **7 launcher
tests passed**; the shader plus the exact pinned Ghostty prefix compiled to
10,412 bytes of SPIR-V using already-installed libshaderc 2026.1, and an invalid
GLSL negative control was rejected. Static and short scroll workloads generated
output. A 100×24 pseudo-terminal smoke run verified that the curses workload
entered and left its alternate screen and exited successfully. These are
source/harness checks, not rendered-terminal tests. The real launcher reported
the missing Ghostty binary and exited 127. No shader visual/driver test ran.

**Issue #1 remains incomplete.** The draft owner-review PR preserves the
research and testable experiment while native execution is blocked. Before
choosing a foundation, obtain the approved binary, execute the runbook, retain
unedited screenshots and paired measurements, then ask the owner to evaluate
GO / CONDITIONAL GO / NO-GO. Do not proceed to another phase or issue on the
basis of this report.
