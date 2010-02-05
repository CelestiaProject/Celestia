// modelgeometry.h
//
// Copyright (C) 2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_MODEL_GEOMETRY_H_
#define _CELENGINE_MODEL_GEOMETRY_H_

#include "geometry.h"
#include <celmodel/model.h>
#include <celutil/resmanager.h>


class CelestiaTextureResource : public cmod::Material::TextureResource
{
public:
    CelestiaTextureResource(ResourceHandle textureHandle) :
        m_textureHandle(textureHandle)
    {
    }

    ResourceHandle textureHandle() const
    {
        return m_textureHandle;
    }

    std::string source() const;

private:
    ResourceHandle m_textureHandle;
};

class ModelOpenGLData;

class ModelGeometry : public Geometry
{
 public:
    ModelGeometry(cmod::Model* model);
    ~ModelGeometry();

    /*! Find the closest intersection between the ray and the
     *  model.  If the ray intersects the model, return true
     *  and set distance; otherwise return false and leave
     *  distance unmodified.
     */
    virtual bool pick(const Ray3d& r, double& distance) const;

    //! Render the model in the current OpenGL context
    virtual void render(RenderContext&, double t = 0.0);

    virtual bool usesTextureType(cmod::Material::TextureSemantic) const;
    virtual bool isOpaque() const;
    virtual bool isNormalized() const;

    void loadTextures();

 private:
    cmod::Model* m_model;
    bool m_vbInitialized;
    ModelOpenGLData* m_glData;
};

#endif // !_CELENGINE_MODEL_H_
