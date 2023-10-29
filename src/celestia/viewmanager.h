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

#include <list>

#include "view.h"

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
    double flashFrameStart() const;
    void flashFrameStart(double);
    bool isResizing() const;

    ViewBorderType checkViewBorder(const WindowMetrics&, float x, float y) const;

    bool pickView(Simulation*, const WindowMetrics&, float x, float y);
    void nextView(Simulation*);

    void tryStartResizing(const WindowMetrics&, float x, float y);
    bool resizeViews(const WindowMetrics&, float dx, float dy);
    bool stopResizing();

    ViewSplitResult splitView(Simulation*, View::Type, View*, float);
    void singleView(Simulation*, const View*);
    void setActiveView(Simulation*, const View*);
    bool deleteView(Simulation*, View*);

private:
    std::list<View*> m_views;
    std::list<View*>::iterator m_activeView;
    View* m_resizeSplit{ nullptr };
    double m_flashFrameStart{ 0.0 };
};

}
