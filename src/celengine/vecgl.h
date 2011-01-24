// vecgl.h
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// Overloaded versions of GL functions
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _VECGL_H_
#define _VECGL_H_

#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celutil/color.h>


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
	Vec4f dir4 = Vec4f(dir.x, dir.y, dir.z, 0.0f);
    glLightfv(light, GL_POSITION, &(dir4.x));
}

inline void glLightPosition(GLenum light, const Point3f& pos)
{
	Vec4f pos4 = Vec4f(pos.x, pos.y, pos.z, 1.0f);
    glLightfv(light, GL_POSITION, &(pos4.x));
}

inline void glLightColor(GLenum light, GLenum which, const Vec3f& color)
{
	Vec4f color4 = Vec4f(color.x, color.y, color.z, 1.0f);
    glLightfv(light, which, &(color4.x));
}

inline void glLightColor(GLenum light, GLenum which, const Vec4f& color)
{
    glLightfv(light, which, &color.x);
}

inline void glLightColor(GLenum light, GLenum which, const Color& color)
{
	Vec4f color4 = Vec4f(color.red(), color.green(), color.blue(), color.alpha());
    glLightfv(light, which, &(color4.x));
}

inline void glAmbientLightColor(const Color& color)
{
	Vec4f color4 = Vec4f(color.red(), color.green(), color.blue(), color.alpha());
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &(color4.x));
}

#endif // _VECGL_H_

