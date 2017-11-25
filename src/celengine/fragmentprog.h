// fragmentprog.h
//
// Copyright (C) 2003 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_FRAGMENTPROG_H_
#define _CELENGINE_FRAGMENTPROG_H_

#include <celutil/color.h>
#include <celutil/util.h>
#include <GL/glew.h>
#include <Eigen/Core>

class FragmentProcessor;

namespace fp
{
    FragmentProcessor* initNV();
    FragmentProcessor* initARB();
    void enable();
    void disable();
    void use(unsigned int);

    void parameter(unsigned int, const Eigen::Vector3f&);
    void parameter(unsigned int, const Eigen::Vector4f&);
    void parameter(unsigned int, const Color&);
    void parameter(unsigned int, float, float, float, float);

    enum Parameter {
        SunDirection       = 0,
        EyePosition        = 1,
        DiffuseColor       = 2,
        SpecularColor      = 3,
        SpecularExponent   = 4,
        AmbientColor       = 5,
        HazeColor          = 6,
        TextureTranslation = 7,
        TexGen_S           = 8,
        TexGen_T           = 9,
        ShadowParams0      = 20,
        ShadowParams1      = 21,
    };

    extern unsigned int sphereShadowOnRings;
    extern unsigned int eclipseShadow1;
    extern unsigned int eclipseShadow2;
    extern unsigned int texDiffuse;
    extern unsigned int texDiffuseBump;
    extern unsigned int texSpecular;
    extern unsigned int texSpecularAlpha;
};


class FragmentProcessor
{
 public:
    FragmentProcessor();
    virtual ~FragmentProcessor();

    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual void use(unsigned int) = 0;
    virtual void parameter(fp::Parameter, const Eigen::Vector3f&);
    virtual void parameter(fp::Parameter, const Eigen::Vector4f&);
    virtual void parameter(fp::Parameter, const Color&);
    virtual void parameter(fp::Parameter, float, float, float, float) = 0;
    virtual void parameter(fp::Parameter, const float*) = 0;

 private:
    int dummy;
};

#endif // _CELENGINE_FRAGMENTPROG_H_
