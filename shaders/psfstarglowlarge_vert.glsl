// psfstarglowlarge_vert.glsl
//
// Copyright (C) 2026-present, the Celestia Development Team
// Original rendering algorithm by Askaniy Anpilogov <aaskaniy@gmail.com>
// (https://github.com/Askaniy/CelestiaStarRenderer)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Batched billboard fallback for PSF glow stars whose gl_PointSize would
// exceed the driver's GL_ALIASED_POINT_SIZE_RANGE.  Six vertices per star;
// per-star fields are replicated so the whole frame's glows draw at once.

layout(location = 0) in vec2  in_Position;    // quad corner in [-1, 1]
layout(location = 1) in vec3  in_Normal;      // star world position
layout(location = 2) in vec2  in_TexCoord0;   // quad UV in [0, 1]
layout(location = 8) in vec4  in_Color;       // linear RGB
layout(location = 9) in float in_Intensity;   // peak radiance

uniform float psfA;             // optimization / pointRadius
uniform float psfPointScale;    // DPI scale
uniform vec2  psfViewportRcp;   // (1/width, 1/height)

out vec2  v_uv;
out vec4  v_color;
out float v_peakRadiance;
out float v_psfRadius;

void main(void)
{
    v_uv           = in_TexCoord0;
    v_color        = in_Color;
    v_peakRadiance = in_Intensity;

    float r        = pow(in_Intensity, 0.4) / psfA;
    v_psfRadius    = r;

    set_vp(vec4(in_Normal, 1.0));

    float sizePhys = 2.0 * r * psfPointScale;
    vec2  extent   = sizePhys * psfViewportRcp;
    gl_Position.xy += in_Position * extent * gl_Position.w;
}
