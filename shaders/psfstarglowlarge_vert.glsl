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

// Billboard fallback for PSF glow stars whose gl_PointSize would exceed
// the driver's GL_ALIASED_POINT_SIZE_RANGE upper bound.

layout(location = 0) in vec2 in_Position;   // quad corner offset in [-0.5, 0.5]
layout(location = 2) in vec2 in_TexCoord0;  // quad UV in [0, 1]

uniform vec3  center;         // star world position
uniform float pointWidth;     // 2 * sizePhys / windowWidth   (clip-space full extent)
uniform float pointHeight;    // 2 * sizePhys / windowHeight  (clip-space full extent)

out vec2 v_uv;

void main(void)
{
    v_uv = in_TexCoord0;
    set_vp(vec4(center, 1.0));
    vec2 transformed = vec2(in_Position.x * pointWidth, in_Position.y * pointHeight);
    gl_Position.xy += transformed * gl_Position.w;
}
