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
in float v_peakRadiance;
in float v_psfRadius;
in float v_pointSize;

void main(void)
{
    // Pixel offset from the centre of the point sprite (in screen pixels).
    vec2  d  = (gl_PointCoord.xy - vec2(0.5)) * v_pointSize;
    float px = length(d) / max(pointScale, 1e-6);

    // intensity = clamp(((peak^0.4 / px - a) * b)^2.5, 0, peak)
    if (px <= 0.0 || px >= v_psfRadius)
        discard;

    float base = pow(max(v_peakRadiance, 1e-6), 0.4) / px - psfA;
    if (base <= 0.0)
        discard;

    float val = pow(base * psfB, 2.5);
    val = clamp(val, 0.0, v_peakRadiance);

    fragColor = vec4(v_color * val, 1.0);
}
