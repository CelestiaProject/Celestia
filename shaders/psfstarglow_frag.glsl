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
in float v_alpha;
in float v_peakRadiance;
in float v_psfRadius;
in float v_p04;

void main(void)
{
    // Pixel offset from the centre of the point sprite (in screen pixels).
    // gl_PointSize was set to 2 * v_psfRadius * pointScale in the vertex
    // shader, so length(gl_PointCoord - 0.5) * 2 * v_psfRadius equals the
    // distance from the sprite centre in unscaled pixels (pointScale
    // cancels out).
    float px = length(gl_PointCoord.xy - vec2(0.5)) * 2.0 * v_psfRadius;

    // intensity = clamp(((peak^0.4 / px - a) * b)^2.5, 0, peak)
    // v_psfRadius now bounds the visible disc (where val*alpha >= 1 sRGB
    // code); the discard mostly fires for the corners of the point quad.
    if (px >= v_psfRadius)
        discard;

    // p04 comes through the varying; it isn't recoverable from
    // v_psfRadius anymore because v_psfRadius is the visibility-clipped
    // radius rather than p04/psfA.
    float base = v_p04 / px - psfA;
    float val = pow(base * psfB, 2.5);
    val = min(val, v_peakRadiance);

    // Clamp each channel of v_color * val to 1 BEFORE applying the
    // fade alpha so the bleached-white centre of a colored (e.g.
    // blue) star stays white through the fade, instead of reverting
    // to its underlying tint once v_alpha drops the per-channel value
    // back below saturation.
    vec3 clampedColor = min(vec3(1.0), v_color * val);
    fragColor = vec4(clampedColor * v_alpha, 1.0);
}
