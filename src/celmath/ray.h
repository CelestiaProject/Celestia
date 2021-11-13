// ray.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace celmath
{
// Eigen 3.4 will support a transform method on ParametrizedLine, which may enable
// removing at least one of the below overloads.

template<typename T>
Eigen::ParametrizedLine<T, 3> transformRay(const Eigen::ParametrizedLine<T, 3>& line,
                                            const Eigen::Matrix<T, 3, 3>& m)
{
    return Eigen::ParametrizedLine<T, 3>(m * line.origin(), m * line.direction());
}

template<typename T>
Eigen::ParametrizedLine<T, 3> transformRay(const Eigen::ParametrizedLine<T, 3>& line,
                                            const Eigen::Matrix<T, 4, 4>& m)
{
    Eigen::Matrix<T, 4, 1> o(Eigen::Matrix<T, 4, 1>::Ones());
    o.head(3) = line.origin();
    Eigen::Matrix<T, 4, 1> d(Eigen::Matrix<T, 4, 1>::Zero());
    d.head(3) = line.direction();
    return Eigen::ParametrizedLine<T, 3>((m * o).head(3), (m * d).head(3));
}
} // end namespace celmath
