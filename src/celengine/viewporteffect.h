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

#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>

class FramebufferObject;
class Renderer;
class CelestiaGLProgram;
class WarpMesh;

class ViewportEffect
{
public:
    virtual ~ViewportEffect() = default;

    virtual bool preprocess(Renderer*, FramebufferObject*);
    virtual bool prerender(Renderer*, FramebufferObject*);
    virtual bool render(Renderer*, FramebufferObject*, int width, int height) = 0;
    virtual bool distortXY(float& x, float& y);

private:
    GLint oldFboId;
};

class PassthroughViewportEffect : public ViewportEffect
{
public:
    PassthroughViewportEffect();
    ~PassthroughViewportEffect() override = default;

    bool render(Renderer*, FramebufferObject*, int width, int height) override;

private:
    celestia::gl::VertexObject vo{ celestia::util::NoCreateT{} };
    celestia::gl::Buffer bo{ celestia::util::NoCreateT{} };

    void initialize();

    bool initialized{ false };
};

class WarpMeshViewportEffect : public ViewportEffect
{
public:
    WarpMeshViewportEffect(WarpMesh *mesh);
    ~WarpMeshViewportEffect() override = default;

    bool prerender(Renderer*, FramebufferObject* fbo) override;
    bool render(Renderer*, FramebufferObject*, int width, int height) override;
    bool distortXY(float& x, float& y) override;

private:
    celestia::gl::VertexObject vo{ celestia::util::NoCreateT{} };
    celestia::gl::Buffer bo{ celestia::util::NoCreateT{} };

    WarpMesh *mesh;

    void initialize();

    bool initialized{ false };
};
