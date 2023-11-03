// viewmanager.cpp
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

#include "viewmanager.h"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <utility>

#include <celengine/observer.h>
#include <celengine/simulation.h>
#include <celutil/color.h>
#include "windowmetrics.h"

namespace celestia
{

namespace
{

constexpr float borderSize = 2.0f;
constexpr double flashDuration = 0.5;

constexpr Color frameColor{ 0.5f, 0.5f, 0.5f, 1.0f };
constexpr Color activeFrameColor{ 0.5f, 0.5f, 1.0f, 1.0f };

inline std::pair<float, float>
metricsSizeFloat(const WindowMetrics& metrics)
{
    return std::make_pair(static_cast<float>(metrics.width), static_cast<float>(metrics.height));
}

bool
pointOutsideView(const View& view,
                 float mwidth, float mheight,
                 float x, float y)
{
    return x + borderSize < view.x * mwidth ||
        x - borderSize > (view.x + view.width) * mwidth ||
        (mheight - y) + borderSize < view.y * mheight ||
        (mheight - y) - borderSize > (view.y + view.height) * mheight;
}

std::tuple<float, float, float, float>
viewBoundaryTestValues(const View& view,
                       float mwidth, float mheight,
                       float x, float y)
{
    float vx = (x / mwidth - view.x) / view.width;
    float vy = ((1 - y / mheight) - view.y ) / view.height;
    float vxp = vx * view.width * mwidth;
    float vyp = vy * view.height * mheight;
    return std::make_tuple(vx, vy, vxp, vyp);
}

inline bool
testEdge(float vpara, float vperpp, float vperpDim, float mperpDim)
{
    return vpara >= 0.0f && vpara <= 1.0f &&
           (std::abs(vperpp) <= borderSize || std::abs(vperpp - vperpDim * mperpDim) <= borderSize);
}

} // end unnamed namespace

ViewManager::ViewManager(View* view)
{
    m_views.push_back(view);
    m_activeView = m_views.begin();
}

const std::list<View*>&
ViewManager::views() const
{
    return m_views;
}

const View*
ViewManager::activeView() const
{
    return *m_activeView;
}

ViewBorderType
ViewManager::checkViewBorder(const WindowMetrics& metrics, float x, float y) const
{
    auto [mwidth, mheight] = metricsSizeFloat(metrics);
    for (const auto v : m_views)
    {
        if (v->type != View::ViewWindow)
            continue;

        auto [vx, vy, vxp, vyp] = viewBoundaryTestValues(*v, mwidth, mheight, x, y);
        if (testEdge(vx, vyp, v->height, mheight))
            return ViewBorderType::SizeVertical;

        if (testEdge(vy, vxp, v->width, mwidth))
            return ViewBorderType::SizeHorizontal;
    }

    return ViewBorderType::None;
}

/// Makes the view under x, y the active view.
void
ViewManager::pickView(Simulation* sim,
                      const WindowMetrics& metrics,
                      float x, float y)
{
    // Clang does not support capturing structured bindings, so work with the pair here
    auto msize = metricsSizeFloat(metrics);
    if (!pointOutsideView(**m_activeView, msize.first, msize.second, x, y))
        return;

    m_activeView = std::find_if(m_views.begin(), m_views.end(),
                                [&msize, x, y](const View* view) {
                                    return view->type == View::ViewWindow &&
                                           !pointOutsideView(*view, msize.first, msize.second, x, y);
                                });

    // Make sure that we're left with a valid view
    if (m_activeView == m_views.end())
        m_activeView = m_views.begin();

    sim->setActiveObserver((*m_activeView)->observer);
    if (!m_showActiveViewFrame)
        m_startFlash = true;
}

void
ViewManager::nextView(Simulation* sim)
{
    do
    {
        ++m_activeView;
        if (m_activeView == m_views.end())
            m_activeView = m_views.begin();
    } while ((*m_activeView)->type != View::ViewWindow);
    sim->setActiveObserver((*m_activeView)->observer);
    if (!m_showActiveViewFrame)
        m_startFlash = true;
}

void
ViewManager::tryStartResizing(const WindowMetrics& metrics, float x, float y)
{
    View* v1 = nullptr;
    View* v2 = nullptr;
    auto [mwidth, mheight] = metricsSizeFloat(metrics);
    for (View* v : m_views)
    {
        if (v->type != View::ViewWindow)
            continue;

        auto [vx, vy, vxp, vyp] = viewBoundaryTestValues(*v, mwidth, mheight, x, y);
        if (testEdge(vx, vyp, v->height, mheight) || testEdge(vy, vxp, v->width, mwidth))
        {
            if (v1 == nullptr)
            {
                v1 = v;
            }
            else
            {
                v2 = v;
                break;
            }
        }
    }

    if (v2 == nullptr)
        return;

    // Look for common ancestor to v1 & v2 = split being dragged.
    View *p1 = v1;
    View *p2 = v2;
    while ((p1 = p1->parent) != nullptr )
    {
        p2 = v2;
        while (((p2 = p2->parent) != nullptr) && p1 != p2) ;
        if (p2 != nullptr)
            break;
    }

    if (p2 != nullptr)
        m_resizeSplit = p1;
}

bool
ViewManager::resizeViews(const WindowMetrics& metrics, float dx, float dy)
{
    if (m_resizeSplit == nullptr)
        return false;

    switch(m_resizeSplit->type)
    {
    case View::HorizontalSplit:
        if (float delta = dy / static_cast<float>(metrics.height);
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child1, delta, true) &&
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child2, delta, true))
        {
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child1, delta, false);
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child2, delta, false);
        }
        break;
    case View::VerticalSplit:
        if (float delta = dx / static_cast<float>(metrics.width);
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child1, delta, true) &&
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child2, delta, true))
        {
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child1, delta, false);
            m_resizeSplit->walkTreeResizeDelta(m_resizeSplit->child2, delta, false);
        }
        break;
    case View::ViewWindow:
        break;
    }

    return true;
}

bool
ViewManager::stopResizing()
{
    if (m_resizeSplit == nullptr)
        return false;

    m_resizeSplit = nullptr;
    return true;
}

ViewSplitResult
ViewManager::splitView(Simulation* sim, View::Type type, View* av, float splitPos)
{
    if (type == View::ViewWindow)
        return ViewSplitResult::Ignored;

    if (av == nullptr)
        av = *m_activeView;

    if (!av->isSplittable(type))
        return ViewSplitResult::NotSplittable;

    Observer* o = sim->duplicateActiveObserver();

    View* split;
    View* view;
    av->split(type, o, splitPos, &split, &view);
    m_views.push_back(split);
    m_views.push_back(view);

    return ViewSplitResult::Ok;
}

void
ViewManager::singleView(Simulation* sim, const View* av)
{
    if (av == nullptr)
        av = *m_activeView;

    auto start = m_views.begin();
    auto it = std::find(start, m_views.end(), av);
    if (it == m_views.end())
        return;

    if (it != start)
        std::iter_swap(start, it);

    ++start;
    for (auto delIter = start; delIter != m_views.end(); ++delIter)
    {
        sim->removeObserver((*delIter)->getObserver());
        delete (*delIter)->getObserver();
        delete (*delIter);
    }

    m_views.resize(1);

    m_activeView = m_views.begin();
    (*m_activeView)->reset();
    sim->setActiveObserver((*m_activeView)->observer);
}

void
ViewManager::setActiveView(Simulation* sim, const View* view)
{
    auto it = std::find(m_views.begin(), m_views.end(), view);
    if (it == m_views.end())
        return;

    m_activeView = it;
    sim->setActiveObserver((*m_activeView)->observer);
}

bool
ViewManager::deleteView(Simulation* sim, View* v)
{
    if (v == nullptr)
        v = *m_activeView;

    if (v->isRootView())
        return false;

    //Erase view and parent view from views
    m_views.erase(std::remove_if(m_views.begin(), m_views.end(),
                                 [v](const auto mv) { return mv == v || mv == v->parent; }),
                  m_views.end());

    sim->removeObserver(v->getObserver());
    delete(v->getObserver());
    auto sibling = View::remove(v);

    View* nextActiveView = sibling;
    while (nextActiveView->type != View::ViewWindow)
        nextActiveView = nextActiveView->child1;
    m_activeView = std::find(m_views.begin(), m_views.end(), nextActiveView);
    sim->setActiveObserver((*m_activeView)->observer);
    if (!m_showActiveViewFrame)
        m_startFlash = true;

    return true;
}

void
ViewManager::renderBorders(Overlay* overlay, const WindowMetrics& metrics, double currentTime) const
{
    if (m_views.size() < 2)
        return;

    // Render a thin border arround all views
    if (m_showViewFrames || m_resizeSplit)
    {
        for(const auto v : m_views)
        {
            if (v->type == View::ViewWindow)
                v->drawBorder(overlay, metrics.width, metrics.height, frameColor);
        }
    }

    // Render a very simple border around the active view
    const View* av = *m_activeView;

    if (m_showActiveViewFrame)
    {
        av->drawBorder(overlay, metrics.width, metrics.height, activeFrameColor, 2);
    }

    if (m_startFlash)
    {
        m_flashFrameStart = currentTime;
        m_startFlash = false;
    }

    if (currentTime < m_flashFrameStart + flashDuration)
    {
        auto alpha = static_cast<float>(1.0 - (currentTime - m_flashFrameStart) / flashDuration);
        av->drawBorder(overlay, metrics.width, metrics.height, {activeFrameColor, alpha}, 8);
    }
}

} // end namespace celestia
