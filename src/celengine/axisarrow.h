// axisarrow.h
//
// Copyright (C) 2007-2009, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celutil/color.h>
#include <celengine/referencemark.h>
#include <celengine/selection.h>
#include <celengine/shadermanager.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

class Body;

class ArrowReferenceMark : public ReferenceMark
{
public:
    explicit ArrowReferenceMark(const Body& _body);

    void setSize(float _size);
    void setColor(const Color& _color);

    void render(Renderer* renderer,
                const Eigen::Vector3f& position,
                float discSize,
                double tdb,
                const Matrices& m) const override;
    float boundingSphereRadius() const override
    {
        return size;
    }

    bool isOpaque() const override
    {
        return opacity == 1.0f;
    }

    virtual Eigen::Vector3d getDirection(double tdb) const = 0;

protected:
    const Body& body;

private:
    float size;
    Color color;
    float opacity;
    ShaderProperties shadprop;
};

class AxesReferenceMark : public ReferenceMark
{
public:
    explicit AxesReferenceMark(const Body& _body);

    void setSize(float _size);
    void setOpacity(float _opacity);

    void render(Renderer* renderer,
                const Eigen::Vector3f& position,
                float discSize,
                double tdb,
                const Matrices& m) const override;
    float boundingSphereRadius() const override
    {
        return size;
    }

    bool isOpaque() const override
    {
        return opacity == 1.0f;
    }

    virtual Eigen::Quaterniond getOrientation(double tdb) const = 0;

protected:
    const Body& body;

private:
    float size;
    float opacity;
    ShaderProperties shadprop;
};


class BodyAxisArrows : public AxesReferenceMark
{
public:
    explicit BodyAxisArrows(const Body& _body);
    Eigen::Quaterniond getOrientation(double tdb) const override;
};


class FrameAxisArrows : public AxesReferenceMark
{
public:
    explicit FrameAxisArrows(const Body& _body);
    Eigen::Quaterniond getOrientation(double tdb) const override;
};


class SunDirectionArrow : public ArrowReferenceMark
{
public:
    explicit SunDirectionArrow(const Body& _body);
    Eigen::Vector3d getDirection(double tdb) const override;
};


class VelocityVectorArrow : public ArrowReferenceMark
{
public:
    explicit VelocityVectorArrow(const Body& _body);
    Eigen::Vector3d getDirection(double tdb) const override;
};


class SpinVectorArrow : public ArrowReferenceMark
{
public:
    explicit SpinVectorArrow(const Body& _body);
    Eigen::Vector3d getDirection(double tdb) const override;
};


/*! The body-to-body direction arrow points from the center of
 *  the primary body toward a target object.
 */
class BodyToBodyDirectionArrow : public ArrowReferenceMark
{
public:
    BodyToBodyDirectionArrow(const Body& _body, const Selection& _target);
    Eigen::Vector3d getDirection(double tdb) const override;

private:
    Selection target;
};
