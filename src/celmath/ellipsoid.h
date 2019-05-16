// ellipsoid.h
//
// Copyright (C) 2002-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_ELLIPSOID_H_
#define _CELMATH_ELLIPSOID_H_

#include <Eigen/Core>

namespace celmath
{

template<class T> class Ellipsoid
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

     /*! Default Ellipsoid constructor. Create a unit sphere centered
     *  at the origin.
     */
    Ellipsoid() :
        center(0, 0, 0),
        axes(1, 1, 1)
    {
    }

    /*! Created an ellipsoid with the specified semiaxes, centered
     *  at the origin.
     */
    Ellipsoid(const Eigen::Matrix<T, 3, 1>& _axes) :
        center(0, 0, 0),
        axes(_axes)
    {
    }

    /*! Create an ellipsoid with the specified center and semiaxes.
     */
    Ellipsoid(const Eigen::Matrix<T, 3, 1>& _center,
              const Eigen::Matrix<T, 3, 1>& _axes) :
        center(_center),
        axes(_axes)
    {
    }

    /*! Test whether the point p lies inside the ellipsoid.
     */
    bool contains(const Eigen::Matrix<T, 3, 1>& p) const
    {
        Eigen::Matrix<T, 3, 1> v = p - center;
        v = v.cwise() / axes;
        return v * v <= (T) 1.0;
    }


 public:
    Eigen::Matrix<T, 3, 1> center;
    Eigen::Matrix<T, 3, 1> axes;
};

typedef Ellipsoid<float>   Ellipsoidf;
typedef Ellipsoid<double>  Ellipsoidd;

}; // namespace celmath

#endif // _CELMATH_ELLIPSOID_H_
