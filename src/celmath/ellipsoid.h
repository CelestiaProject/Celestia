// ellipsoid.h
//
// Copyright (C) 2002-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>

namespace celmath
{
template<class T>
class Ellipsoid
{
 public:
     /*! Default Ellipsoid constructor. Create a unit sphere centered
     *  at the origin.
     */
    Ellipsoid() :
        center(Eigen::Matrix<T, 3, 1>::Zero()),
        axes(Eigen::Matrix<T, 3, 1>::Ones())
    {
    }

    /*! Created an ellipsoid with the specified semiaxes, centered
     *  at the origin.
     */
    Ellipsoid(const Eigen::Matrix<T, 3, 1>& _axes) :
        center(Eigen::Matrix<T, 3, 1>::Zero()),
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
        return v * v <= static_cast<T>(1);
    }


 public:
    Eigen::Matrix<T, 3, 1> center;
    Eigen::Matrix<T, 3, 1> axes;
};

using Ellipsoidf = Ellipsoid<float>;
using Ellipsoidd = Ellipsoid<double>;
} // namespace celmath
