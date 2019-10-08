// view.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

class Renderer;
class Observer;
class Color;

//namespace celestia
//{

class View
{
 public:
    enum Type
    {
        ViewWindow      = 1,
        HorizontalSplit = 2,
        VerticalSplit   = 3
    };

    View(Type, Renderer*, Observer*, float, float, float, float);
    View() = delete;
    ~View() = default;
    View(const View&) = delete;
    View(View&&) = delete;
    const View& operator=(const View&) = delete;
    View& operator=(View&&) = delete;

    void mapWindowToView(float, float, float&, float&) const;
    void walkTreeResize(View*, int);
    bool walkTreeResizeDelta(View*, float, bool);

    void switchTo(int gWidth, int gHeight);
    Observer* getObserver() const;
    bool isRootView() const;
    bool isSplittable(Type type) const;
    void split(Type type, Observer *o, float splitPos, View **split, View **view);
    void reset();
    static View* remove(View*);
    void drawBorder(int gWidth, int gHeight, const Color &color, float linewidth = 1.0f);

 public:
    Type      type;

    Renderer *renderer;
    Observer *observer;
    View     *parent          { nullptr };
    View     *child1          { nullptr };
    View     *child2          { nullptr };
    float     x;
    float     y;
    float     width;
    float     height;
    uint64_t  renderFlags     { 0 };
    int       labelMode       { 0 };
    float     zoom            { 1.0f };
    float     alternateZoom   { 1.0f };
};

//}
