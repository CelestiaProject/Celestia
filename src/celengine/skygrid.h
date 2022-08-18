// skygrid.h
//
// Celestial longitude/latitude grids.
//
// Copyright (C) 2008-present, the Celestia Development Team
// Initial version by Chris Laurel, <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <celrender/linerenderer.h>
#include <celutil/color.h>

class Renderer;
class Observer;


class SkyGrid
{
public:
    enum LongitudeUnits
    {
        LongitudeDegrees,
        LongitudeHours,
    };

    enum LongitudeDirection
    {
        IncreasingCounterclockwise,
        IncreasingClockwise,
    };

    void render(Renderer& renderer,
                const Observer& observer,
                int windowWidth,
                int windowHeight);

    Eigen::Quaterniond orientation() const
    {
        return m_orientation;
    }

    void setOrientation(const Eigen::Quaterniond& orientation)
    {
        m_orientation = orientation;
    }

    Color lineColor() const
    {
        return m_lineColor;
    }

    void setLineColor(Color lineColor)
    {
        m_lineColor = lineColor;
    }

    Color labelColor() const
    {
        return m_labelColor;
    }

    void setLabelColor(Color labelColor)
    {
        m_labelColor = labelColor;
    }

    //! Get the units of longitude (hours or degrees)
    LongitudeUnits longitudeUnits() const
    {
        return m_longitudeUnits;
    }

    //! Set whether longitude is measured in hours or degrees
    void setLongitudeUnits(LongitudeUnits longitudeUnits)
    {
        m_longitudeUnits = longitudeUnits;
    }

    //! Get the direction of increasing longitude
    LongitudeDirection longitudeDirection() const
    {
        return m_longitudeDirection;
    }

    //! Set the direction of increasing longitude (clockwise or counterclockwise)
    void setLongitudeDirection(LongitudeDirection longitudeDirection)
    {
        m_longitudeDirection = longitudeDirection;
    }

    static void deinit();

private:
    std::string latitudeLabel(int latitude, int latitudeStep) const;
    std::string longitudeLabel(int longitude, int longitudeStep) const;
    int parallelSpacing(double idealSpacing) const;
    int meridianSpacing(double idealSpacing) const;

    Eigen::Quaterniond m_orientation{ Eigen::Quaterniond::Identity() };
    Color m_lineColor{ Color::White };
    Color m_labelColor{ Color::White };
    LongitudeUnits m_longitudeUnits{ LongitudeHours };
    LongitudeDirection m_longitudeDirection{ IncreasingCounterclockwise };

    static celestia::engine::LineRenderer *g_gridRenderer;
    static celestia::engine::LineRenderer *g_crossRenderer;
};
