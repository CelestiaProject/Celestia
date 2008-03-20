// planetgrid.h
//
// Longitude/latitude grids for ellipsoidal bodies.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_PLANETGRID_H_
#define _CELENGINE_PLANETGRID_H_

#include <celengine/referencemark.h>

class Body;


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
    enum LongitudeConvention
    {
        EastWest,
        Westward,
        Eastward,
    };

    /*! NorthReversed indicates that the north pole for this body is /not/
     *  the rotation north. It should be set for retrograde rotators in
     *  order to conform with IAU conventions.
     */
    enum NorthDirection
    {
        NorthNormal,
        NorthReversed
    };

    PlanetographicGrid(const Body& _body);
    ~PlanetographicGrid();

    void render(Renderer* renderer,
                const Point3f& pos,
                float discSizeInPixels,
                double tdb) const;
    float boundingSphereRadius() const;

    void setIAULongLatConvention();

private:
    static void InitializeGeometry();

private:
    const Body& body;

    float minLongitudeStep;
    float minLatitudeStep;

    LongitudeConvention longitudeConvention;
    NorthDirection northDirection;

    static unsigned int circleSubdivisions;
    static float* xyCircle;
    static float* xzCircle;
};

#endif // _CELENGINE_PLANETGRID_H_

