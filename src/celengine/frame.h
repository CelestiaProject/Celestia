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
    
    UniversalCoord convertFrom(const UniversalCoord& uc, double tjd) const;
    UniversalCoord convertTo(const UniversalCoord& uc, double tjd) const;

    Point3d convertFromAstrocentric(const Point3d& p, double tjd) const;

    virtual Quatd getOrientation(double tjd) const = 0;

 private:
    Selection centerObject;
};


//! J2000.0 Earth ecliptic frame
class J2000EclipticFrame : public ReferenceFrame
{
 public:
    J2000EclipticFrame(Selection center);

    Quatd getOrientation(double /* tjd */) const
    {
        return Quatd(1.0);
    }
};


//! J2000.0 Earth Mean Equator
class J2000EquatorFrame : public ReferenceFrame
{
 public:
    J2000EquatorFrame(Selection center);
    Quatd getOrientation(double tjd) const;
};


class BodyFixedFrame : public ReferenceFrame
{
 public:
    BodyFixedFrame(Selection center, Selection obj);
    Quatd getOrientation(double tjd) const;

 private:
    Selection fixObject;
};



class BodyMeanEquatorFrame : public ReferenceFrame
{
 public:
    BodyMeanEquatorFrame(Selection center, Selection obj);
    Quatd getOrientation(double tjd) const;

 private:
    Selection equatorObject;
};




#if 0
class TwoVectorFrame : public ReferenceFrame
{
};
#endif


#endif // _CELENGINE_FRAME_H_
