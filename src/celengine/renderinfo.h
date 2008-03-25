// renderinfo.h
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

struct RenderInfo
{
    Color color;
    Texture* baseTex;
    Texture* bumpTex;
    Texture* nightTex;
    Texture* glossTex;
    Texture* overlayTex;
    Color hazeColor;
    Color specularColor;
    float specularPower;
    Vec3f sunDir_eye;
    Vec3f sunDir_obj;
    Vec3f eyeDir_obj;
    Point3f eyePos_obj;
    Color sunColor;
    Color ambientColor;
#ifdef USE_HDR
    float nightLightScale;
#endif
    float lunarLambert;
    Quatf orientation;
    float pixWidth;
    float pointScale;
    bool useTexEnvCombine;

    RenderInfo() :
#ifdef HDR_COMPRESS
                   color(0.5f, 0.5f, 0.5f),
#else
                   color(1.0f, 1.0f, 1.0f),
#endif
                   baseTex(NULL),
                   bumpTex(NULL),
                   nightTex(NULL),
                   glossTex(NULL),
                   overlayTex(NULL),
                   hazeColor(0.0f, 0.0f, 0.0f),
                   specularColor(0.0f, 0.0f, 0.0f),
                   specularPower(0.0f),
                   sunDir_eye(0.0f, 0.0f, 1.0f),
                   sunDir_obj(0.0f, 0.0f, 1.0f),
                   eyeDir_obj(0.0f, 0.0f, 1.0f),
                   eyePos_obj(0.0f, 0.0f, 0.0f),
                   sunColor(1.0f, 1.0f, 1.0f),
                   ambientColor(0.0f, 0.0f, 0.0f),
#ifdef USE_HDR
                   nightLightScale(1.0f),
#endif
                   lunarLambert(0.0f),
                   orientation(1.0f, 0.0f, 0.0f, 0.0f),
                   pixWidth(1.0f),
                   useTexEnvCombine(false)
    {};
};

extern LODSphereMesh* g_lodSphere;

