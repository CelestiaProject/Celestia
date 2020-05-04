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


/**** Eigen helpers for OpenGL ****/

inline void glMatrix(const Eigen::Matrix4f& m)
{
    glMultMatrixf(m.data());
}

inline void glMatrix(const Eigen::Matrix4d& m)
{
    glMultMatrixd(m.data());
}

inline void glLoadMatrix(const Eigen::Matrix4f& m)
{
    glLoadMatrixf(m.data());
}

inline void glLoadMatrix(const Eigen::Matrix4d& m)
{
    glLoadMatrixd(m.data());
}

inline void glScale(const Eigen::Vector3f& scale)
{
    glScalef(scale.x(), scale.y(), scale.z());
}

inline void glTranslate(const Eigen::Vector3f& offset)
{
    glTranslatef(offset.x(), offset.y(), offset.z());
}

inline void glTranslate(const Eigen::Vector3d& offset)
{
    glTranslated(offset.x(), offset.y(), offset.z());
}

inline void glRotate(const Eigen::Quaternionf& q)
{
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.topLeftCorner(3, 3) = q.toRotationMatrix();
    glMultMatrixf(m.data());
}

inline void glRotate(const Eigen::Quaterniond& q)
{
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    m.topLeftCorner(3, 3) = q.toRotationMatrix();
    glMultMatrixd(m.data());
}

inline void glVertex(const Eigen::Vector3f& v)
{
    glVertex3fv(v.data());
}

#if 0
inline void glVertex(const Eigen::Vector3d& v)
{
    glVertex3dv(v.data());
}
#endif

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

#endif // _CELENGINE_VECGL_H_
