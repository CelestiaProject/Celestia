// univcoord.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Universal coordinate is a high-precision fixed point coordinate for
// locating objects in 3D space on scales ranging from millimeters to
// thousands of light years.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _UNIVCOORD_H_
#define _UNIVCOORD_H_

#include <celutil/bigfix.h>
#include <celmath/vecmath.h>

class UniversalCoord
{
 public:
    UniversalCoord();
    UniversalCoord(BigFix, BigFix, BigFix);
    UniversalCoord(double, double, double);
    UniversalCoord(const Point3d&);
    UniversalCoord(const Point3f&);

    operator Point3d() const;
    operator Point3f() const;

    friend Vec3d operator-(const UniversalCoord&, const UniversalCoord&);
    friend Vec3d operator-(const UniversalCoord&, const Point3d&);
    friend Vec3d operator-(const Point3d&, const UniversalCoord&);
    friend Vec3f operator-(const UniversalCoord&, const Point3f&);
    friend Vec3f operator-(const Point3f&, const UniversalCoord&);
    friend UniversalCoord operator+(const UniversalCoord&, const Vec3d&);
    friend UniversalCoord operator+(const UniversalCoord&, const Vec3f&);
    friend UniversalCoord operator-(const UniversalCoord&, const Vec3d&);
    friend UniversalCoord operator-(const UniversalCoord&, const Vec3f&);

    friend UniversalCoord operator+(const UniversalCoord&, const UniversalCoord&);

    double distanceTo(const UniversalCoord&);
    UniversalCoord difference(const UniversalCoord&) const;

    BigFix x, y, z;
};

Vec3d operator-(const UniversalCoord&, const UniversalCoord&);
Vec3d operator-(const UniversalCoord&, const Point3d&);
Vec3d operator-(const Point3d&, const UniversalCoord&);
Vec3f operator-(const UniversalCoord&, const Point3f&);
Vec3f operator-(const Point3f&, const UniversalCoord&);
UniversalCoord operator+(const UniversalCoord&, const Vec3d&);
UniversalCoord operator+(const UniversalCoord&, const Vec3f&);
UniversalCoord operator-(const UniversalCoord&, const Vec3d&);
UniversalCoord operator-(const UniversalCoord&, const Vec3f&);

// Not really proper--we can't add points.  But the only way around it is
// to create a separate version of UniversalCoord that acts like a vector.
UniversalCoord operator+(const UniversalCoord&, const UniversalCoord&);

#endif // _UNIVCOORD_H_

