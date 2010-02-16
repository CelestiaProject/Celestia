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
#include <QSet>
#include <celmodel/model.h>
#include <Eigen/Core>
#include <Eigen/Geometry>


class MaterialLibrary;

class ModelViewWidget : public QGLWidget
{
Q_OBJECT
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    ModelViewWidget(QWidget *parent);
    ~ModelViewWidget();

    void setModel(cmod::Model* model, const QString& modelDir);
    cmod::Model* model() const
    {
        return m_model;
    }

    void resetCamera();

    enum RenderStyle
    {
        NormalStyle,
        WireFrameStyle,
    };

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent* event);

    void select(const Eigen::Vector2f& point);
    QSet<cmod::Mesh::PrimitiveGroup*> selection()
    {
        return m_selection;
    }

    void setRenderStyle(RenderStyle style);
    RenderStyle renderStyle() const
    {
        return m_renderStyle;
    }

    Eigen::Transform3d cameraTransform() const;

    void setMaterial(unsigned int index, const cmod::Material& material);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:
    void renderModel(cmod::Model* model);
    void renderSelection(cmod::Model* model);
    void bindMaterial(const cmod::Material* material);

private:
    cmod::Model* m_model;
    double m_modelBoundingRadius;
    Eigen::Vector3d m_cameraPosition;
    Eigen::Quaterniond m_cameraOrientation;
    QPoint m_lastMousePosition;
    QPoint m_mouseDownPosition;
    RenderStyle m_renderStyle;

    MaterialLibrary* m_materialLibrary;

    QSet<cmod::Mesh::PrimitiveGroup*> m_selection;
};

#endif // _CMODVIEW_MODEL_VIEW_WIDGET_H_
