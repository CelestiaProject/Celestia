// rendcontext.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_RENDCONTEXT_H_
#define _CELENGINE_RENDCONTEXT_H_

#include "mesh.h"

class RenderContext
{
 public:
    RenderContext(const Mesh::Material*);
    RenderContext();

    virtual void makeCurrent(const Mesh::Material&) = 0;
    virtual void setVertexArrays(const Mesh::VertexDescription& desc,
                                 void* vertexData) = 0;
    virtual void drawGroup(const Mesh::PrimitiveGroup& group);

    void setMaterial(const Mesh::Material*);
    void lock() { locked = true; }
    void unlock() { locked = false; }
    bool isLocked() const { return locked; }

 private:
    const Mesh::Material* material;
    bool locked;
};


class FixedFunctionRenderContext : public RenderContext
{
 public:
    FixedFunctionRenderContext(const Mesh::Material*);
    FixedFunctionRenderContext();

    virtual void makeCurrent(const Mesh::Material&);
    virtual void setVertexArrays(const Mesh::VertexDescription& desc,
                                 void* vertexData);

 private:
    bool blendOn;
    bool specularOn;
};


class VP_FP_RenderContext : public RenderContext
{
 public:
    VP_FP_RenderContext();
    VP_FP_RenderContext(const Mesh::Material*);

    virtual void makeCurrent(const Mesh::Material&);
    virtual void setVertexArrays(const Mesh::VertexDescription& desc,
                                 void* vertexData);
};


class VP_Combiner_RenderContext : public RenderContext
{
    VP_Combiner_RenderContext();
    VP_Combiner_RenderContext(const Mesh::Material*);

    virtual void makeCurrent(const Mesh::Material&);
    virtual void setVertexArrays(const Mesh::VertexDescription& desc,
                                 void* vertexData);
};


#endif // _CELENGINE_RENDCONTEXT_H_

