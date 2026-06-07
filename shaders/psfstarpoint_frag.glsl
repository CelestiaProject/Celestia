// psfstarpoint_frag.glsl
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
// Outputs linear radiance; assumes additive blending and an sRGB framebuffer.

uniform float pointRadius;
uniform float pointScale;

in vec3  v_color;
in float v_peakRadiance;
in float v_pointSize;

void main(void)
{
    // Pixel offset from the centre of the point sprite (in screen pixels).
    vec2  d  = (gl_PointCoord.xy - vec2(0.5)) * v_pointSize;
    float px = length(d) / pointScale;

    // Linear cone of radius `pointRadius`, height min(peakRadiance, 1).
    float falloff = clamp(1.0 - px / pointRadius, 0.0, 1.0);
    float height  = min(v_peakRadiance, 1.0);
    vec3  radiance = v_color * (falloff * height);

    fragColor = vec4(radiance, 1.0);
}
