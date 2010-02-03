// cmoddview - An application for previewing cmod and other 3D file formats
// supported by Celestia.
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CMODVIEW_MODEL_VIEW_WIDGET_H_
#define _CMODVIEW_MODEL_VIEW_WIDGET_H_

#include <QGLWidget>
#include <celmodel/model.h>
#include <Eigen/Core>
#include <Eigen/Geometry>


class ModelViewWidget : public QGLWidget
{
Q_OBJECT
public:
    ModelViewWidget(QWidget *parent);
    ~ModelViewWidget();

    void setModel(cmod::Model* model);
    cmod::Model* model() const
    {
        return m_model;
    }

    enum RenderStyle
    {
        NormalStyle,
        WireFrameStyle,
    };

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent* event);

    void setRenderStyle(RenderStyle style);
    RenderStyle renderStyle() const
    {
        return m_renderStyle;
    }

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:
    void renderModel(cmod::Model* model);

private:
    cmod::Model* m_model;
    double m_modelBoundingRadius;
    Eigen::Vector3d m_cameraPosition;
    Eigen::Quaterniond m_cameraOrientation;
    QPoint m_lastMousePosition;
    RenderStyle m_renderStyle;
};

#endif // _CMODVIEW_MODEL_VIEW_WIDGET_H_
