// ray.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_RAY_H_
#define _CELMATH_RAY_H_

#include <Eigen/Core>

namespace celmath
{

template<class T> class Ray3
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Ray3();
    Ray3(const Eigen::Matrix<T, 3, 1>& origin, const Eigen::Matrix<T, 3, 1>& direction);

    Eigen::Matrix<T, 3, 1> point(T) const;

    Ray3<T> transform(const Eigen::Matrix<T, 3, 3>& m) const
    {
        return Ray3<T>(m * origin, m * direction);
    }

    Ray3<T> transform(const Eigen::Matrix<T, 4, 4>& m) const
    {
        Eigen::Matrix<T, 4, 1> o(Eigen::Matrix<T, 4, 1>::Ones());
        o.head(3) = origin;
        Eigen::Matrix<T, 4, 1> d(Eigen::Matrix<T, 4, 1>::Zero());
        d.head(3) = direction;
        return Ray3<T>((m * o).head(3), (m * d).head(3));
    }

 public:
    Eigen::Matrix<T, 3, 1> origin;
    Eigen::Matrix<T, 3, 1> direction;
};

typedef Ray3<float>   Ray3f;
typedef Ray3<double>  Ray3d;


template<class T> Ray3<T>::Ray3() :
    origin(0, 0, 0), direction(0, 0, -1)
{
}

template<class T> Ray3<T>::Ray3(const Eigen::Matrix<T, 3, 1>& _origin,
                                const Eigen::Matrix<T, 3, 1>& _direction) :
    origin(_origin), direction(_direction)
{
}

template<class T> Eigen::Matrix<T, 3, 1> Ray3<T>::point(T t) const
{
    return origin + direction * t;
}

} // namespace celmath

#endif // _CELMATH_RAY_H_
