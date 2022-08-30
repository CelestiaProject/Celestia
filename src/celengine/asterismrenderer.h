// asterismrenderer.h
//
// Copyright (C) 2018-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>
#include <celengine/asterism.h>
#include <celrender/linerenderer.h>

class Color;
class Renderer;
struct Matrices;

class AsterismRenderer
{
public:
    AsterismRenderer(const Renderer &renderer, const AsterismList *asterisms);
    ~AsterismRenderer() = default;
    AsterismRenderer() = delete;
    AsterismRenderer(const AsterismRenderer&) = delete;
    AsterismRenderer(AsterismRenderer&&) = delete;
    AsterismRenderer& operator=(const AsterismRenderer&) = delete;
    AsterismRenderer& operator=(AsterismRenderer&&) = delete;

    void render(const Color &color, const Matrices &mvp);
    bool sameAsterisms(const AsterismList *asterisms) const;

private:
    bool prepare();

    celestia::engine::LineRenderer  m_lineRenderer;
    std::vector<int>                m_lineCount;
    const Renderer                 &m_renderer; 
    const AsterismList             *m_asterisms       { nullptr };
    int                             m_totalLineCount  { 0 };
    bool                            m_initialized     { false };
};
