// marker.cpp
//
// Copyright (C) 2019, Celestia Development Team
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "marker.h"

#include "render.h"

namespace celestia::engine
{

UniversalCoord
Marker::position(double jd) const
{
    return m_object.getPosition(jd);
}

Selection
Marker::object() const noexcept
{
    return m_object;
}

int
Marker::priority() const noexcept
{
    return m_priority;
}

void
Marker::setPriority(int priority)
{
    m_priority = priority;
}

void
Marker::setRepresentation(const MarkerRepresentation& rep)
{
    m_representation = rep;
}

bool
Marker::occludable() const noexcept
{
    return m_occludable;
}

void
Marker::setOccludable(bool occludable)
{
    m_occludable = occludable;
}

MarkerSizing
Marker::sizing() const noexcept
{
    return m_sizing;
}

void
Marker::setSizing(MarkerSizing sizing)
{
    m_sizing = sizing;
}

void
Marker::render(Renderer& r, float size, const Matrices &m) const
{
    m_representation.render(r, m_sizing == MarkerSizing::DistanceBasedSize ? size : m_representation.size(), m);
}

void
MarkerRepresentation::setColor(Color color)
{
    m_color = color;
}

void
MarkerRepresentation::setSize(float size)
{
    m_size = size;
}

void
MarkerRepresentation::setLabel(std::string_view label)
{
    m_label = label;
}

/*! Render the marker symbol at the specified size. The size is
 *  the diameter of the marker in pixels.
 */
void
MarkerRepresentation::render(Renderer &r, float size, const Matrices &m) const
{
    r.renderMarker(m_symbol, size, m_color, m);
}

} // end namespace celestia::engine
