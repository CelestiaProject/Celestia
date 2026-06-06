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

void main(void)
{
    // r = peak^0.4 / a is the PSF support radius in logical pixels; the
    // quad spans [-r, +r] so px = length(uv - 0.5) * 2 * r.
    float p04 = pow(max(v_peakRadiance, 1e-6), 0.4);
    float r   = (psfA > 0.0) ? (p04 / psfA) : 1.0;

    vec2  d  = (v_uv - vec2(0.5)) * 2.0 * r;
    float px = length(d);
    if (px <= 0.0 || px >= r)
        discard;

    float base = p04 / px - psfA;
    if (base <= 0.0)
        discard;

    float val = pow(base * psfB, 2.5);
    val = clamp(val, 0.0, v_peakRadiance);

    // Cap per-fragment output radiance at v_color.a (hue-preserving):
    // see psfstarglow_frag.glsl for the rationale — scaling by alpha
    // is what makes the C++-side alpha fade visible.
    float maxCh = max(max(v_color.r, v_color.g), v_color.b);
    val = min(val, v_color.a / max(maxCh, 1e-6));

    fragColor = vec4(v_color.rgb * val, 1.0);
}
