// star_vert.glsl
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
const float max_square_size = 512.0; // px
const float glow_scale = 0.123; // empirical constant, deg (not to change)
const float max_irradiation = pow((degree_per_px * max_square_size / glow_scale), 2.0) / (2.0 * 3.141592653);

varying vec3 v_color;
varying float max_theta;
varying float pointSize;

attribute vec4 in_Position;
attribute vec3 in_Color;
attribute float in_PointSize;

const float color_saturation_limit = 0.1; // the ratio of the minimum color component to the maximum

// Normalizes the color by its green value and corrects extreme saturation
vec3 green_normalization(vec3 color)
{
    // color /= max(color.r, max(color.g, color.b)); // we do this in XYZRGBConverter::convertUnnormalized()
    float delta = color_saturation_limit - min(color.r, min(color.g, color.b));

    if (delta > 0)
    {
        vec3 diff = vec3(1.0) - color;
        color += diff * diff * delta; // desaturating to the saturation limit
    }
    return color / color.g;
}

void main(void)
{
    vec3 color = green_normalization(in_Color);
    vec3 scaled_color = color * in_PointSize;

    if (all(lessThan(scaled_color, vec3(1.0))))
    {
        // Dim light source (9 pixels mode)
        max_theta = -1.0; // mode indicator
        pointSize = 3.0;
        v_color = scaled_color;
    }
    else
    {
        // Bright light source (glow mode)
        float irradiation = atan(in_PointSize / max_irradiation) * max_irradiation; // dimming
        max_theta = glow_scale * sqrt(irradiation);
        float half_sq = max_theta / degree_per_px;
        pointSize = 2.0 * half_sq - 1.0;
        v_color = color;
    }

    gl_PointSize = pointSize;
    set_vp(in_Position);
}
