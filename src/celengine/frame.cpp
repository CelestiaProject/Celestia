// frame.cpp
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/frame.h>


RigidTransform FrameOfReference::toUniversal(const RigidTransform& xform,
                                             double t) const
{
    // Handle the easy case . . .
    if (coordSys == astro::Universal)
        return xform;
    UniversalCoord origin = refObject.getPosition(t);

    if (coordSys == astro::Geographic)
    {
        Quatd rotation(1, 0, 0, 0);
        switch (refObject.getType())
        {
        case Selection::Type_Body:
            rotation = refObject.body()->getEclipticalToGeographic(t);
            break;
        case Selection::Type_Star:
            rotation = refObject.star()->getRotationElements().eclipticalToPlanetographic(t);
            break;
        case Selection::Type_Location:
            if (refObject.location()->getParentBody() != NULL)
                rotation = refObject.location()->getParentBody()->getEclipticalToGeographic(t);
            break;
        default:
            break;
        }

        Point3d p = (Point3d) xform.translation * rotation.toMatrix4();
        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * rotation);
    }
    else if (coordSys == astro::PhaseLock)
    {
        Mat3d m;
        Vec3d lookDir = refObject.getPosition(t) - targetObject.getPosition(t);
        lookDir.normalize();

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }
            break;
        case Selection::Type_Star:
            {
                Star* star = refObject.star();
                Vec3d axisDir = Vec3d(0, 1, 0) * star->getRotationElements().eclipticalToEquatorial(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }
        default:
            break;
        }

        Point3d p = (Point3d) xform.translation * m;

        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * Quatd(m));
    }
    else if (coordSys == astro::Chase)
    {
        Mat3d m;

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d lookDir = body->getOrbit()->positionAtTime(t) -
                    body->getOrbit()->positionAtTime(t - 1.0 / 1440.0);
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                lookDir.normalize();
                Vec3d v = lookDir ^ axisDir;
                v.normalize();
                Vec3d u = v ^ lookDir;
                m = Mat3d(v, u, -lookDir);
            }
            break;
        default:
            break;
        }

        Point3d p = (Point3d) xform.translation * m;

        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * Quatd(m));
    }
    else
    {
        return RigidTransform(origin + xform.translation, xform.rotation);
    }
}


RigidTransform FrameOfReference::fromUniversal(const RigidTransform& xform,
                                               double t) const
{
    // Handle the easy case . . .
    if (coordSys == astro::Universal)
        return xform;
    UniversalCoord origin = refObject.getPosition(t);

    if (coordSys == astro::Geographic)
    {
        Quatd rotation(1, 0, 0, 0);
        switch (refObject.getType())
        {
        case Selection::Type_Body:
            rotation = refObject.body()->getEclipticalToGeographic(t);
            break;
        case Selection::Type_Star:
            rotation = refObject.star()->getRotationElements().eclipticalToPlanetographic(t);
            break;
        case Selection::Type_Location:
            if (refObject.location()->getParentBody() != NULL)
                rotation = refObject.location()->getParentBody()->getEclipticalToGeographic(t);
            break;
        default:
            break;
        }
        Vec3d v = (xform.translation - origin) * (~rotation).toMatrix4();
        
        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~rotation);
    }
    else if (coordSys == astro::PhaseLock)
    {
        Mat3d m;
        Vec3d lookDir = refObject.getPosition(t) - targetObject.getPosition(t);
        lookDir.normalize();

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }
            break;

        case Selection::Type_Star:
            {
                Star* star = refObject.star();
                Vec3d axisDir = Vec3d(0, 1, 0) * star->getRotationElements().eclipticalToEquatorial(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }

        default:
            break;
        }

        Vec3d v = (xform.translation - origin) * m.transpose();

        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~Quatd(m));
    }
    else if (coordSys == astro::Chase)
    {
        Mat3d m;

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d lookDir = body->getOrbit()->positionAtTime(t) -
                    body->getOrbit()->positionAtTime(t - 1.0 / 1440.0);
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                lookDir.normalize();
                Vec3d v = lookDir ^ axisDir;
                v.normalize();
                Vec3d u = v ^ lookDir;
                m = Mat3d(v, u, -lookDir);
            }
            break;

        default:
            break;
        }

        Vec3d v = (xform.translation - origin) * m.transpose();

        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~Quatd(m));
    }
    else
    {
        return RigidTransform(xform.translation.difference(origin),
                              xform.rotation);
    }
}
