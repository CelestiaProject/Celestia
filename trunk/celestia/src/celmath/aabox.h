// aabox.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Axis-aligned bounding box class
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _AABOX_H_
#define _AABOX_H_

#include <celmath/vecmath.h>

class AxisAlignedBox
{
 public:
    inline AxisAlignedBox();
    AxisAlignedBox(Point3f _min, Point3f _max) :
        minimum(_min), maximum(_max) {};
    AxisAlignedBox(Point3f center) :
        minimum(center), maximum(center) {};

    inline Point3f getMinimum() const;
    inline Point3f getMaximum() const;
    inline Point3f getCenter() const;
    inline Vec3f getExtents() const;

    inline bool empty() const;
    inline bool contains(const Point3f&) const;

    inline void include(const Point3f&);
    inline void include(const AxisAlignedBox&);

 private:    
    Point3f minimum;
    Point3f maximum;
};


AxisAlignedBox::AxisAlignedBox() :
    minimum(1.0e20f, 1.0e20f, 1.0e20f),
    maximum(-1.0e20f, -1.0e20f, -1.0e20f)
{
}

Point3f AxisAlignedBox::getMinimum() const
{
    return minimum;
}

Point3f AxisAlignedBox::getMaximum() const
{
    return maximum;
}

Point3f AxisAlignedBox::getCenter() const
{
    return Point3f((minimum.x + maximum.x) * 0.5f,
                   (minimum.y + maximum.y) * 0.5f,
                   (minimum.z + maximum.z) * 0.5f);
}

Vec3f AxisAlignedBox::getExtents() const
{
    return maximum - minimum;
}

bool AxisAlignedBox::empty() const
{
    return maximum.x < minimum.x || maximum.y < minimum.y || maximum.z < minimum.z;
}

bool AxisAlignedBox::contains(const Point3f& p) const
{
    return (p.x >= minimum.x && p.x <= maximum.x &&
            p.y >= minimum.y && p.y <= maximum.y &&
            p.z >= minimum.z && p.z <= maximum.z);
}


void AxisAlignedBox::include(const Point3f& p)
{
    if (p.x < minimum.x) minimum.x = p.x;
    if (p.x > maximum.x) maximum.x = p.x;
    if (p.y < minimum.y) minimum.y = p.y;
    if (p.y > maximum.y) maximum.y = p.y;
    if (p.z < minimum.z) minimum.z = p.z;
    if (p.z > maximum.z) maximum.z = p.z;
}

void AxisAlignedBox::include(const AxisAlignedBox& b)
{
    if (b.minimum.x < minimum.x) minimum.x = b.minimum.x;
    if (b.maximum.x > maximum.x) maximum.x = b.maximum.x;
    if (b.minimum.y < minimum.y) minimum.y = b.minimum.y;
    if (b.maximum.y > maximum.y) maximum.y = b.maximum.y;
    if (b.minimum.z < minimum.z) minimum.z = b.minimum.z;
    if (b.maximum.z > maximum.z) maximum.z = b.maximum.z;
}

#if 0
AxisAlignedBox union(const AxisAlignedBox& a, const AxisAlignedBox& b)
{
    AxisAlignedBox box(a);
    box.union(b);
    return box;
}
#endif

#endif // _AABOX_H_
