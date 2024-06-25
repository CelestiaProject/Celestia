// view.cpp
//
// Copyright (C) 2001-2019, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "view.h"

#include <celengine/framebuffer.h>
#include <celengine/glsupport.h>
#include <celengine/overlay.h>
#include <celengine/rectangle.h>
#include <celengine/shadermanager.h>
#include <celutil/logger.h>

using celestia::util::GetLogger;

namespace celestia
{

View::View(View::Type type,
           Observer *observer,
           float x, float y,
           float width, float height) :
    type(type),
    observer(observer),
    x(x),
    y(y),
    width(width),
    height(height)
{
}


View::~View() = default;


void
View::mapWindowToView(float wx, float wy, float& vx, float& vy) const
{
    vx = (wx - x) / width;
    vy = (wy + (y + height - 1)) / height;
    vx = (vx - 0.5f) * (width / height);
    vy = 0.5f - vy;
}


void
View::walkTreeResize(View *sibling, int sign)
{
    float ratio;
    float diff;
    switch (parent->type)
    {
    case View::HorizontalSplit:
        ratio = parent->height / (parent->height -  height);
        sibling->height *= ratio;
        diff = sign == 1 ? parent->y : y + height;
        sibling->y = parent->y + (sibling->y - diff) * ratio;
        break;

    case View::VerticalSplit:
        ratio = parent->width / (parent->width - width);
        sibling->width *= ratio;
        diff = sign == 1 ? parent->x : x + width;
        sibling->x = parent->x + (sibling->x - diff) * ratio;
        break;
    default:
        break;
    }
    if (sibling->child1 != nullptr)
        walkTreeResize(sibling->child1, sign);
    if (sibling->child2 != nullptr)
        walkTreeResize(sibling->child2, sign);
}


bool
View::walkTreeResizeDelta(View *v, float delta, bool check)
{
    View *p = v;
    int sign = -1;
    float ratio, newSize;

    if (v->child1 != nullptr)
    {
        if (!walkTreeResizeDelta(v->child1, delta, check))
            return false;
    }

    if (v->child2 != nullptr)
    {
        if (!walkTreeResizeDelta(v->child2, delta, check))
            return false;
    }

    while (p != child1 && p != child2 && p->parent != nullptr)
        p = p->parent;

    if (p == child1)
        sign = 1;

    switch (type)
    {
    case View::HorizontalSplit:
        delta = -delta;
        ratio = (p->height  + sign * delta) / p->height;
        newSize = v->height * ratio;
        if (newSize <= .1)
            return false;
        if (check)
            return true;
        v->height = newSize;
        v->y = p->y + (v->y - p->y) * ratio;
        if (sign != 1)
            v->y += delta;
        break;

    case View::VerticalSplit:
        ratio = (p->width + sign * delta) / p->width;
        newSize = v->width * ratio;
        if (newSize <= .1)
            return false;
        if (check)
            return true;
        v->width = newSize;
        v->x = p->x + (v->x - p->x) * ratio;
        if (sign != 1)
            v->x += delta;
        break;
    case View::ViewWindow:
        break;
    }

    return true;
}


Observer*
View::getObserver() const
{
    return observer;
}


bool
View::isSplittable(Type _type) const
{
    // If active view is too small, don't split it.
    return (_type == View::HorizontalSplit && height >= 0.2f) ||
           (_type == View::VerticalSplit && width >= 0.2f);
}


bool
View::isRootView() const
{
    return parent == nullptr;
}


void
View::split(Type _type, Observer *o, float splitPos, View **split, View **view)
{
    float w1 = width;
    float h1 = height;
    float w2 = width;
    float h2 = height;
    float x1 = x;
    float y1 = y;
    if (_type == View::VerticalSplit)
    {
        w1 *= splitPos;
        w2 -= w1;
        x1 += w1;
    }
    else
    {
        h1 *= splitPos;
        h2 -= h1;
        y1 += h1;
    }

    *split = new View(_type, nullptr, x, y, width, height);
    (*split)->parent = parent;
    if (parent != nullptr)
    {
        if (parent->child1 == this)
            parent->child1 = *split;
        else
            parent->child2 = *split;
    }
    (*split)->child1 = this;

    width = w1;
    height = h1;
    parent = *split;

    *view = new View(View::ViewWindow, o, x1, y1, w2, h2);
    (*split)->child2 = *view;
    (*view)->parent = *split;
}


View*
View::remove(View* v)
{
    int sign;
    View *sibling;
    if (v->parent->child1 == v)
    {
        sibling = v->parent->child2;
        sign = -1;
    }
    else
    {
        sibling = v->parent->child1;
        sign = 1;
    }
    sibling->parent = v->parent->parent;
    if (v->parent->parent != nullptr)
    {
        if (v->parent->parent->child1 == v->parent)
            v->parent->parent->child1 = sibling;
        else
            v->parent->parent->child2 = sibling;
    }
    v->walkTreeResize(sibling, sign);
    delete(v->parent);
    delete(v);
    return sibling;
}


void
View::reset()
{
    x = 0.0f;
    y = 0.0f;
    width = 1.0f;
    height = 1.0f;
    parent = nullptr;
    child1 = nullptr;
    child2 = nullptr;
    fbo = nullptr;
}


void
View::drawBorder(Overlay* overlay, int gWidth, int gHeight, const Color &color, float linewidth) const
{
    celestia::Rect r(x * gWidth, y * gHeight, width * gWidth - 1, height * gHeight - 1);
    r.setColor(color);
    r.setType(celestia::Rect::Type::BorderOnly);
    r.setLineWidth(linewidth);
    overlay->drawRectangle(r);
}


void
View::updateFBO(int gWidth, int gHeight)
{
    auto newWidth = static_cast<GLuint>(width * gWidth);
    auto newHeight = static_cast<GLuint>(height * gHeight);
    if (fbo && fbo.get()->width() == newWidth && fbo.get()->height() == newHeight)
        return;

    // recreate FBO when FBO not exisits or on size change
    fbo = std::make_unique<FramebufferObject>(newWidth, newHeight,
                                              FramebufferObject::ColorAttachment | FramebufferObject::DepthAttachment);
    if (!fbo->isValid())
    {
        GetLogger()->error("Error creating view FBO.\n");
        fbo = nullptr;
    }
}


FramebufferObject*
View::getFBO() const
{
    return fbo.get();
}

} // end namespace celestia
