// psfstarglowlarge_frag.glsl
//
// Copyright (C) 2026-present, the Celestia Development Team
// Original rendering algorithm by Askaniy Anpilogov <aaskaniy@gmail.com>
// (https://github.com/Askaniy/CelestiaStarRenderer)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Batched billboard fragment shader for PSF glow stars.

uniform float psfA;
uniform float psfB;

in vec2  v_uv;
in vec3  v_color;
in float v_alpha;
in float v_peakRadiance;
in float v_psfRadius;
in float v_p04;
in float v_limbRadius;   // resolved-body limb radius in unscaled px, 0 if none
in float v_valLimb;      // PSF value at the limb radius (per-sprite constant)
in float v_alphaFade;    // pow(v_alpha, fadeExp), interior-fade crush (per sprite)

void main(void)
{
    // r is now the visibility-clipped radius (see vert shader).
    float r  = v_psfRadius;
    vec2  d  = (v_uv - vec2(0.5)) * 2.0 * r;
    float px = length(d);
    if (px >= r)
        discard;

    // p04 comes through the varying; it isn't recoverable from r
    // anymore because r is the visibility-clipped radius rather than
    // p04/psfA.
    float base = v_p04 / px - psfA;
    float s    = base * psfB;
    float val  = s * s * sqrt(s);   // s^2.5, cheaper than pow()
    val = min(val, v_peakRadiance);

    // Inside the geometric limb, crush the plateau (v_alphaFade) and floor at the
    // limb brightness (v_valLimb); both are per-sprite constants from the vertex shader.
    float brightness = val * v_alpha;
    if (v_limbRadius > 0.0 && px < v_limbRadius && v_peakRadiance > 1.0)
    {
        float t = 1.0 - px / v_limbRadius;
        t = sqrt(sqrt(sqrt(t)));   // t^(1/8), kPsfCoreSharpness == 8
        float valPlateau = mix(v_valLimb, v_peakRadiance, t);
        brightness = max(valPlateau * v_alphaFade, v_valLimb * v_alpha);
    }

    fragColor = vec4(v_color.rgb * brightness, 1.0);
}
