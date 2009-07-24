// univcoord.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Universal coordinate is a high-precision fixed point coordinate for
// locating objects in 3D space on scales ranging from millimeters to
// thousands of light years.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_UNIVCOORD_H_
#define _CELENGINE_UNIVCOORD_H_

#include <celutil/bigfix.h>
#include "astro.h"
#include <Eigen/Core>

#define DEPRECATED_UNIVCOORD_METHODS 1

#if DEPRECATED_UNIVCOORD_METHODS
#include <celmath/vecmath.h>
#endif

class UniversalCoord
{
 public:
    UniversalCoord()
    {
    }

    UniversalCoord(BigFix _x, BigFix _y, BigFix _z) :
    x(_x), y(_y), z(_z)
    {
    }

    UniversalCoord(double _x, double _y, double _z) :
        x(_x), y(_y), z(_z)
    {
    }

    explicit UniversalCoord(const Eigen::Vector3d& v) :
        x(v.x()), y(v.y()), z(v.z())
    {
    }

#if DEPRECATED_UNIVCOORD_METHODS
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
#endif
    friend UniversalCoord operator+(const UniversalCoord&, const UniversalCoord&);

    /** Compute a universal coordinate that is the sum of this coordinate and
      * an offset in kilometers.
      */
    UniversalCoord offsetKm(const Eigen::Vector3d& v)
    {
        Eigen::Vector3d vUly = v * astro::kilometersToMicroLightYears(1.0);
        return *this + UniversalCoord(vUly);
    }

    /** Compute a universal coordinate that is the sum of this coordinate and
      * an offset in micro-light years.
      *
      * This method is only here to help in porting older code; it shouldn't be
      * necessary to use it in new code, where the use of the rather the rather
      * obscure unit micro-light year isn't necessary.
      */
    UniversalCoord offsetUly(const Eigen::Vector3d& vUly)
    {
        return *this + UniversalCoord(vUly);
    }

    /** Get the offset in kilometers of this coordinate from another coordinate.
      * The result is double precision, calculated as (this - uc) * scale, where
      * scale is a factor that converts from Celestia's internal units to kilometers.
      */
    Eigen::Vector3d offsetFromKm(const UniversalCoord& uc) const
    {
        return Eigen::Vector3d((double) (x - uc.x), (double) (y - uc.y), (double) (z - uc.z)) * astro::microLightYearsToKilometers(1.0);
    }

    /** Get the offset in light years of this coordinate from a point (also with
      * units of light years.) The difference is calculated at high precision and
      * the reduced to single precision.
      */
    Eigen::Vector3f offsetFromLy(const Eigen::Vector3f& v) const
    {
        Eigen::Vector3f vUly = v * 1.0e6f;
        Eigen::Vector3f offsetUly((float) (x - (BigFix) vUly.x()),
                                  (float) (y - (BigFix) vUly.y()),
                                  (float) (z - (BigFix) vUly.z()));
        return offsetUly * 1.0e-6f;
    }

    /** Get the offset in light years of this coordinate from a point (also with
      * units of light years.) The difference is calculated at high precision and
      * the reduced to single precision. 
      *
      * This method is only here to help in porting older code; it shouldn't be
      * necessary to use it in new code, where the use of the rather the rather
      * obscure unit micro-light year isn't necessary.
      */
    Eigen::Vector3d offsetFromUly(const UniversalCoord& uc) const
    {
        return Eigen::Vector3d((double) (x - uc.x), (double) (y - uc.y), (double) (z - uc.z));
    }

    /** Get the value of the coordinate in light years. The result is truncated to
      * double precision.
      */
    Eigen::Vector3d toLy() const
    {
        return Eigen::Vector3d((double) x, (double) y, (double) z) * 1.0e-6;
    }

    double distanceFromKm(const UniversalCoord& uc)
    {
        return offsetFromKm(uc).norm();
    }

    double distanceFromLy(const UniversalCoord& uc)
    {
        return astro::kilometersToLightYears(offsetFromKm(uc).norm());
    }

#if DEPRECATED_UNIVCOORD_METHODS
    double distanceTo(const UniversalCoord&);
#endif
    UniversalCoord difference(const UniversalCoord&) const;

    static UniversalCoord Zero()
    {
        // Default constructor returns zero, but this static method is clearer
        return UniversalCoord();
    }

    /** Convert double precision coordinates in kilometers to high precision
      * universal coordinates.
      */
    static UniversalCoord CreateKm(const Eigen::Vector3d& v)
    {
        Eigen::Vector3d vUly = v * astro::microLightYearsToKilometers(1.0);
        return UniversalCoord(vUly.x(), vUly.y(), vUly.z());
    }

    /** Convert double precision coordinates in light years to high precision
      * universal coordinates.
      */
    static UniversalCoord CreateLy(const Eigen::Vector3d& v)
    {
        Eigen::Vector3d vUly = v * 1.0e6;
        return UniversalCoord(vUly.x(), vUly.y(), vUly.z());
    }

    /** Convert double precision coordinates in micro-light years to high precision
      * universal coordinates. This method is intended only for porting older code;
      * it should not be used by new code.
      */
    static UniversalCoord CreateUly(const Eigen::Vector3d& v)
    {
        Eigen::Vector3d vUly = v;
        return UniversalCoord(vUly.x(), vUly.y(), vUly.z());
    }

public:
    BigFix x, y, z;
};

#if DEPRECATED_UNIVCOORD_METHODS
Vec3d operator-(const UniversalCoord&, const UniversalCoord&);
Vec3d operator-(const UniversalCoord&, const Point3d&);
Vec3d operator-(const Point3d&, const UniversalCoord&);
Vec3f operator-(const UniversalCoord&, const Point3f&);
Vec3f operator-(const Point3f&, const UniversalCoord&);
UniversalCoord operator+(const UniversalCoord&, const Vec3d&);
UniversalCoord operator+(const UniversalCoord&, const Vec3f&);
UniversalCoord operator-(const UniversalCoord&, const Vec3d&);
UniversalCoord operator-(const UniversalCoord&, const Vec3f&);
#endif

UniversalCoord operator+(const UniversalCoord&, const UniversalCoord&);

#endif // _CELENGINE_UNIVCOORD_H_

