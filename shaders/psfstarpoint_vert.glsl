// psfstarpoint_vert.glsl
//
// Copyright (C) 2026-present, the Celestia Development Team
// Original rendering algorithm by Askaniy Anpilogov <aaskaniy@gmail.com>
// (https://github.com/Askaniy/CelestiaStarRenderer)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Point Spread Function (PSF) star renderer - point (cone) pass.

layout(location = 0) in vec4 in_Position;
layout(location = 8) in vec4 in_Color;        // green-normalised, linear-space, 8-bit per channel
layout(location = 9) in float in_Intensity;   // peakRadiance = exposure * 3 * irradiance / (pi * pointRadius^2)

uniform float pointRadius; // pt, user setting (1..10)
uniform float pointScale;  // DPI scale

out vec3  v_color;
out float v_peakRadiance;
out float v_pointSize;

void main(void)
{
    v_color = in_Color.rgb;
    v_peakRadiance = in_Intensity;

    float size = 2.0 * pointRadius * pointScale;
    v_pointSize = size;
    gl_PointSize = size;
    set_vp(in_Position);
}
