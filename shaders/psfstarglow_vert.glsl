// psfstarglow_vert.glsl
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

layout(location = 0) in vec4 in_Position;
layout(location = 8) in vec4 in_Color;
layout(location = 9) in float in_Intensity;

uniform float pointRadius;
uniform float psfA;        // = optimization / pointRadius
uniform float psfB;        // = 1 / (pi/pointRadius - a)
uniform float pointScale;  // DPI scale

out vec3  v_color;
out float v_peakRadiance;
out float v_psfRadius;  // PSF cutoff radius in px (>0)
out float v_pointSize;

void main(void)
{
    v_color = in_Color.rgb * in_Color.a;
    v_peakRadiance = in_Intensity;

    // Glow mode: PSF support radius depends on peak radiance.
    // psf_radius = peakRadiance^0.4 / a  (px, in unscaled coordinates)
    float intensity = max(in_Intensity, 1e-6);
    float r = (psfA > 0.0) ? (pow(intensity, 0.4) / psfA) : 1.0;
    v_psfRadius = r;
    float size = max(2.0 * r * pointScale, 1.0);

    v_pointSize = size;
    gl_PointSize = size;
    set_vp(in_Position);
}
