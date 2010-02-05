// geometry.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_GEOMETRY_H_
#define _CELENGINE_GEOMETRY_H_

#include <celmodel/material.h>
#include <celmath/ray.h>

class RenderContext;


class Geometry
{
public:
    Geometry() {};
    virtual ~Geometry() {};

    //! Render the geometry in the specified OpenGL context
    virtual void render(RenderContext& rc, double t = 0.0) = 0;

    /*! Find the closest intersection between the ray and the
     *  model.  If the ray intersects the model, return true
     *  and set distance; otherwise return false and leave
     *  distance unmodified.
     */
    virtual bool pick(const Ray3d& r, double& distance) const = 0;
    
    virtual bool isOpaque() const = 0;

    virtual bool isNormalized() const
    {
        return true;
    }
    
    /*! Return true if the specified texture map type is used at
     *  all within this geometry object. This information is used
     *  to decide whether multiple rendering passes are required.
     */
    virtual bool usesTextureType(cmod::Material::TextureSemantic) const
    {
        return false;
    }

    /*! Load all textures used by the model. */
    virtual void loadTextures()
    {
    }
};

#endif // _CELENGINE_GEOMETRY_H_
