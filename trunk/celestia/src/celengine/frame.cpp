// frame.cpp
// 
// Copyright (C) 2003-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <celengine/frame.h>

using namespace std;


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
            rotation = refObject.body()->getEclipticalToBodyFixed(t);
            break;
        case Selection::Type_Star:
            rotation = refObject.star()->getRotationModel()->orientationAtTime(t);
            break;
        case Selection::Type_Location:
            if (refObject.location()->getParentBody() != NULL)
                rotation = refObject.location()->getParentBody()->getEclipticalToBodyFixed(t);
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
                Vec3d axisDir = Vec3d(0, 1, 0) * star->getRotationModel()->equatorOrientationAtTime(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }
        default:
            return xform;
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
            return xform;
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
            rotation = refObject.body()->getEclipticalToBodyFixed(t);
            break;
        case Selection::Type_Star:
            rotation = refObject.star()->getRotationModel()->orientationAtTime(t);
            break;
        case Selection::Type_Location:
            if (refObject.location()->getParentBody() != NULL)
                rotation = refObject.location()->getParentBody()->getEclipticalToBodyFixed(t);
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
                Vec3d axisDir = Vec3d(0, 1, 0) * star->getRotationModel()->equatorOrientationAtTime(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }

        default:
            return xform;
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
            return xform;
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


/*** ReferenceFrame ***/

ReferenceFrame::ReferenceFrame(Selection center) :
    centerObject(center),
    refCount(0)
{
}


int
ReferenceFrame::addRef() const
{
    return ++refCount;
}


int
ReferenceFrame::release() const
{
    --refCount;
    assert(refCount >= 0);
    if (refCount <= 0)
        delete this;

    return refCount;
}


// TODO: Not correct; requires BigFix * double multiplication
UniversalCoord
ReferenceFrame::convertFrom(const UniversalCoord& uc, double tjd) const
{
    UniversalCoord center = centerObject.getPosition(tjd);
    Vec3d relative = uc - center;

    return center + relative * getOrientation(tjd).toMatrix3();
}


// TODO: Not correct; requires BigFix * double multiplication
UniversalCoord
ReferenceFrame::convertTo(const UniversalCoord& uc, double tjd) const
{
    UniversalCoord center = centerObject.getPosition(tjd);
    Vec3d relative = uc - center;

    return center + relative * conjugate(getOrientation(tjd)).toMatrix3();
}


Point3d
ReferenceFrame::convertFromAstrocentric(const Point3d& p, double tjd) const
{
    Point3d center;
    if (centerObject.getType() == Selection::Type_Body)
    {
        Point3d center = centerObject.body()->getHeliocentricPosition(tjd);
        return center + p * getOrientation(tjd).toMatrix3();
    }
    else if (centerObject.getType() == Selection::Type_Star)
    {
        return p * getOrientation(tjd).toMatrix3();
    }
    else
    {
        // TODO:
        // bad if the center object is a galaxy
        // what about locations?
        return Point3d(0.0, 0.0, 0.0);
    }
}


Selection
ReferenceFrame::getCenter() const
{
    return centerObject;
}



/*** J2000EclipticFrame ***/

J2000EclipticFrame::J2000EclipticFrame(Selection center) :
    ReferenceFrame(center)
{
}


/*** J2000EquatorFrame ***/

J2000EquatorFrame::J2000EquatorFrame(Selection center) :
    ReferenceFrame(center)
{
}


Quatd
J2000EquatorFrame::getOrientation(double /* tjd */) const
{
    return Quatd::xrotation(astro::J2000Obliquity);
}


/*** BodyFixedFrame ***/

BodyFixedFrame::BodyFixedFrame(Selection center, Selection obj) :
    ReferenceFrame(center),
    fixObject(obj)
{
}


Quatd
BodyFixedFrame::getOrientation(double tjd) const
{
    // Rotation of 180 degrees about the y axis is required
    // TODO: this rotation could go in getEclipticalToBodyFixed()
    Quatd yrot180 = Quatd(0.0, 0.0, 1.0, 0.0);

    switch (fixObject.getType())
    {
    case Selection::Type_Body:
        return yrot180 * fixObject.body()->getEclipticalToBodyFixed(tjd);
    case Selection::Type_Star:
        return yrot180 * fixObject.star()->getRotationModel()->orientationAtTime(tjd);
    default:
        return yrot180;
    }
}


/*** BodyMeanEquatorFrame ***/

BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection center,
                                           Selection obj) :
    ReferenceFrame(center),
    equatorObject(obj),
    freezeEpoch(0.0),
    isFrozen(false)
{
}


BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection center,
                                           Selection obj,
                                           double freeze) :
    ReferenceFrame(center),
    equatorObject(obj),
    freezeEpoch(freeze),
    isFrozen(true)
{
}


Quatd
BodyMeanEquatorFrame::getOrientation(double tjd) const
{
    double t = isFrozen ? tjd : freezeEpoch;

    // TODO: need to consider frame of object
    switch (equatorObject.getType())
    {
    case Selection::Type_Body:
        return equatorObject.body()->getRotationModel()->equatorOrientationAtTime(t);
    case Selection::Type_Star:
        return equatorObject.star()->getRotationModel()->equatorOrientationAtTime(t);
    default:
        return Quatd(1.0);
    }
}


/*** CachingFrame ***/

CachingFrame::CachingFrame(Selection _center) :
    ReferenceFrame(_center),
    lastTime(-1.0e50),
    lastOrientation(1.0)
{
}


Quatd
CachingFrame::getOrientation(double tjd) const
{
    if (tjd == lastTime)
    {
        return lastOrientation;
    }
    else
    {
        lastTime = tjd;
        lastOrientation = computeOrientation(tjd);
        return lastOrientation;
    }
}


/*** TwoVectorFrame ***/

// Minimum angle permitted between primary and secondary axes of
// a two-vector frame.
const double TwoVectorFrame::Tolerance = 1.0e-6;

TwoVectorFrame::TwoVectorFrame(Selection center,
                               const FrameVector& prim,
                               int primAxis,
                               const FrameVector& sec,
                               int secAxis) :
    CachingFrame(center),
    primaryVector(prim),
    primaryAxis(primAxis),
    secondaryVector(sec),
    secondaryAxis(secAxis)
{
    // Verify that primary and secondary axes are valid
    assert(primaryAxis != 0 && secondaryAxis != 0);
    assert(abs(primaryAxis) <= 3 && abs(secondaryAxis) <= 3);
    // Verify that the primary and secondary axes aren't collinear
    assert(abs(primaryAxis) != abs(secondaryAxis));

    // The tertiary axis is the cross product of the primary and
    // secondary axes.
    int tertiarySign = sign(primaryAxis * secondaryAxis);
    if ((abs(primaryAxis) != 1 && abs(secondaryAxis) != 1))
    {
        // Order and signs of the primary and secondary determine the sign of
        // the tertiary axis.
        tertiaryAxis = 1 * tertiarySign;
        if (abs(primaryAxis) != 3)
            tertiaryAxis = -tertiaryAxis;
    }
    else if (abs(primaryAxis) != 2 && abs(secondaryAxis) != 2)
    {
        tertiaryAxis = 2 * tertiarySign;
        if (abs(primaryAxis) != 1)
            tertiaryAxis = -tertiaryAxis;
    }
    else
    {
        tertiaryAxis = 3 * tertiarySign;
        if (abs(primaryAxis) != 2)
            tertiaryAxis = -tertiaryAxis;
    }
}


Quatd
TwoVectorFrame::computeOrientation(double tjd) const
{
    Vec3d v0 = primaryVector.direction(tjd);
    Vec3d v1 = secondaryVector.direction(tjd);
    
    // TODO: verify that v0 and v1 aren't zero length
    v0.normalize();
    v1.normalize();

    Vec3d v2 = v0 ^ v1;

    // Check for degenerate case when the primary and secondary vectors
    // are collinear. A well-chosen two vector frame should never have this
    // problem.
    double length = v2.length();
    if (length < Tolerance)
    {
        // Just return identity . . .
        return Quatd(1.0);
    }
    else
    {
        v2 = v2 / length;
        Vec3d v[3];

        // Set the rotation matrix axes
        if (primaryAxis > 0)
            v[abs(primaryAxis) - 1] = v0;
        else
            v[abs(primaryAxis) - 1] = -v0;

        if (secondaryAxis > 0)
            v[abs(secondaryAxis) - 1] = v0 ^ v2;
        else
            v[abs(secondaryAxis) - 1] = v2 ^ v0;
    
        if (tertiaryAxis > 0)
            v[abs(tertiaryAxis) - 1] = v2;
        else
            v[abs(tertiaryAxis) - 1] = -v2;

        // The axes are the rows of a rotation matrix. The getOrientation
        // method must return the quaternion representation of the 
        // orientation, so convert the rotation matrix to a quaternion now.
        Quatd q = Quatd(Mat3d(v[0], v[1], v[2]));

        // A rotation matrix will have a determinant of 1; if the matrix also
        // includes a reflection, the determinant will be -1, indicating that
        // there's a bug and there's a reversed cross-product or sign error
        // somewhere.
        // assert(Mat3d(v[0], v[1], v[2]).determinant() > 0);

        return q;
    }
}


// Copy constructor
FrameVector::FrameVector(const FrameVector& fv) :
    vecType(fv.vecType),
    observer(fv.observer),
    target(fv.target),
    vec(fv.vec),
    frame(fv.frame)
{
    if (frame != NULL)
        frame->addRef();
}


// Assignment operator (since we have a copy constructor)
FrameVector&
FrameVector::operator=(const FrameVector& fv)
{
    vecType = fv.vecType;
    observer = fv.observer;
    target = fv.target;
    vec = fv.vec;

    if (frame != NULL)
        frame->release();
    frame = fv.frame;
    if (frame != NULL)
        frame->addRef();
    
    return *this;
}



FrameVector::FrameVector(FrameVectorType t) :
    vecType(t),
    observer(),
    target(),
    vec(0.0, 0.0, 0.0),
    frame(NULL)
{
}


FrameVector::~FrameVector()
{
    if (frame != NULL)
        frame->release();
}


FrameVector
FrameVector::createRelativePositionVector(const Selection& _observer,
                                          const Selection& _target)
{
    FrameVector fv(RelativePosition);
    fv.observer = _observer;
    fv.target = _target;

    return fv;
}


FrameVector
FrameVector::createRelativeVelocityVector(const Selection& _observer,
                                          const Selection& _target)
{
    FrameVector fv(RelativeVelocity);
    fv.observer = _observer;
    fv.target = _target;

    return fv;
}


FrameVector
FrameVector::createConstantVector(const Vec3d& _vec,
                                  const ReferenceFrame* _frame)
{
    FrameVector fv(ConstantVector);
    fv.vec = _vec;
    fv.frame = _frame;
    if (fv.frame != NULL)
        fv.frame->addRef();
    return fv;
}


Vec3d
FrameVector::direction(double tjd) const
{
    Vec3d v;

    switch (vecType)
    {
    case RelativePosition:
        v = target.getPosition(tjd) - observer.getPosition(tjd);
        break;

    case RelativeVelocity:
        // TODO: add getVelocity() method to selection
        {
            Vec3d v0 = observer.getPosition(tjd) - observer.getPosition(tjd - 1.0 / 1440.0);
            Vec3d v1 = target.getPosition(tjd) - target.getPosition(tjd - 1.0 / 1440.0);
            v = v1 - v0;
        }
        break;

    case ConstantVector:
        if (frame == NULL)
            v = vec;
        else
            v = vec * frame->getOrientation(tjd).toMatrix3();
        break;

    default:
        // unhandled vector type
        v = Vec3d(0.0, 0.0, 0.0);
        break;
    }

    return v;
}
