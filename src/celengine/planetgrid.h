// planetgrid.h
//
// Longitude/latitude grids for ellipsoidal bodies.
//
// Copyright (C) 2008-present, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string_view>

#include <Eigen/Core>

#include <celengine/referencemark.h>

class Body;
struct Matrices;
class Renderer;

namespace celestia::render
{
class ReferenceMarkRenderer;
}

class PlanetographicGrid : public ReferenceMark
{
public:

    /*! Three different longitude conventions are in use for
     *  solar system bodies:
     *      Westward is for prograde rotators (rotation pole above the ecliptic)
     *      Eastward is for retrograde rotators
     *      EastWest measures longitude both east and west, and is used only
     *         for the Earth and Moon (strictly because of convention.)
     */
    enum class LongitudeConvention
    {
        EastWest,
        Westward,
        Eastward,
    };

    /*! NorthReversed indicates that the north pole for this body is /not/
     *  the rotation north. It should be set for retrograde rotators in
     *  order to conform with IAU conventions.
     */
    enum class NorthDirection
    {
        NorthNormal,
        NorthReversed
    };

    PlanetographicGrid(const Body& _body);
    ~PlanetographicGrid() = default;

    void render(celestia::render::ReferenceMarkRenderer* renderer,
                const Eigen::Vector3f& pos,
                float discSizeInPixels,
                double tdb,
                const Matrices& m) const override;
    float boundingSphereRadius() const override;

    void setIAULongLatConvention();

protected:
    std::string_view defaultTag() const override;

private:
    const Body& body;

    float minLongitudeStep{ 10.0f };
    float minLatitudeStep{ 10.0f };

    LongitudeConvention longitudeConvention{ LongitudeConvention::Westward };
    NorthDirection northDirection{ NorthDirection::NorthNormal };
};
