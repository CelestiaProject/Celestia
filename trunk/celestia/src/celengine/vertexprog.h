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

namespace vp
{
    bool init();
    void enable();
    void disable();
    void use(unsigned int);

    void parameter(unsigned int, const Vec3f&);
    void parameter(unsigned int, const Point3f&);
    void parameter(unsigned int, const Color&);
    void parameter(unsigned int, float, float, float, float);

    extern unsigned int specular;
    extern unsigned int diffuse;
    extern unsigned int diffuseHaze;
    extern unsigned int diffuseBump;
    extern unsigned int diffuseBumpHaze;
    extern unsigned int everything;
    extern unsigned int shadowTexture;
    extern unsigned int diffuseTexOffset;
    extern unsigned int ringIllum;
};

#endif // _VERTEXPROG_H_
