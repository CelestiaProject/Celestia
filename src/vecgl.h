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

#include "vecmath.h"
#include "quaternion.h"
#include "color.h"

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

inline void glRotate(const Quatf& q)
{
    Vec3f axis;
    float angle;
    q.getAxisAngle(axis, angle);
    glRotatef(radToDeg(angle), axis.x, axis.y, axis.z);
}

inline void glTranslate(const Vec3f& v)
{
    glTranslatef(v.x, v.y, v.z);
}

inline void glTranslate(const Point3f& p)
{
    glTranslatef(p.x, p.y, p.z);
}

inline void glLightDirection(int light, Vec3f& dir)
{
    glLightfv(light, GL_POSITION, &(Vec4f(dir.x, dir.y, dir.z, 0.0f).x));
}

inline void glLightPosition(int light, Point3f& pos)
{
    glLightfv(light, GL_POSITION, &(Vec4f(pos.x, pos.y, pos.z, 1.0f).x));
}

inline void glLightColor(int light, int which, Vec3f& color)
{
    glLightfv(light, which, &(Vec4f(color.x, color.y, color.z, 1.0f).x));
}

inline void glLightColor(int light, int which, Vec4f& color)
{
    glLightfv(light, which, &color.x);
}

#endif // _VECGL_H_

