// observer.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Because of the vastness of interstellar space, floats and doubles aren't
// sufficient when we need to represent distances to millimeter accuracy.
// BigFix is a high precision (128 bit) fixed point type used to represent
// the position of an observer in space.  However, it's not practical to use
// high-precision numbers for the positions of everything.  To get around
// this problem, object positions are stored at two different scales--light
// years for stars, and kilometers for objects within a star system.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _OBSERVER_H_
#define _OBSERVER_H_

#include "bigfix.h"
#include "vecmath.h"
#include "univcoord.h"
#include "quaternion.h"


class Observer
{
public:
    Observer();

    // The getPosition method returns the observer's position in light
    // years.
    UniversalCoord getPosition() const;

    // getRelativePosition returns in units of kilometers the difference
    // between the position of the observer and a location specified in
    // light years.
    Point3d       getRelativePosition(Point3d&) const;

    Quatf         getOrientation() const;
    void          setOrientation(Quatf);
    Vec3d         getVelocity() const;
    void          setVelocity(Vec3d);

    void          setPosition(BigFix x, BigFix y, BigFix z);
    void          setPosition(UniversalCoord);
    void          setPosition(Point3d);

    void          update(double dt);
    
private:
    UniversalCoord position;
    Quatf          orientation;
    Vec3d          velocity;
};

#endif // _OBSERVER_H_
