// star_frag.glsl
//
// Copyright (C) 2023-present, the Celestia Development Team
// Original rendering algorithm by Askaniy Anpilogov <aaskaniy@gmail.com>
// Original shader implementation by Hleb Valoshka <375gnu@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

const float degree_per_px = 0.01; // higher value causes blinking due to optimizations in the psf_glow()
const float inv_max_offset = 2.0 / (3.0 * sqrt(2.0) * degree_per_px); // 1/px

varying vec3 v_color;
varying float max_theta;
varying float pointSize;

float psf_core(float offset)
{
    // Human eye's point source function from the research by Greg Spencer et al. (1995)
    // Optimized for the central part of the PSF (3x3 pixels square).
    return 1.0 - offset * inv_max_offset;
}

float psf_glow(float offset)
{
    // Human eye's point source function from the research by Greg Spencer et al. (1995)
    // Optimized for the outer part of the PSF. Designed with bounds by arctangent in mind.
    // Causes star blinking with degree_per_px > 0.01, large grid misses the center peak of brightness.
    float theta = offset * degree_per_px;
    if (theta == 0)
        return 100.0; // the center is always overexposed (zero division error)
    if (theta < max_theta)
    {
        float brackets = max_theta / theta - 1.0;
        return 0.0016 * brackets * brackets; // empirical
    }
    return 0.0; // after max_theta function starts to grow again
}

void main(void)
{
    // in fragment shader all points have virtual dimension 1x1, so gl_PointCoord has a value from [0; 1]
    float offset = length((gl_PointCoord.xy - vec2(0.5)) * pointSize);
    float point = (max_theta == -1.0) ? psf_core(offset) : psf_glow(offset);
    gl_FragColor = vec4(v_color * point, 1.0); // + vec4(0.1, 0.0, 0.0, 0.0); // red square for debugging
}
