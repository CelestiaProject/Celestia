// psfstarglow_frag.glsl
//
// Copyright (C) 2026-present, the Celestia Development Team
// Original rendering algorithm by Askaniy Anpilogov <aaskaniy@gmail.com>
// (https://github.com/Askaniy/CelestiaStarRenderer)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Point Spread Function (PSF) star renderer - glow (eye-PSF) pass.
// Approximation of Greg Spencer et al. (1995) photopic PSF.
// Outputs linear radiance; assumes additive blending and an sRGB framebuffer.

uniform float pointRadius;
uniform float psfA;
uniform float psfB;
uniform float pointScale;

in vec3  v_color;
in float v_alpha;
in float v_peakRadiance;
in float v_psfRadius;
in float v_p04;
in float v_limbRadius;   // resolved-body limb radius in unscaled px, 0 if none
in float v_fadeExp;      // interior-fade crush exponent (per sprite)

void main(void)
{
    // Pixel offset from the centre of the point sprite (in screen pixels).
    // gl_PointSize was set to 2 * v_psfRadius * pointScale in the vertex
    // shader, so length(gl_PointCoord - 0.5) * 2 * v_psfRadius equals the
    // distance from the sprite centre in unscaled pixels (pointScale
    // cancels out).
    float px = length(gl_PointCoord.xy - vec2(0.5)) * 2.0 * v_psfRadius;

    // intensity = clamp(((peak^0.4 / px - a) * b)^2.5, 0, peak)
    // v_psfRadius now bounds the visible disc (where val*alpha >= 1 sRGB
    // code); the discard mostly fires for the corners of the point quad.
    if (px >= v_psfRadius)
        discard;

    // p04 comes through the varying; it isn't recoverable from
    // v_psfRadius anymore because v_psfRadius is the visibility-clipped
    // radius rather than p04/psfA.
    float base = v_p04 / px - psfA;
    float val = pow(base * psfB, 2.5);
    val = min(val, v_peakRadiance);

    // Fade the glow toward the resolving disc.  Outside the geometric limb the
    // halo fades linearly with v_alpha.  Inside the limb reshape the interior
    // into a plateau (kPsfCoreSharpness) so it reads as a flat-topped face, then
    // crush it and floor at the limb brightness (valLimb * v_alpha).  The crush
    // exponent v_fadeExp = 1 + log2(peak/valLimb) is evaluated per sprite so the
    // disc core meets the limb floor at v_alpha == 0.5 while preserving the full
    // plateau at v_alpha == 1.  Point sources (v_limbRadius == 0) keep the plain
    // linear fade.
    const float kPsfCoreSharpness = 8.0;
    float brightness = val * v_alpha;
    if (v_limbRadius > 0.0 && px < v_limbRadius && v_peakRadiance > 1.0)
    {
        float baseL   = v_p04 / v_limbRadius - psfA;
        float valLimb = min(pow(max(baseL, 0.0) * psfB, 2.5), v_peakRadiance);
        float t = 1.0 - px / v_limbRadius;
        t = pow(t, 1.0 / kPsfCoreSharpness);
        float valPlateau = mix(valLimb, v_peakRadiance, t);
        brightness = max(valPlateau * pow(v_alpha, v_fadeExp),
                         valLimb * v_alpha);
    }

    fragColor = vec4(v_color * brightness, 1.0);
}
