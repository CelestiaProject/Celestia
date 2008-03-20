// axisarrow.h
//
// Copyright (C) 2007, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_AXISARROW_H_
#define _CELENGINE_AXISARROW_H_

#include <celutil/color.h>
#include <celmath/quaternion.h>
#include <celengine/referencemark.h>
#include <celengine/selection.h>

class Body;


class ArrowReferenceMark : public ReferenceMark
{
 public:
    ArrowReferenceMark(const Body& _body);

    void setSize(float _size);
    void setColor(Color _color);

    void render(Renderer* renderer, const Point3f& position, float discSize, double tdb) const;
    float boundingSphereRadius() const
    {
        return size;
    }

    bool isOpaque() const
    {
        return opacity == 1.0f;
    }

    virtual Vec3d getDirection(double tdb) const = 0;

 protected:
    const Body& body;

 private:
    float size;
    Color color;
    float opacity;
};


class AxesReferenceMark : public ReferenceMark
{
 public:
    AxesReferenceMark(const Body& _body);

    void setSize(float _size);
    void setOpacity(float _opacity);

    void render(Renderer* renderer, const Point3f& position, float discSize, double tdb) const;
    float boundingSphereRadius() const
    {
        return size;
    }

    bool isOpaque() const
    {
        return opacity == 1.0f;
    }

    virtual Quatd getOrientation(double tdb) const = 0;

 protected:
    const Body& body;

 private:
    float size;
    float opacity;
};


class BodyAxisArrows : public AxesReferenceMark
{
public:
    BodyAxisArrows(const Body& _body);
    Quatd getOrientation(double tdb) const;
};


class FrameAxisArrows : public AxesReferenceMark
{
public:
    FrameAxisArrows(const Body& _body);
    Quatd getOrientation(double tdb) const;
};


class SunDirectionArrow : public ArrowReferenceMark
{
public:
    SunDirectionArrow(const Body& _body);
    Vec3d getDirection(double tdb) const;
};


class VelocityVectorArrow : public ArrowReferenceMark
{
public:
    VelocityVectorArrow(const Body& _body);
    Vec3d getDirection(double tdb) const;
};


class SpinVectorArrow : public ArrowReferenceMark
{
public:
    SpinVectorArrow(const Body& _body);
    Vec3d getDirection(double tdb) const;
};


/*! The body-to-body direction arrow points from the center of
 *  the primary body toward a target object.
 */
class BodyToBodyDirectionArrow : public ArrowReferenceMark
{
public:
    BodyToBodyDirectionArrow(const Body& _body, const Selection& _target);
    Vec3d getDirection(double tdb) const;

private:
    Selection target;
};

#endif // _CELENGINE_AXISARROW_H_



