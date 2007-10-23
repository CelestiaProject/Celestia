// frame.h
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_FRAME_H_
#define _CELENGINE_FRAME_H_

#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/astro.h>
#include <celengine/selection.h>


struct RigidTransform
{
    RigidTransform() :
        translation(0.0, 0.0, 0.0), rotation(1.0, 0.0, 0.0, 0.0) {};
    RigidTransform(const UniversalCoord& uc) :
        translation(uc), rotation(1.0f) {};
    RigidTransform(const UniversalCoord& uc, const Quatd& q) :
        translation(uc), rotation(q) {};
    RigidTransform(const UniversalCoord& uc, const Quatf& q) :
        translation(uc), rotation(q.w, q.x, q.y, q.z) {};
    UniversalCoord translation;
    Quatd rotation;
};


struct FrameOfReference
{
    FrameOfReference() :
        coordSys(astro::Universal) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Body* _body) :
        coordSys(_coordSys), refObject(_body) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Star* _star) :
        coordSys(_coordSys), refObject(_star) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, DeepSkyObject* _deepsky) :
        coordSys(_coordSys), refObject(_deepsky) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, const Selection& sel) :
        coordSys(_coordSys), refObject(sel) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, const Selection& ref,
                     const Selection& target) :
        coordSys(_coordSys), refObject(ref), targetObject(target) {};

    RigidTransform toUniversal(const RigidTransform& xform, double t) const;
    RigidTransform fromUniversal(const RigidTransform& xform, double t) const;

    astro::CoordinateSystem coordSys;
    Selection refObject;
    Selection targetObject;
};


// class RefCountedObject

/*!
Frame
{
   Center "Sol"
   # Orientation "J2000"
   # Orientation "J2000Ecliptic"
   TwoAxis
   {
       Primary
       {
          Axis "+X"
          Observer "Earth"
          Target "Sun"
       }

       Secondary
       {
           Axis "+Y"
       }
   }
}
*/


class ReferenceFrame
{
 public:
    ReferenceFrame(Selection center);
    virtual ~ReferenceFrame() {};

    int addRef() const;
    int release() const;
    
    UniversalCoord convertFrom(const UniversalCoord& uc, double tjd) const;
    UniversalCoord convertTo(const UniversalCoord& uc, double tjd) const;

    Point3d convertFromAstrocentric(const Point3d& p, double tjd) const;
    
    Selection getCenter() const;

    virtual Quatd getOrientation(double tjd) const = 0;

    unsigned int nestingDepth(unsigned int maxDepth) const;

    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth) const = 0;

 private:
    Selection centerObject;
    mutable int refCount;
};


/*! Base class for complex frames where there may be some benefit
 *  to caching the last calculated orientation.
 */
class CachingFrame : public ReferenceFrame
{
 public:
    CachingFrame(Selection _center);
    virtual ~CachingFrame() {};

    Quatd getOrientation(double tjd) const;
    virtual Quatd computeOrientation(double tjd) const = 0;

 private:
    mutable double lastTime;
    mutable Quatd lastOrientation;
};


//! J2000.0 Earth ecliptic frame
class J2000EclipticFrame : public ReferenceFrame
{
 public:
    J2000EclipticFrame(Selection center);
    virtual ~J2000EclipticFrame() {};

    Quatd getOrientation(double /* tjd */) const
    {
        return Quatd(1.0);
    }

    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth) const;
};


//! J2000.0 Earth Mean Equator
class J2000EquatorFrame : public ReferenceFrame
{
 public:
    J2000EquatorFrame(Selection center);
    virtual ~J2000EquatorFrame() {};
    Quatd getOrientation(double tjd) const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth) const;
};


/*! A BodyFixed frame is a coordinate system with the x-axis pointing
 *  from the body center through the intersection of the prime meridian
 *  and the equator, and the z-axis aligned with the north pole. The
 *  y-axis is the cross product of x and z, and points toward the 90
 *  meridian.
 */
class BodyFixedFrame : public ReferenceFrame
{
 public:
    BodyFixedFrame(Selection center, Selection obj);
    virtual ~BodyFixedFrame() {};
    Quatd getOrientation(double tjd) const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth) const;

 private:
    Selection fixObject;
};


class BodyMeanEquatorFrame : public ReferenceFrame
{
 public:
    BodyMeanEquatorFrame(Selection center, Selection obj, double freeze);
    BodyMeanEquatorFrame(Selection center, Selection obj);
    virtual ~BodyMeanEquatorFrame() {};
    Quatd getOrientation(double tjd) const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth) const;

 private:
    Selection equatorObject;
    double freezeEpoch;
    bool isFrozen;
};


/*! FrameVectors are used to define the axes for TwoVector frames
 */
class FrameVector
{
 public:
    FrameVector(const FrameVector& fv);
    ~FrameVector();
    FrameVector& operator=(const FrameVector&);

    Vec3d direction(double tjd) const;

    /*! Frames can be defined in reference to other frames; this method
     *  counts the depth of such nesting, up to some specified maximum
     *  level. This method is used to test for circular references in
     *  frames.
     */
    unsigned int nestingDepth(unsigned int depth, unsigned int maxDepth) const;

    enum FrameVectorType
    {
        RelativePosition,
        RelativeVelocity,
        ConstantVector,
    };
    
    static FrameVector createRelativePositionVector(const Selection& _observer,
                                                    const Selection& _target);
    static FrameVector createRelativeVelocityVector(const Selection& _observer,
                                                    const Selection& _target);
    static FrameVector createConstantVector(const Vec3d& _vec,
                                            const ReferenceFrame* _frame);

 private:
    /*! Type-only constructor is private. Code outside the class should
     *  use create*Vector methods to create new FrameVectors.
     */
    FrameVector(FrameVectorType t);

    FrameVectorType vecType;
    Selection observer;
    Selection target;
    Vec3d vec;                   // constant vector
    const ReferenceFrame* frame; // frame for constant vector
};


/*! A two vector frame is a coordinate system defined by a primary and
 *  secondary vector. The primary axis points in the direction of the
 *  primary vector. The secondary axis points in the direction of the
 *  component of the secondary vector that is orthogonal to the primary
 *  vector. The third axis is the cross product of the primary and
 *  secondary axis.
 */
class TwoVectorFrame : public CachingFrame
{
 public:
    /*! primAxis and secAxis are the labels of the axes defined by
     *  the primary and secondary vectors:
     *  1 = x, 2 = y, 3 = z, -1 = -x, -2 = -y, -3 = -z
     */
    TwoVectorFrame(Selection center, 
                   const FrameVector& prim,
                   int primAxis,
                   const FrameVector& sec,
                   int secAxis);
    virtual ~TwoVectorFrame() {};

    Quatd computeOrientation(double tjd) const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth) const;

    //! The sine of minimum angle between the primary and secondary vectors
    static const double Tolerance;

 private:
    FrameVector primaryVector;
    int primaryAxis;
    FrameVector secondaryVector;
    int secondaryAxis;
    int tertiaryAxis;
};


#endif // _CELENGINE_FRAME_H_
