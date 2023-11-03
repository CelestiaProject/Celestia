// view.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>

class Color;
class FramebufferObject;
class Observer;
class Overlay;

namespace celestia
{

class View
{
public:
    enum Type
    {
        ViewWindow      = 1,
        HorizontalSplit = 2,
        VerticalSplit   = 3
    };

    View(Type, Observer*, float, float, float, float);
    View() = delete;
    ~View();
    View(const View&) = delete;
    View(View&&) = delete;
    const View& operator=(const View&) = delete;
    View& operator=(View&&) = delete;

    void mapWindowToView(float, float, float&, float&) const;
    void walkTreeResize(View*, int);
    bool walkTreeResizeDelta(View*, float, bool);

    Observer* getObserver() const;
    bool isRootView() const;
    bool isSplittable(Type _type) const;
    void split(Type _type, Observer *o, float splitPos, View **split, View **view);
    void reset();
    static View* remove(View*);
    void drawBorder(Overlay*, int gWidth, int gHeight, const Color &color, float linewidth = 1.0f) const;
    void updateFBO(int gWidth, int gHeight);
    FramebufferObject *getFBO() const;

    Type           type;

    Observer      *observer;
    View          *parent          { nullptr };
    View          *child1          { nullptr };
    View          *child2          { nullptr };
    float          x;
    float          y;
    float          width;
    float          height;

private:
    std::unique_ptr<FramebufferObject> fbo;
};

}
