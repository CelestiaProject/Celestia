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
uniform float psfMinVisRad; // smallest linear radiance value that the
                            // framebuffer can still encode as non-zero
                            // (1/255 for linear, 1/(255*12.92) for sRGB)
uniform float pointScale;  // DPI scale

out vec3  v_color;
out float v_alpha;
out float v_peakRadiance;
out float v_psfRadius;  // PSF cutoff radius in px (>0)
out float v_p04;        // pow(peakRadiance, 0.4); needed because v_psfRadius
                        // is now the shrunken visibility radius, not p04/a.

void main(void)
{
    v_color = in_Color.rgb;
    v_alpha = in_Color.a;
    v_peakRadiance = in_Intensity;

    // Glow mode: PSF support radius depends on peak radiance.
    // psf_radius = peakRadiance^0.4 / a  (px, in unscaled coordinates)
    float p04   = pow(in_Intensity, 0.4);
    float rFull = p04 / psfA;

    // Tighten the rasterised radius to where the post-alpha brightest
    // channel falls below one framebuffer code (psfMinVisRad linear).
    // The shape of the PSF inside is unchanged; we just discard the
    // invisible rim up-front instead of running the fragment shader
    // to discard it.
    //
    //   val(px) = ((p04/px - a) * b)^2.5
    //   visible: val * alpha >= psfMinVisRad
    //   => px <= p04 / (a + (psfMinVisRad/alpha)^0.4 / b)
    float tVal  = psfMinVisRad / max(v_alpha, 1.0e-3);
    float rEff  = p04 / (psfA + pow(tVal, 0.4) / psfB);
    rEff        = min(rEff, rFull);

    v_psfRadius = rEff;
    v_p04       = p04;

    gl_PointSize = 2.0 * rEff * pointScale;
    set_vp(in_Position);
}
