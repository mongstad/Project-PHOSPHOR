// PHOSPHOR-001: a falsifiable composited-frame experiment, not a glyph material.
// Ghostty supplies the Shadertoy prefix; do not add #version or a main().
// This assumes config's opaque background, amber foreground and linear blending.
// Amber backgrounds/images WILL be confused with text. See the report.

const vec3 BG = vec3(6.0, 5.0, 4.0) / 255.0;
const vec3 FG = vec3(255.0, 158.0, 46.0) / 255.0;

vec3 toLinear(vec3 srgb) {
    return mix(srgb / 12.92, pow((srgb + 0.055) / 1.055, vec3(2.4)),
               greaterThan(srgb, vec3(0.04045)));
}

vec4 sourceAt(vec2 pixel) {
    // Explicit bounds stop glow sampling from wrapping to the opposite edge.
    vec2 bounded = clamp(pixel, vec2(0.5), iResolution.xy - vec2(0.5));
    return texture(iChannel0, bounded / iResolution.xy);
}

float amberCoverage(vec3 color) {
    // Recover coverage ONLY for the known foreground/background color line.
    // This is not a general glyph mask or a solution for arbitrary ANSI colors.
    vec3 background = toLinear(BG);
    vec3 foreground = toLinear(FG);
    vec3 direction = foreground - background;
    float coverage = clamp(dot(color - background, direction) / dot(direction, direction), 0.0, 1.0);
    float residual = length(color - mix(background, foreground, coverage));
    return coverage * (1.0 - smoothstep(0.025, 0.075, residual));
}

float maskAt(vec2 pixel) {
    return amberCoverage(sourceAt(pixel).rgb);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec4 original = sourceAt(fragCoord);
    float center = amberCoverage(original.rgb);

    // Keep the visible cursor and its immediate neighborhood untouched.
    // On Linux +Y is up; iCurrentCursor.xy is the cursor's left/top corner.
    if (iCursorVisible > 0) {
        vec2 lo = iCurrentCursor.xy - vec2(0.0, iCurrentCursor.w);
        vec2 hi = iCurrentCursor.xy + vec2(iCurrentCursor.z, 0.0);
        if (all(greaterThanEqual(fragCoord, lo - vec2(4.0))) &&
            all(lessThanEqual(fragCoord, hi + vec2(4.0)))) {
            fragColor = original;
            return;
        }
    }

    // 17 texture samples per fragment in the full path, no temporal feedback.
    float left = maskAt(fragCoord + vec2(-1.0, 0.0));
    float right = maskAt(fragCoord + vec2(1.0, 0.0));
    float up = maskAt(fragCoord + vec2(0.0, 1.0));
    float down = maskAt(fragCoord + vec2(0.0, -1.0));
    float diagonals = maskAt(fragCoord + vec2(-1.0, -1.0))
                    + maskAt(fragCoord + vec2(-1.0, 1.0))
                    + maskAt(fragCoord + vec2(1.0, -1.0))
                    + maskAt(fragCoord + vec2(1.0, 1.0));
    float nearGlow = (left + right + up + down + 0.5 * diagonals) / 6.0;
    float middleGlow = (maskAt(fragCoord + vec2(-2.0, 0.0))
                      + maskAt(fragCoord + vec2(2.0, 0.0))
                      + maskAt(fragCoord + vec2(0.0, -2.0))
                      + maskAt(fragCoord + vec2(0.0, 2.0))) / 4.0;
    float farGlow = (maskAt(fragCoord + vec2(-4.0, 0.0))
                   + maskAt(fragCoord + vec2(4.0, 0.0))
                   + maskAt(fragCoord + vec2(0.0, -4.0))
                   + maskAt(fragCoord + vec2(0.0, 4.0))) / 4.0;

    float innerEdge = max(center - min(min(left, right), min(up, down)), 0.0);
    float directionalHighlight = clamp(down - up + right - left, -1.0, 1.0);
    vec3 material = mix(vec3(0.72, 0.25, 0.025), vec3(1.0, 0.88, 0.54),
                        clamp(0.55 + 0.30 * directionalHighlight + 0.25 * innerEdge, 0.0, 1.0));
    // Retain the original antialiasing footprint and most of its sharp color.
    vec3 result = original.rgb + center * (toLinear(material) - toLinear(FG)) * 0.55;
    result += center * vec3(0.13, 0.11, 0.06);
    // Restrict halos to the configured dark background, retaining other colors.
    float backgroundOnly = 1.0 - smoothstep(0.015, 0.055, length(original.rgb - toLinear(BG)));
    result += backgroundOnly * vec3(1.0, 0.40, 0.06)
            * (0.12 * nearGlow + 0.055 * middleGlow + 0.02 * farGlow);
    fragColor = vec4(clamp(result, 0.0, 1.0), original.a);
}
