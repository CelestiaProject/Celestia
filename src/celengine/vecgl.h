// vecgl.h
//
// Copyright (C) 2000-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Overloaded versions of GL functions
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_VECGL_H_
#define _CELENGINE_VECGL_H_

#include <celutil/color.h>
#include "glsupport.h"
#include <Eigen/Core>
#include <Eigen/Geometry>


/**** Helpers for OpenGL ****/

inline void glVertexAttrib(GLuint index, const Color &color)
{
#ifdef GL_ES
    glVertexAttrib4fv(index, color.toVector4().data());
#else
    glVertexAttrib4Nubv(index, color.data());
#endif
}

inline void glVertexAttrib(GLuint index, const Eigen::Vector4f &v)
{
    glVertexAttrib4fv(index, v.data());
}

namespace celestia::vecgl
{
template<typename T>
inline Eigen::Matrix<T,4,4>
scale(const Eigen::Matrix<T,4,4> &m, const Eigen::Matrix<T,4,1> &s)
{
    return m * Eigen::Transform<T,3,Eigen::Affine>(Eigen::Scaling(s.x(), s.y(), s.z())).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
scale(const Eigen::Matrix<T,4,1> &s)
{
    Eigen::Matrix<T,4,4> sm(Eigen::Matrix<T,4,4>::Zero());
    sm.diagonal() = s;
    return sm;
}

template<typename T>
inline Eigen::Matrix<T,4,4>
scale(const Eigen::Matrix<T,4,4> &m, const Eigen::Matrix<T,3,1> &s)
{
    return m * Eigen::Transform<T,3,Eigen::Affine>(Eigen::Scaling(s.x(), s.y(), s.z())).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
scale(const Eigen::Matrix<T,3,1> &s)
{
    return Eigen::Transform<T,3,Eigen::Affine>(Eigen::Scaling(s.x(), s.y(), s.z())).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
scale(const Eigen::Matrix<T,4,4> &m, T s)
{
    return m * Eigen::Transform<T,3,Eigen::Affine>(Eigen::Scaling(s)).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
scale(T s)
{
    return Eigen::Transform<T,3,Eigen::Affine>(Eigen::Scaling(s)).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
rotate(const Eigen::Matrix<T,4,4> &m, const Eigen::Quaternion<T> &q)
{
    return m * Eigen::Transform<T,3,Eigen::Affine>(q).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
rotate(const Eigen::Quaternion<T> &q)
{
    return Eigen::Transform<T,3,Eigen::Affine>(q).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
rotate(const Eigen::Matrix<T,4,4> &m, const Eigen::AngleAxis<T> &aa)
{
    return m * Eigen::Transform<T,3,Eigen::Affine>(aa).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
rotate(const Eigen::AngleAxis<T> &aa)
{
    return Eigen::Transform<T,3,Eigen::Affine>(aa).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
rotate(const Eigen::Matrix<T,4,4> &m, T angle, const Eigen::Matrix<T,3,1> &axis)
{
    return rotate(m, Eigen::AngleAxis<T>(angle, axis));
}

template<typename T>
inline Eigen::Matrix<T,4,4>
rotate(T angle, const Eigen::Matrix<T,3,1> &axis)
{
    return rotate(Eigen::AngleAxis<T>(angle, axis));
}

template<typename T>
inline Eigen::Matrix<T,4,4>
translate(const Eigen::Matrix<T,4,4> &m, const Eigen::Matrix<T,3,1> &t)
{
    return m * Eigen::Transform<T,3,Eigen::Affine>(Eigen::Translation<T,3>(t)).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
translate(const Eigen::Matrix<T,3,1> &t)
{
    return Eigen::Transform<T,3,Eigen::Affine>(Eigen::Translation<T,3>(t)).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
translate(const Eigen::Matrix<T,4,4> &m, T x, T y, T z)
{
    return m * Eigen::Transform<T,3,Eigen::Affine>(Eigen::Translation<T,3>(Eigen::Matrix<T,3,1>(x,y,z))).matrix();
}

template<typename T>
inline Eigen::Matrix<T,4,4>
translate(T x, T y, T z)
{
    return Eigen::Transform<T,3,Eigen::Affine>(Eigen::Translation<T,3>(Eigen::Matrix<T,3,1>(x,y,z))).matrix();
}
} // end namespace celestia::vecgl

#endif // _CELENGINE_VECGL_H_
