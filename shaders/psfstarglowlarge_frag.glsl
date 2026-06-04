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

// Billboard fallback for PSF glow stars - Spencer (1995) photopic PSF in
// fragment.  Identical math to psfstarglow_frag, but parameterised over the
// quad's UV instead of gl_PointCoord so we can exceed gl_PointSize limits.

uniform vec3  color;
uniform float peakRadiance;
uniform float psfA;
uniform float psfB;

in vec2 v_uv;

void main(void)
{
    // r = peak^0.4 / a is the PSF support radius in logical pixels.
    // The quad spans [-r, +r] in each axis, so px = length(uv - 0.5) * 2 * r.
    float p04 = pow(max(peakRadiance, 1e-6), 0.4);
    float r   = (psfA > 0.0) ? (p04 / psfA) : 1.0;

    vec2  d  = (v_uv - vec2(0.5)) * 2.0 * r;
    float px = length(d);
    if (px <= 0.0 || px >= r)
        discard;

    float base = p04 / px - psfA;
    if (base <= 0.0)
        discard;

    float val = pow(base * psfB, 2.5);
    val = clamp(val, 0.0, peakRadiance);

    fragColor = vec4(color * val, 1.0);
}
