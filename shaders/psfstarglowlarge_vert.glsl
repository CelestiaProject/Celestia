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
layout(location = 8) in vec3  in_Color;       // linear RGB
layout(location = 9) in float in_Intensity;   // peak radiance
layout(location = 13) in float in_Alpha;       // glow fade
layout(location = 14) in float in_LimbRadius;  // resolved-body limb radius (px)

uniform float psfA;             // optimization / pointRadius
uniform float psfB;             // 1 / (pi/pointRadius - a)
uniform float psfMinVisRad;     // smallest linear radiance the framebuffer
                                // encodes non-zero (see psfstarglow_vert).
uniform float psfPointScale;    // DPI scale
uniform vec2  psfViewportRcp;   // (1/width, 1/height)

out vec2  v_uv;
out vec3  v_color;
out float v_alpha;
out float v_peakRadiance;
out float v_psfRadius;
out float v_p04;
out float v_limbRadius;   // resolved-body limb radius in unscaled px, 0 if none
out float v_valLimb;      // PSF value at the limb radius (per-sprite constant)
out float v_alphaFade;    // pow(v_alpha, fadeExp), interior-fade crush (per sprite)

void main(void)
{
    v_uv           = in_TexCoord0;
    v_color        = in_Color;
    v_alpha        = in_Alpha;
    v_peakRadiance = in_Intensity;
    v_limbRadius   = in_LimbRadius;

    float p04   = pow(in_Intensity, 0.4);
    float rFull = p04 / psfA;

    // Match psfstarglow_vert.glsl: tighten the rasterised radius to
    // where the post-alpha brightest channel falls below one
    // framebuffer code (psfMinVisRad linear).
    float tVal  = psfMinVisRad / max(in_Alpha, 1.0e-3);
    float rEff  = p04 / (psfA + pow(tVal, 0.4) / psfB);
    rEff        = min(rEff, rFull);

    v_psfRadius = rEff;
    v_p04       = p04;

    // Interior-fade exponent, evaluated once per sprite so the crushed disc
    // core meets the limb floor at v_alpha == 0.5: N = 1 + log2(peak/valLimb).
    // Precompute valLimb and pow(alpha, fadeExp) here instead of per fragment.
    float fadeExp = 10.0;
    float valLimb = 0.0;
    if (in_LimbRadius > 0.0)
    {
        float baseL = p04 / in_LimbRadius - psfA;
        float s     = max(baseL, 0.0) * psfB;
        valLimb     = min(s * s * sqrt(s), in_Intensity);   // s^2.5
        if (valLimb > 0.0)
            fadeExp = 1.0 + log2(in_Intensity / valLimb);
    }
    v_valLimb   = valLimb;
    v_alphaFade = pow(v_alpha, fadeExp);

    set_vp(vec4(in_Normal, 1.0));

    float sizePhys = 2.0 * rEff * psfPointScale;
    vec2  extent   = sizePhys * psfViewportRcp;
    gl_Position.xy += in_Position * extent * gl_Position.w;
}
