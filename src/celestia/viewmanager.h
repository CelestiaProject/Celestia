// viewmanager.h
//
// Copyright (C) 2023, the Celestia Development Team
//
// Split out from celestiacore.h/celestiacore.cpp
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <limits>
#include <list>

#include "view.h"

class Overlay;
class Simulation;

namespace celestia
{

struct WindowMetrics;

enum class ViewBorderType
{
    None,
    SizeHorizontal,
    SizeVertical,
};

enum class ViewSplitResult
{
    Ignored,
    NotSplittable,
    Ok,
};

class ViewManager
{
public:
    explicit ViewManager(View*);

    const std::list<View*>& views() const;
    const View* activeView() const;

    ViewBorderType checkViewBorder(const WindowMetrics&, float x, float y) const;

    void pickView(Simulation*, const WindowMetrics&, float x, float y);
    void nextView(Simulation*);

    void tryStartResizing(const WindowMetrics&, float x, float y);
    bool resizeViews(const WindowMetrics&, float dx, float dy);
    bool stopResizing();

    ViewSplitResult splitView(Simulation*, View::Type, View*, float);
    void singleView(Simulation*, const View*);
    void setActiveView(Simulation*, const View*);
    bool deleteView(Simulation*, View*);

    void renderBorders(Overlay*, const WindowMetrics&, double) const;

    bool showViewFrames() const noexcept { return m_showViewFrames; }
    void showViewFrames(bool value) noexcept { m_showViewFrames = value; }
    bool showActiveViewFrame() const noexcept { return m_showActiveViewFrame; }
    void showActiveViewFrame(bool value) noexcept { m_showActiveViewFrame = value; }

private:
    std::list<View*> m_views;
    std::list<View*>::iterator m_activeView;
    View* m_resizeSplit{ nullptr };

    mutable double m_flashFrameStart{ -std::numeric_limits<double>::infinity() };
    mutable bool m_startFlash{ false };

    bool m_showViewFrames{ true };
    bool m_showActiveViewFrame{ false };
};

}
