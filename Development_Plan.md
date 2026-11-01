# PHOSPHOR — Visual Feasibility & Implementation Plan

**Status:** Proposed — feasibility testing only  
**Platform:** Fedora Linux 44, KDE Plasma, Wayland  
**Initial GPU target:** NVIDIA RTX 3070 Ti, 8 GB  
**Project type:** Native Linux terminal visual enhancement  
**Primary objective:** Reproduce the approved PHOSPHOR concept designs as faithfully as technically possible.

## 1. Design contract

The existing four approved concept images are the visual specification.

**Presets:**
- Molten Amber
- Emerald
- Arctic Cyan
- Neon Magenta

Preserve their appearance, including:

- Bright, metallic-looking glyph surfaces with illuminated contours.
- Distinctive luminous typography and controlled soft glow.
- Dark, clean terminal background.
- Thin illuminated window accents.
- Clear, visually consistent icons and controls.
- Color presets that change the overall luminous appearance.
- Normal desktop window geometry, without artificial CRT curvature, monitor housings or decorative screen cutouts.

These images are artistic targets, not evidence of functioning terminal-rendering capabilities.

Changes to the approved designs require owner authorization.

## 2. Architectural principle

Do not develop a complete terminal emulator unless an initial technical investigation demonstrates that existing solutions cannot reasonably support the visual requirements.

Prefer extending a proven terminal renderer.

Keep visual appearance separate from command execution, shell integration and terminal protocol handling wherever possible.

## 3. Phase 0 — Technical reconnaissance

**No implementation yet.**

Investigate Ghostty's current version, licensing, Linux renderer, shader configuration and relevant source architecture.

Determine:

- Whether custom GLSL shaders work correctly on Fedora/KDE/Wayland.
- Whether a shader can reliably identify text pixels and produce illuminated contours.
- Whether metallic-looking glyph interiors can be achieved using only a post-processing shader.
- Whether precise effects would require changes to glyph rendering itself.
- Whether native tabs, window decorations and controls can be styled sufficiently.
- Whether effects can be selectively applied without destroying terminal background and ANSI colors.
- Whether the intended architecture supports future maintenance and distribution.

Record technical limitations and clearly distinguish documented capabilities from unverified possibilities.

**Deliverable:** Short feasibility assessment identifying Ghostty's promising features, architectural risks and a minimal implementation approach.

## 4. Phase 1 — Minimal Ghostty proof of concept

This is the project's first development milestone.

Develop a disposable, isolated prototype using Ghostty's documented shader support.

Target only **Molten Amber** initially.

### Required visual experiments

**Experiment A — Glyph illumination**

Render normal monospaced terminal text using:

- Amber foreground.
- Bright character cores.
- Thin illuminated outlines.
- Soft local glow.

Determine whether the effect is visually distinguishable from simple orange text.

**Experiment B — Metallic appearance**

Investigate whether highlight gradients, internal shading and edge lighting can make characters resemble the molten-metal typography in the reference.

Test using a standard monospaced font first.

If font-aware rendering is required, document that limitation instead of approximating the design with unrelated visual effects.

**Experiment C — Background preservation**

Verify that glowing text does not introduce:

- Unwanted halos around empty areas.
- Glowing rectangular text-cell boundaries.
- Visible artifacts around selection backgrounds.
- Excessive distortion of colored command output.

**Experiment D — Normal terminal operation**

Test:

- Typing and cursor visibility.
- Text selection and copy/paste.
- Scrolling and rapid command output.
- Window resizing.
- Unicode and box-drawing characters.
- Full-screen terminal applications.
- Shell prompts and fastfetch.

**Experiment E — Performance**

Evaluate:

- 1080p.
- 1440p.
- 4K.
- Idle terminal.
- Continuous scrolling and output.
- CPU/GPU usage and responsiveness.

Record baseline Ghostty performance without shaders, then compare the prototype.

Avoid continuous rendering when static output does not require it.

### Scope restrictions

- No permanent changes to Konsole.
- No system-wide default-terminal replacement.
- No custom application shell.
- No four-theme implementation.
- No extensive settings interface.
- No unrelated terminal features.

**Deliverable:** Working Molten Amber prototype, screenshots, measured performance, limitations and owner review.

## 5. Phase 1 acceptance gate

The Chief visually compares the running terminal against the approved design.

The prototype must demonstrate:

1. The metallic/illuminated character effect is recognizably similar to the reference.
2. Text remains sharp and readable at ordinary terminal font sizes.
3. Glow and outlines are visible without excessive blur.
4. The terminal retains normal usability.
5. Performance is acceptable at 4K on the target machine.
6. No unacceptable graphical artifacts or major usability regressions occur.

**GO:** Continue with Ghostty if the visual requirements are achievable without disproportionate modifications.

**CONDITIONAL GO:** Continue only with a documented, bounded renderer modification if that offers a credible path to the reference.

**NO-GO:** Stop Ghostty implementation if fundamental rendering limitations prevent the approved appearance or require an unjustifiably large rewrite.

Do not substitute a visually inferior design merely to pass the gate.

## 6. Phase 2 — Alternative renderer feasibility

**Only if Phase 1 cannot meet the requirements.**

Investigate whether a custom rendering layer integrated with an existing terminal-emulation component can implement true per-glyph materials.

Potential capabilities:

- Glyph-mask and distance-field rendering.
- Separate metallic interior and glowing edge layers.
- Multi-pass blur and compositing.
- Per-theme color/material parameters.
- Precise cursor and selection handling.

Compare the engineering effort, licensing, ongoing maintenance and platform integration against the Ghostty approach.

Build only a small rendering demonstration before committing to a standalone terminal.

**Deliverable:** Technical alternative and GO / NO-GO recommendation.

## 7. Phase 3 — Product implementation

**Requires a successful feasibility gate and separate authorization.**

Implement the approved design on the selected foundation.

Priorities:

1. Production-quality typography and rendering.
2. All four approved color presets.
3. Native window and terminal usability.
4. Theme switching.
5. Persistent settings.
6. GPU performance and quality controls.
7. Packaging and distribution.
8. Automated tests and documentation.

Do not expand into additional application features during initial implementation.

## 8. Quality and safety requirements

- Preserve normal shell and terminal security boundaries.
- Avoid privileged execution or unnecessary background services.
- Never overwrite existing Konsole profiles or user configurations.
- Keep prototypes removable.
- Review third-party licenses before distributing modified software.
- Test accessibility and text clarity with glow enabled and disabled.
- Ensure shader failures cannot leave the user's normal terminal unusable.

## 9. Review and project governance

Work in isolated increments, with explicit owner approval at each major gate.

Maintain evidence and concise reports.

Do not begin full implementation merely because a shader compiles successfully.

**The visual result is the primary acceptance criterion, with performance, usability and maintainability as equally necessary constraints.**

## Central feasibility question

**Can an existing Linux terminal renderer faithfully produce PHOSPHOR's approved metallic, illuminated typography at normal font sizes and 4K resolution without requiring us to develop an entire terminal emulator?**

Answer this question before committing to the full product.
