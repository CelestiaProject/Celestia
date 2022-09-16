// rendcontext.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celmodel/material.h>
#include <celmodel/mesh.h>

#include "glsupport.h"
#include "shadermanager.h"


class Atmosphere;
class Color;
class LightingState;
class Renderer;

class RenderContext
{
 public:
    RenderContext(const cmod::Material*);
    RenderContext(Renderer*);
    virtual ~RenderContext() = default;

    virtual void makeCurrent(const cmod::Material&) = 0;
    virtual void setVertexArrays(const cmod::VertexDescription& desc,
                                 const cmod::VWord* vertexData);
    virtual void updateShader(const cmod::VertexDescription& desc, cmod::PrimitiveGroupType primType);
    virtual void drawGroup(const cmod::PrimitiveGroup& group);

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

 protected:
    Renderer* renderer { nullptr };
    bool usePointSize{ false };
    bool useStaticPointSize{ false };
    bool useNormals{ true };
    bool useColors{ false };
    bool useTexCoords{ true };

 private:
    const cmod::Material* material{ nullptr };
    bool locked{ false };
    RenderPass renderPass{ PrimaryPass };
    float pointScale{ 1.0f };
    Eigen::Quaternionf cameraOrientation;  // required for drawing billboards
};


class Shadow_RenderContext : public RenderContext
{
 public:
    Shadow_RenderContext(Renderer *r) :
        RenderContext(r)
    {
    }
    void makeCurrent(const cmod::Material&) override
    {
    }
};


class GLSL_RenderContext : public RenderContext
{
 public:
    GLSL_RenderContext(Renderer* r,
                       const LightingState& ls,
                       float _objRadius,
                       const Eigen::Quaternionf& orientation,
                       const Eigen::Matrix4f *_modelViewMatrix,
                       const Eigen::Matrix4f *_projectionMatrix);
    GLSL_RenderContext(Renderer* r,
                       const LightingState& ls,
                       const Eigen::Vector3f& _objScale,
                       const Eigen::Quaternionf& orientation,
                       const Eigen::Matrix4f *_modelViewMatrix,
                       const Eigen::Matrix4f *_projectionMatrix);
    ~GLSL_RenderContext() override;

    void makeCurrent(const cmod::Material&) override;
    void setLunarLambert(float);
    void setAtmosphere(const Atmosphere*);
    void setShadowMap(GLuint, GLuint, const Eigen::Matrix4f*);

 private:
    void initLightingEnvironment();
    void setLightingParameters(CelestiaGLProgram& prog, Color diffuseColor, Color specularColor);
    void setShadowParameters(CelestiaGLProgram& prog);

    const LightingState& lightingState;
    const Atmosphere* atmosphere{ nullptr };
    cmod::BlendMode blendMode{ cmod::BlendMode::InvalidBlend };
    float objRadius;
    Eigen::Vector3f objScale;
    Eigen::Quaternionf objOrientation;

    // extended material properties
    float lunarLambert{ 0.0f };

    ShaderProperties shaderProps;
    const Eigen::Matrix4f *modelViewMatrix;
    const Eigen::Matrix4f *projectionMatrix;
    const Eigen::Matrix4f *lightMatrix { nullptr };
    GLuint shadowMap { 0 };
    GLuint shadowMapWidth { 0 };
};


class GLSLUnlit_RenderContext : public RenderContext
{
 public:
    GLSLUnlit_RenderContext(Renderer* r,
                            float _objRadius,
                            const Eigen::Matrix4f *_modelViewMatrix,
                            const Eigen::Matrix4f *_projectionMatrix);
    ~GLSLUnlit_RenderContext() override;

    void makeCurrent(const cmod::Material&) override;

 private:
    void initLightingEnvironment();
    void setLightingParameters(CelestiaGLProgram& prog, Color diffuseColor, Color specularColor);

    cmod::BlendMode blendMode;
    float objRadius;

    ShaderProperties shaderProps;

    const Eigen::Matrix4f *modelViewMatrix;
    const Eigen::Matrix4f *projectionMatrix;
};
