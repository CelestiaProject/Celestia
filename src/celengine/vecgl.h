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

#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celutil/color.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

inline void glVertex(const Point3f& p)
{
    glVertex3fv(&p.x);
}

inline void glVertex(const Vec3f& v)
{
    glVertex3fv(&v.x);
}

inline void glNormal(const Vec3f& n)
{
    glNormal3fv(&n.x);
}

inline void glTexCoord(const Point2f& p)
{
    glTexCoord2fv(&p.x);
}

inline void glColor(const Color& c)
{
    glColor4f(c.red(), c.green(), c.blue(), c.alpha());
}

inline void glColor(const Color& c, float a)
{
    glColor4f(c.red(), c.green(), c.blue(), c.alpha() * a);
}


inline void glMatrix(const Mat4f& m)
{
    Mat4f trans = m.transpose();
    glMultMatrixf(&trans[0].x);
}


inline void glMatrix(const Mat4d& m)
{
    Mat4d trans = m.transpose();
    glMultMatrixd(&trans[0].x);
}


inline void glRotate(const Quatf& q)
{
    glMatrix(q.toMatrix4());
}

inline void glRotate(const Quatd& q)
{
    glMatrix(q.toMatrix4());
}

inline void glTranslate(const Vec3f& v)
{
    glTranslatef(v.x, v.y, v.z);
}

inline void glTranslate(const Point3f& p)
{
    glTranslatef(p.x, p.y, p.z);
}

inline void glScale(const Vec3f& v)
{
    glScalef(v.x, v.y, v.z);
}

inline void glLightDirection(GLenum light, const Vec3f& dir)
{
    glLightfv(light, GL_POSITION, &(Vec4f(dir.x, dir.y, dir.z, 0.0f).x));
}

inline void glLightPosition(GLenum light, const Point3f& pos)
{
    glLightfv(light, GL_POSITION, &(Vec4f(pos.x, pos.y, pos.z, 1.0f).x));
}

inline void glLightColor(GLenum light, GLenum which, const Vec3f& color)
{
    glLightfv(light, which, &(Vec4f(color.x, color.y, color.z, 1.0f).x));
}

inline void glLightColor(GLenum light, GLenum which, const Vec4f& color)
{
    glLightfv(light, which, &color.x);
}

inline void glLightColor(GLenum light, GLenum which, const Color& color)
{
    glLightfv(light, which,
              &(Vec4f(color.red(), color.green(), color.blue(), color.alpha()).x));
}

inline void glAmbientLightColor(const Color& color)
{
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,
                   &(Vec4f(color.red(), color.green(), color.blue(),
                           color.alpha()).x));
}


/**** Eigen helpers for OpenGL ****/

inline void glMatrix(const Eigen::Matrix4f& m)
{
    glMultMatrixf(m.data());
}

inline void glMatrix(const Eigen::Matrix4d& m)
{
    glMultMatrixd(m.data());
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
    m.corner<3, 3>(Eigen::TopLeft) = q.toRotationMatrix();
    glMultMatrixf(m.data());
}

inline void glRotate(const Eigen::Quaterniond& q)
{
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    m.corner<3, 3>(Eigen::TopLeft) = q.toRotationMatrix();
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

inline void glLightDirection(GLenum light, const Eigen::Vector3f& dir)
{
    glLightfv(light, GL_POSITION, Eigen::Vector4f(dir.x(), dir.y(), dir.z(), 0.0f).data());
}

inline void glLightPosition(GLenum light, const Eigen::Vector3f& pos)
{
    glLightfv(light, GL_POSITION, Eigen::Vector4f(pos.x(), pos.y(), pos.z(), 1.0f).data());
}

template<typename DERIVED>
void glLightColor(GLenum light, GLenum which, const Eigen::MatrixBase<DERIVED>& color)
{
    glLightfv(light, which, Eigen::Vector4f(color.x(), color.y(), color.z(), 1.0f).data());
}

inline void glLightColor(GLenum light, GLenum which, const Eigen::Vector4f& color)
{
    glLightfv(light, which, color.data());
}

#endif // _CELENGINE_VECGL_H_

