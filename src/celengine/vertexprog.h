// vertexprog.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _VERTEXPROG_H_
#define _VERTEXPROG_H_

#include <celmath/vecmath.h>
#include <celutil/color.h>
#include <celengine/gl.h>

class VertexProcessor;

namespace vp
{
    VertexProcessor* initNV();
    VertexProcessor* initARB();
    void enable();
    void disable();
    void use(unsigned int);

    void parameter(unsigned int, const Vec3f&);
    void parameter(unsigned int, const Point3f&);
    void parameter(unsigned int, const Color&);
    void parameter(unsigned int, float, float, float, float);

    enum Parameter {
        LightDirection0    = 0,
        EyePosition        = 1,
        DiffuseColor0      = 2,
        SpecularColor0     = 3,
        SpecularExponent   = 4,
        AmbientColor       = 5,
        HazeColor          = 6,
        TextureTranslation = 7,
        Constant0          = 8,
        TexGen_S           = 10,
        TexGen_T           = 11,
        TexGen_S2          = 12,
        TexGen_T2          = 13,
        TexGen_S3          = 14,
        TexGen_T3          = 15,
        TexGen_S4          = 16,
        TexGen_T4          = 17,
        LightDirection1    = 18,
        DiffuseColor1      = 19,
        SpecularColor1     = 20,
        Scale              = 21,
        InverseScale       = 22
    };

    extern unsigned int specular;
    extern unsigned int diffuse;
    extern unsigned int diffuseHaze;
    extern unsigned int diffuseBump;
    extern unsigned int diffuseBumpHaze;
    extern unsigned int everything;
    extern unsigned int shadowTexture;
    extern unsigned int multiShadow;
    extern unsigned int diffuseTexOffset;
    extern unsigned int ringIllum;
    extern unsigned int ringShadow;
    extern unsigned int cometTail;
    extern unsigned int nightLights;
    extern unsigned int glossMap;
    extern unsigned int perFragmentSpecular;
    extern unsigned int perFragmentSpecularAlpha;
    extern unsigned int diffuse_2light;
    extern unsigned int diffuseTexOffset_2light;
    extern unsigned int diffuseHaze_2light;
    extern unsigned int specular_2light;
    extern unsigned int nightLights_2light;
    extern unsigned int ellipticalGalaxy;
    extern unsigned int starDisc;
#ifdef HDR_COMPRESS
    extern unsigned int diffuseBumpHDR;
    extern unsigned int diffuseBumpHazeHDR;
    extern unsigned int nightLightsHDR;
    extern unsigned int nightLights_2lightHDR;
#endif
};


namespace arbvp
{
    void parameter(unsigned int, const Vec3f&);
    void parameter(unsigned int, const Point3f&);
    void parameter(unsigned int, const Color&);
    void parameter(unsigned int, float, float, float, float);
    void parameter(unsigned int, const float*);
};


class VertexProcessor
{
 public:
    VertexProcessor();
    virtual ~VertexProcessor();

    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual void use(unsigned int) = 0;
    virtual void parameter(vp::Parameter, const Vec3f&);
    virtual void parameter(vp::Parameter, const Point3f&);
    virtual void parameter(vp::Parameter, const Color&);
    virtual void parameter(vp::Parameter, float, float, float, float) = 0;
    virtual void parameter(vp::Parameter, const float*) = 0;

    virtual void enableAttribArray(unsigned int) = 0;
    virtual void disableAttribArray(unsigned int) = 0;
    virtual void attribArray(unsigned int index,
                             int size,
                             GLenum type,
                             unsigned int strude,
                             const void* pointer) = 0;

 private:
    int dummy;
};


#endif // _VERTEXPROG_H_
