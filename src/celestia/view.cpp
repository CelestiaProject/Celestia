// view.cpp
//
// Copyright (C) 2001-2019, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/rectangle.h>
#include <celengine/render.h>
#include <celengine/framebuffer.h>
#include <celutil/color.h>
#include "view.h"

using namespace std;

View::View(View::Type _type,
           Renderer *_renderer,
           Observer *_observer,
           float _x, float _y,
           float _width, float _height) :
    type(_type),
    renderer(_renderer),
    observer(_observer),
    x(_x),
    y(_y),
    width(_width),
    height(_height)
{
}

void View::mapWindowToView(float wx, float wy, float& vx, float& vy) const
{
    vx = (wx - x) / width;
    vy = (wy + (y + height - 1)) / height;
    vx = (vx - 0.5f) * (width / height);
    vy = 0.5f - vy;
}

void View::walkTreeResize(View* sibling, int sign)
{
    float ratio;
    switch (parent->type)
    {
    case View::HorizontalSplit:
        ratio = parent->height / (parent->height -  height);
        sibling->height *= ratio;
        if (sign == 1)
        {
            sibling->y = parent->y + (sibling->y - parent->y) * ratio;
        }
        else
        {
            sibling->y = parent->y + (sibling->y - (y + height)) * ratio;
        }
        break;

    case View::VerticalSplit:
        ratio = parent->width / (parent->width - width);
        sibling->width *= ratio;
        if (sign == 1)
        {
            sibling->x = parent->x + (sibling->x - parent->x) * ratio;
        }
        else
        {
            sibling->x = parent->x + (sibling->x - (x + width) ) * ratio;
        }
        break;
    case View::ViewWindow:
        break;
    }
    if (sibling->child1 != nullptr)
        walkTreeResize(sibling->child1, sign);
    if (sibling->child2 != nullptr)
        walkTreeResize(sibling->child2, sign);
}

bool View::walkTreeResizeDelta(View* v, float delta, bool check)
{
    View *p = v;
    int sign = -1;
    float ratio;
    double newSize;

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

    while ( p != child1 && p != child2 && (p = p->parent) != nullptr ) ;

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
        v->height = (float) newSize;
        if (sign == 1)
        {
            v->y = p->y + (v->y - p->y) * ratio;
        }
        else
        {
            v->y = p->y + delta + (v->y - p->y) * ratio;
        }
        break;

    case View::VerticalSplit:
        ratio = (p->width + sign * delta) / p->width;
        newSize = v->width * ratio;
        if (newSize <= .1)
            return false;
        if (check)
            return true;
        v->width = (float) newSize;
        if (sign == 1)
        {
            v->x = p->x + (v->x - p->x) * ratio;
        }
        else
        {
            v->x = p->x + delta + (v->x - p->x) * ratio;
        }
        break;
    case View::ViewWindow:
        break;
    }

    return true;
}

Observer* View::getObserver() const
{
    return observer;
}

bool View::isSplittable(Type type) const
{
    // If active view is too small, don't split it.
    return (type == View::HorizontalSplit && height >= 0.2f) ||
           (type == View::VerticalSplit && width >= 0.2f);
}

bool View::isRootView() const
{
    return parent == nullptr;
}

void View::split(Type type, Observer *o, float splitPos, View **split, View **view)
{
    float w1, h1, w2, h2, x1, y1;
    w1 = w2 = width;
    h1 = h2 = height;
    x1 = x;
    y1 = y;
    if (type == View::VerticalSplit)
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

    *split = new View(type,
                      renderer,
                      nullptr,
                      x, y, width, height);
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

    *view = new View(View::ViewWindow,
                     renderer,
                     o,
                     x1, y1, w2, h2);
    (*split)->child2 = *view;
    (*view)->parent = *split;
    (*view)->zoom = zoom;
}

View* View::remove(View* v)
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

void View::reset()
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

void View::drawBorder(int gWidth, int gHeight, const Color &color, float linewidth)
{
    Rect r(x * gWidth, y * gHeight, width * gWidth - 1, height * gHeight - 1);
    r.setColor(color);
    r.setType(Rect::Type::BorderOnly);
    r.setLineWidth(linewidth);
    renderer->drawRectangle(r, ShaderProperties::FisheyeOverrideModeDisabled, renderer->getOrthoProjectionMatrix());
}

void View::updateFBO(int gWidth, int gHeight)
{
    int newWidth = width * gWidth;
    int newHeight = height * gHeight;
    if (fbo && fbo.get()->width() == newWidth && fbo.get()->height() == newHeight)
        return;

    // recreate FBO when FBO not exisits or on size change
    fbo = unique_ptr<FramebufferObject>(new FramebufferObject(newWidth, newHeight,
                                                              FramebufferObject::ColorAttachment | FramebufferObject::DepthAttachment));
    if (!fbo->isValid())
    {
        DPRINTF(LOG_LEVEL_ERROR, "Error creating view FBO.\n");
        fbo = nullptr;
    }
}

FramebufferObject *View::getFBO() const
{
    return fbo.get();
}
