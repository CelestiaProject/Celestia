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


static const unsigned int MaxLights = 8;

struct DirectionalLight
{
    Color color;
    float irradiance;
    Vec3f direction_eye;
    Vec3f direction_obj;

    // Required for eclipse shadows only--may be able to use
    // distance instead of position.
    Point3d position;
    float apparentSize;
};

struct EclipseShadow
{
    Point3f origin;
    Vec3f direction;
    float penumbraRadius;
    float umbraRadius;
};

struct LightingState
{
    LightingState() : nLights(0),
                      eyeDir_obj(0.0f, 0.0f, -1.0f),
                      eyePos_obj(0.0f, 0.0f, -1.0f)
    { shadows[0] = NULL; };

    unsigned int nLights;
    DirectionalLight lights[MaxLights];
    std::vector<EclipseShadow>* shadows[MaxLights];

    Vec3f eyeDir_obj;
    Point3f eyePos_obj;
};


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

    enum RenderPass
    {
        PrimaryPass,
        EmissivePass,
    };

    RenderPass getRenderPass() const { return renderPass; }
    void setRenderPass(RenderPass rp) { renderPass = rp; }
    
 private:
    const Mesh::Material* material;
    bool locked;
    RenderPass renderPass;
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
 public:
    VP_Combiner_RenderContext();
    VP_Combiner_RenderContext(const Mesh::Material*);

    virtual void makeCurrent(const Mesh::Material&);
    virtual void setVertexArrays(const Mesh::VertexDescription& desc,
                                 void* vertexData);
};


class GLSL_RenderContext : public RenderContext
{
 public:
    GLSL_RenderContext(const LightingState& ls);
    GLSL_RenderContext(const LightingState& ls,
		       const Mesh::Material*);
    
    virtual void makeCurrent(const Mesh::Material&);
    virtual void setVertexArrays(const Mesh::VertexDescription& desc,
				 void* vertexData);

 private:
    const LightingState& lightingState;
};

#endif // _CELENGINE_RENDCONTEXT_H_

