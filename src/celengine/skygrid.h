// skygrid.h
//
// Celestial longitude/latitude grids.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SKYGRID_H_
#define _CELENGINE_SKYGRID_H_

#include <string>
#include <celutil/color.h>
#include <celmath/quaternion.h>

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
    
    SkyGrid();
    ~SkyGrid();

    void render(Renderer& renderer,
                const Observer& observer,
                int windowWidth,
                int windowHeight);

    Quatd orientation() const
    {
        return m_orientation;
    }

    void setOrientation(const Quatd& orientation)
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

private:
    std::string latitudeLabel(int latitude, int latitudeStep) const;
    std::string longitudeLabel(int longitude, int longitudeStep) const;
    int parallelSpacing(double idealSpacing) const;
    int meridianSpacing(double idealSpacing) const;

private:
    Quatd m_orientation;
    Color m_lineColor;
    Color m_labelColor;
    LongitudeUnits m_longitudeUnits;
    LongitudeDirection m_longitudeDirection;
};

#endif // _CELENGINE_PLANETGRID_H_

