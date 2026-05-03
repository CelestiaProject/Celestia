//
// viewporteffect.h
//
// Copyright © 2020 Celestia Development Team. All rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include "shadermanager.h"

class FramebufferObject;
class Renderer;
class CelestiaGLProgram;
class WarpMesh;

class ViewportEffect
{
public:
    virtual ~ViewportEffect() = default;

    virtual bool preprocess(Renderer*, FramebufferObject*);
    virtual bool prerender(Renderer*, FramebufferObject* fbo, FramebufferObject* dst);
    virtual bool render(Renderer*, FramebufferObject*, int width, int height) = 0;
    virtual bool distortXY(float& x, float& y);

    // Whether this effect needs its source FBO to use a floating-point
    // color buffer (GL_RGBA16F) instead of the default GL_RGBA8.
    virtual bool needsFloatSource() const { return false; }
};

class PassthroughViewportEffect : public ViewportEffect
{
public:
    explicit PassthroughViewportEffect(StaticShader shaderName = StaticShader::Passthrough,
                                       bool needsFloatSource = false);
    ~PassthroughViewportEffect() override = default;

    bool needsFloatSource() const override { return m_needsFloatSource; }
    bool render(Renderer*, FramebufferObject*, int width, int height) override;

private:
    StaticShader m_shaderName;
    bool m_needsFloatSource;

    celestia::gl::VertexObject vo{ celestia::util::NoCreateT{} };
    celestia::gl::Buffer bo{ celestia::util::NoCreateT{} };

    void initialize();

    bool initialized{ false };
};

class WarpMeshViewportEffect : public ViewportEffect //NOSONAR
{
public:
    explicit WarpMeshViewportEffect(std::unique_ptr<WarpMesh>&& mesh);
    ~WarpMeshViewportEffect() override;

    bool prerender(Renderer*, FramebufferObject* fbo, FramebufferObject* dst) override;
    bool render(Renderer*, FramebufferObject*, int width, int height) override;
    bool distortXY(float& x, float& y) override;

private:
    celestia::gl::VertexObject vo{ celestia::util::NoCreateT{} };
    celestia::gl::Buffer bo{ celestia::util::NoCreateT{} };

    std::unique_ptr<WarpMesh> mesh;

    void initialize();

    bool initialized{ false };
};
