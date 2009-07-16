// eigenport.h
//
// Utility functions for porting from Celestia vector library
// to Eigen. This file will be removed from Celestia when the port
// is complete.
//
// Copyright (C) 2009, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

template<typename T>
Eigen::Matrix<T, 3, 1> toEigen(const Vector3<T>& v)
{
    return Eigen::Matrix<T, 3, 1>(v.x, v.y, v.z);
}

template<typename T>
Eigen::Matrix<T, 3, 1> toEigen(const Point3<T>& p)
{
    return Eigen::Matrix<T, 3, 1>(p.x, p.y, p.z);
}

inline Eigen::Quaternionf toEigen(const Quatf& q)
{
    return Eigen::Quaternionf(q.w, q.x, q.y, q.z);
}

inline Eigen::Quaterniond toEigen(const Quatd& q)
{
    return Eigen::Quaterniond(q.w, q.x, q.y, q.z);
}

template<typename SCALAR>
Quat<SCALAR> fromEigen(const Eigen::Quaternion<SCALAR>& q)
{
    return Quat<SCALAR>(q.w(), q.x(), q.y(), q.z());
}



