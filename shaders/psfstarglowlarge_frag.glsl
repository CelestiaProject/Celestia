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
in vec4  v_color;
in float v_peakRadiance;
in float v_psfRadius;
in float v_p04;

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
    float val = pow(base * psfB, 2.5);
    val = min(val, v_peakRadiance);

    // Clamp each channel of v_color * val to 1 BEFORE applying the
    // fade alpha (v_color.a) so the bleached-white centre of a
    // coloured star stays white through the fade, instead of
    // reverting to its underlying tint once alpha drops the
    // per-channel value back below saturation.  See
    // psfstarglow_frag.glsl for the full rationale.
    vec3 clampedColor = min(vec3(1.0), v_color.rgb * val);
    fragColor = vec4(clampedColor * v_color.a, 1.0);
}
