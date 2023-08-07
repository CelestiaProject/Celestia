// cmoddview - An application for previewing cmod and other 3D file formats
// supported by Celestia.
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <celengine/glsupport.h>

#include <QColor>
#include <QHash>
#include <QList>
#include <QMouseEvent>
#include <QObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QWheelEvent>
#include <QWidget>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celmodel/material.h>
#include <celmodel/mesh.h>


namespace cmod
{
class Model;
}

class FramebufferObject;
class GLProgram;

namespace cmodview
{

class MaterialLibrary;

class LightingEnvironment
{
public:
    LightingEnvironment() : lightCount(0), shadowCount(0) {}

    unsigned int lightCount;
    unsigned int shadowCount;
};

class ShaderKey
{
public:
    ShaderKey() : m_info(0u) {}

    enum {
        LightCountMask       = 0x0000f,
        SpecularMask         = 0x00010,
        DiffuseMapMask       = 0x00100,
        SpecularMapMask      = 0x00200,
        NormalMapMask        = 0x00400,
        EmissiveMapMask      = 0x00800,
        AnyMapMask           = 0x00f00,
        CompressedNormalMapMask = 0x01000,
        ShadowCountMask      = 0xf0000,
    };

    static ShaderKey Create(const cmod::Material* material,
                            const LightingEnvironment* lighting,
                            const cmod::VertexDescription* vertexDesc);

    unsigned int hash() const
    {
        return m_info;
    }

    bool operator==(const ShaderKey& other) const
    {
        return m_info == other.m_info;
    }

    bool hasSpecular() const
    {
        return (m_info & SpecularMask) != 0;
    }

    bool hasMaps() const
    {
        return (m_info & AnyMapMask) != 0;
    }

    bool hasDiffuseMap() const
    {
        return (m_info & DiffuseMapMask) != 0;
    }

    bool hasSpecularMap() const
    {
        return (m_info & SpecularMapMask) != 0;
    }

    bool hasEmissiveMap() const
    {
        return (m_info & EmissiveMapMask) != 0;
    }

    bool hasNormalMap() const
    {
        return (m_info & NormalMapMask) != 0;
    }

    bool hasCompressedNormalMap() const
    {
        return (m_info & CompressedNormalMapMask) != 0;
    }

    unsigned int lightSourceCount() const
    {
        return m_info & LightCountMask;
    }

    unsigned int shadowCount() const
    {
        return (m_info & ShadowCountMask) >> 16;
    }

private:
    ShaderKey(unsigned int info) : m_info(info) {}

private:
    unsigned int m_info;
};


inline unsigned int qHash(const ShaderKey& key)
{
    return ::qHash(key.hash());
}


class ModelViewWidget : public QOpenGLWidget
{
Q_OBJECT
public:
    ModelViewWidget(QWidget *parent);
    ~ModelViewWidget();

    void setModel(std::unique_ptr<cmod::Model>&& model, const QString& modelDir);
    cmod::Model* model() { return m_model.get(); }
    const cmod::Model* model() const { return m_model.get(); }

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
    QSet<const cmod::PrimitiveGroup*> selection()
    {
        return m_selection;
    }

    RenderStyle renderStyle() const
    {
        return m_renderStyle;
    }

    QColor backgroundColor() const
    {
        return m_backgroundColor;
    }

    Eigen::Affine3d cameraTransform() const;

    void setMaterial(unsigned int index, const cmod::Material& material);

    struct LightSource
    {
        Eigen::Vector3d direction;
        Eigen::Vector3f color;
        float intensity;
    };

    bool isLightingEnabled() const
    {
        return m_lightingEnabled;
    }

signals:
    void selectionChanged();

public slots:
    void setBackgroundColor(const QColor& color);
    void setRenderStyle(RenderStyle style);
    void setLighting(bool enable);
    void setAmbientLight(bool enable);
    void setShadows(bool enable);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:
    void renderModel(cmod::Model* model);
    void renderSelection(cmod::Model* model);
    void renderDepthOnly(cmod::Model* model);
    void renderShadow(unsigned int lightIndex);
    void bindMaterial(const cmod::Material* material,
                      const LightingEnvironment* lighting,
                      const cmod::VertexDescription* vertexDesc);

    void setupDefaultLightSources();
    GLProgram* createShader(const ShaderKey& shaderKey);

private:
    std::unique_ptr<cmod::Model> m_model;
    double m_modelBoundingRadius;
    Eigen::Vector3d m_cameraPosition;
    Eigen::Quaterniond m_cameraOrientation;
    QPoint m_lastMousePosition;
    QPoint m_mouseDownPosition;
    RenderStyle m_renderStyle;

    MaterialLibrary* m_materialLibrary;

    QSet<const cmod::PrimitiveGroup*> m_selection;
    QHash<ShaderKey, GLProgram*> m_shaderCache;

    QColor m_backgroundColor;

    QList<LightSource> m_lightSources;
    Eigen::Quaterniond m_lightOrientation;
    QList<FramebufferObject*> m_shadowBuffers;

    bool m_lightingEnabled;
    bool m_ambientLightEnabled;
    bool m_shadowsEnabled;
};

} // end namespace cmodview
