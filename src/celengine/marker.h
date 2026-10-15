// marker.h
//
// Copyright (C) 2003-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <celutil/color.h>
#include <celengine/selection.h>

class Renderer;
struct Matrices;

namespace celestia::engine
{
enum class MarkerSymbol : int
{
    Diamond    = 0,
    Triangle   = 1,
    Square     = 2,
    FilledSquare = 3,
    Plus       = 4,
    X          = 5,
    LeftArrow  = 6,
    RightArrow = 7,
    UpArrow    = 8,
    DownArrow  = 9,
    Circle     = 10,
    Disk       = 11,
    Crosshair  = 12,
};

class MarkerRepresentation
{
public:
    explicit MarkerRepresentation(MarkerSymbol symbol = MarkerSymbol::Diamond,
                                  float size = 10.0f,
                                  Color color = Color::White,
                                  std::string label = {}) :
        m_symbol(symbol),
        m_size(size),
        m_color(color),
        m_label(std::move(label))
    {
    }

    MarkerSymbol symbol() const noexcept { return m_symbol; }
    Color color() const noexcept { return m_color; }
    void setColor(Color);
    float size() const noexcept { return m_size; }
    void setSize(float size);
    std::string_view label() const { return m_label; }
    void setLabel(std::string_view);

    void render(Renderer &r, float size, const Matrices &m) const;

private:
    MarkerSymbol m_symbol;
    float m_size;
    Color m_color;
    std::string m_label;
};

/*! Options for marker sizing:
 *    When the sizing is set to ConstantSize, the marker size is interpreted
 *    as a fixed size in pixels.
 *    When the sizing is set to DistancedBasedSize, the marker size is
 *    in kilometers, and the size of the marker on screen is based on
 *    the size divided by the marker's distance from the observer.
 */
enum class MarkerSizing
{
    ConstantSize,
    DistanceBasedSize,
};

class Marker
{
public:
    Marker(const Selection& s) : m_object(s) {};
    ~Marker() = default;

    UniversalCoord position(double jd) const;
    Selection object() const noexcept;

    int priority() const noexcept;
    void setPriority(int);
    bool occludable() const noexcept;
    void setOccludable(bool);
    MarkerSizing sizing() const noexcept;
    void setSizing(MarkerSizing);

    const MarkerRepresentation& representation() const noexcept { return m_representation; }
    MarkerRepresentation& representation() noexcept { return m_representation; }
    void setRepresentation(const MarkerRepresentation& rep);

    void render(Renderer &r, float size, const Matrices &m) const;

private:
    Selection m_object;
    int m_priority{ 0 };
    MarkerRepresentation m_representation{ MarkerSymbol::Diamond };
    bool m_occludable { true };
    MarkerSizing m_sizing { MarkerSizing::ConstantSize };
};

using MarkerList = std::vector<Marker>;
} // end namespace celestia::engine
