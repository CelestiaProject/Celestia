// univcoord.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <cmath>
#include <celmath/mathlib.h>
#include "univcoord.h"


UniversalCoord operator+(const UniversalCoord& uc0, const UniversalCoord& uc1)
{
    return UniversalCoord(uc0.x + uc1.x, uc0.y + uc1.y, uc0.z + uc1.z);
}

UniversalCoord UniversalCoord::difference(const UniversalCoord& uc) const
{
    return UniversalCoord(x - uc.x, y - uc.y, z - uc.z);
}

#if DEPRECATED_UNIVCOORD_METHODS
UniversalCoord::UniversalCoord(const Point3d& p) :
    x(p.x), y(p.y), z(p.z)
{
}

UniversalCoord::UniversalCoord(const Point3f& p) :
    x(p.x), y(p.y), z(p.z)
{
}

UniversalCoord::operator Point3d() const
{
    return Point3d((double) x, (double) y, (double) z);
}

UniversalCoord::operator Point3f() const
{
    return Point3f((float) x, (float) y, (float) z);
}

double UniversalCoord::distanceTo(const UniversalCoord& uc)
{
    return sqrt(square((double) (uc.x - x)) +
                square((double) (uc.y - y)) +
                square((double) (uc.z - z)));
}

Vec3d operator-(const UniversalCoord& uc0, const UniversalCoord& uc1)
{
    return Vec3d((double) (uc0.x - uc1.x),
                 (double) (uc0.y - uc1.y),
                 (double) (uc0.z - uc1.z));
}

Vec3d operator-(const UniversalCoord& uc, const Point3d& p)
{
    return Vec3d((double) (uc.x - (BigFix) p.x),
                 (double) (uc.y - (BigFix) p.y),
                 (double) (uc.z - (BigFix) p.z));
}

Vec3d operator-(const Point3d& p, const UniversalCoord& uc)
{
    return Vec3d((double) ((BigFix) p.x - uc.x),
                 (double) ((BigFix) p.y - uc.y),
                 (double) ((BigFix) p.z - uc.z));
}

Vec3f operator-(const UniversalCoord& uc, const Point3f& p)
{
    return Vec3f((float) (uc.x - (BigFix) p.x),
                 (float) (uc.y - (BigFix) p.y),
                 (float) (uc.z - (BigFix) p.z));
}

Vec3f operator-(const Point3f& p, const UniversalCoord& uc)
{
    return Vec3f((float) ((BigFix) p.x - uc.x),
                 (float) ((BigFix) p.y - uc.y),
                 (float) ((BigFix) p.z - uc.z));
}

UniversalCoord operator+(const UniversalCoord& uc, const Vec3d& v)
{
    return UniversalCoord(uc.x + (BigFix) v.x,
                          uc.y + (BigFix) v.y,
                          uc.z + (BigFix) v.z);
}

UniversalCoord operator+(const UniversalCoord& uc, const Vec3f& v)
{
    return UniversalCoord(uc.x + BigFix((double) v.x),
                          uc.y + BigFix((double) v.y),
                          uc.z + BigFix((double) v.z));
}

UniversalCoord operator-(const UniversalCoord& uc, const Vec3d& v)
{
    return UniversalCoord(uc.x - BigFix(v.x),
                          uc.y - BigFix(v.y),
                          uc.z - BigFix(v.z));
}

UniversalCoord operator-(const UniversalCoord& uc, const Vec3f& v)
{
    return UniversalCoord(uc.x - BigFix((double) v.x),
                          uc.y - BigFix((double) v.y),
                          uc.z - BigFix((double) v.z));
}
#endif // DEPRECATED_UNIVCOORD_METHODS
