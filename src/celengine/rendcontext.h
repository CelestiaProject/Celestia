// rendcontext.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_RENDCONTEXT_H_
#define _CELENGINE_RENDCONTEXT_H_

#include "shadermanager.h"
#include <celmodel/mesh.h>
#include <Eigen/Geometry>

class Renderer;

class RenderContext
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    RenderContext(const cmod::Material*);
    RenderContext(const Renderer*);
    virtual ~RenderContext() = default;

    virtual void makeCurrent(const cmod::Material&) = 0;
    virtual void setVertexArrays(const cmod::Mesh::VertexDescription& desc,
                                 const void* vertexData);
    virtual void drawGroup(const cmod::Mesh::PrimitiveGroup& group);

    const cmod::Material* getMaterial() const;
    void setMaterial(const cmod::Material*);
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

    void setPointScale(float);
    float getPointScale() const;

    void setCameraOrientation(const Eigen::Quaternionf& q);
    Eigen::Quaternionf getCameraOrientation() const;

 private:
    const cmod::Material* material{ nullptr };
    bool locked{ false };
    RenderPass renderPass{ PrimaryPass };
    float pointScale{ 1.0f };
    Eigen::Quaternionf cameraOrientation;  // required for drawing billboards

 protected:
    const Renderer* renderer { nullptr };
    bool usePointSize{ false };
    bool useNormals{ true };
    bool useColors{ false };
    bool useTexCoords{ true };
};



class GLSL_RenderContext : public RenderContext
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    GLSL_RenderContext(const Renderer* r, const LightingState& ls, float _objRadius, const Eigen::Quaternionf& orientation);
    GLSL_RenderContext(const Renderer* r, const LightingState& ls, const Eigen::Vector3f& _objScale, const Eigen::Quaternionf& orientation);
    ~GLSL_RenderContext() override;

    void makeCurrent(const cmod::Material&) override;
    void setLunarLambert(float);
    void setAtmosphere(const Atmosphere*);

 private:
     void initLightingEnvironment();
     void setLightingParameters(CelestiaGLProgram& prog, Color diffuseColor, Color specularColor);
     void setShadowParameters(CelestiaGLProgram& prog);

 private:
    const LightingState& lightingState;
    const Atmosphere* atmosphere{ nullptr };
    cmod::Material::BlendMode blendMode{ cmod::Material::InvalidBlend };
    float objRadius;
    Eigen::Vector3f objScale;
    Eigen::Quaternionf objOrientation;

    // extended material properties
    float lunarLambert{ 0.0f };

    ShaderProperties shaderProps;
};


class GLSLUnlit_RenderContext : public RenderContext
{
 public:
    GLSLUnlit_RenderContext(const Renderer* r, float _objRadius);
    ~GLSLUnlit_RenderContext() override;

    void makeCurrent(const cmod::Material&) override;

 private:
    void initLightingEnvironment();
    void setLightingParameters(CelestiaGLProgram& prog, Color diffuseColor, Color specularColor);

 private:
    cmod::Material::BlendMode blendMode;
    float objRadius;

    ShaderProperties shaderProps;
};


#endif // _CELENGINE_RENDCONTEXT_H_
