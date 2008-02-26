// frustum.h
// 
// Copyright (C) 2000-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_FRUSTUM_H_
#define _CELMATH_FRUSTUM_H_

#include <celmath/plane.h>
#include <celmath/capsule.h>


class Frustum
{
 public:
    Frustum(float fov, float aspectRatio, float nearDist);
    Frustum(float fov, float aspectRatio, float nearDist, float farDist);

    void transform(const Mat3f&);
    void transform(const Mat4f&);

    inline Planef getPlane(int) const;

    enum {
        Bottom    = 0,
        Top       = 1,
        Left      = 2,
        Right     = 3,
        Near      = 4,
        Far       = 5,
    };

    enum Aspect {
        Outside   = 0,
        Inside    = 1,
        Intersect = 2,
    };

    Aspect test(const Point3f&) const;
    Aspect testSphere(const Point3f& center, float radius) const;
    Aspect testSphere(const Point3d& center, double radius) const;
    Aspect testCapsule(const Capsulef&) const;

 private:
    void init(float, float, float, float);

    Planef planes[6];
    bool infinite;
};

Planef Frustum::getPlane(int which) const
{
    return planes[which];
}

#endif // _CELMATH_FRUSTUM_H_
