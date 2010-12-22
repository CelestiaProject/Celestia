// sensorgeometry.h
//
// Copyright (C) 2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SENSOR_GEOMETRY_H_
#define _CELENGINE_SENSOR_GEOMETRY_H_

#include "geometry.h"
#include <celutil/color.h>
#include <celutil/resmanager.h>

class Body;

class SensorGeometry : public Geometry
{
 public:
    SensorGeometry();
    ~SensorGeometry();

    enum SensorShape
    {
        EllipticalShape,
        RectangularShape,
    };

    virtual bool pick(const Ray3d& r, double& distance) const;

    //! Render the model in the current OpenGL context
    virtual void render(RenderContext&, double t = 0.0);

    virtual bool isOpaque() const;
    virtual bool isNormalized() const;

    virtual bool isMultidraw() const
    {
        return true;
    }

    virtual void setPartVisible(const std::string& partName, bool visible);
    virtual bool isPartVisible(const std::string& partName) const;

    Body* observer() const
    {
        return m_observer;
    }

    void setObserver(Body* observer)
    {
        m_observer = observer;
    }

    Body* target() const
    {
        return m_target;
    }

    void setTarget(Body* target)
    {
        m_target = target;
    }

    double range() const
    {
        return m_range;
    }

    void setRange(double range)
    {
        m_range = range;
    }

    SensorShape shape() const
    {
        return m_shape;
    }

    void setShape(SensorShape shape)
    {
        m_shape = shape;
    }

    Color frustumColor() const
    {
        return m_frustumColor;
    }

    void setFrustumColor(const Color& color)
    {
        m_frustumColor = color;
    }

    Color frustumBaseColor() const
    {
        return m_frustumBaseColor;
    }

    void setFrustumBaseColor(const Color& color)
    {
        m_frustumBaseColor = color;
    }

    float frustumOpacity() const
    {
        return m_frustumOpacity;
    }

    void setFrustumOpacity(float opacity)
    {
        m_frustumOpacity = opacity;
    }

    float gridOpacity() const
    {
        return m_gridOpacity;
    }

    void setGridOpacity(float opacity)
    {
        m_gridOpacity = opacity;
    }

    void setFOVs(double horizontalFov, double verticalFov);


 private:
    Body* m_observer;
    Body* m_target;
    double m_range;
    double m_horizontalFov;
    double m_verticalFov;
    Color m_frustumColor;
    Color m_frustumBaseColor;
    float m_frustumOpacity;
    float m_gridOpacity;
    SensorShape m_shape;
    bool m_frustumVisible;
    bool m_frustumBaseVisible;
};

#endif // !_CELENGINE_SENSOR_GEOMETRY_H_
