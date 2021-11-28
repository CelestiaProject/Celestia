// cmoddview - An application for previewing cmod and other 3D file formats
// supported by Celestia.
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <utility>

#include <QFileInfo>
#include <QIODevice>
#include <QMap>
#include <QPixmap>
#include <QTextStream>

#include <celmath/mathlib.h>
#include <celmodel/model.h>

#include "pathmanager.h"

#include "glframebuffer.h"
#include "glshader.h"
#include "glsupport.h"
#include "modelviewwidget.h"


#define DEBUG_SHADOWS 0

namespace
{

constexpr float VIEWPORT_FOV = 45.0;

constexpr int ShadowBufferSize = 1024;
constexpr int ShadowSampleKernelWidth = 2;


enum {
    TangentAttributeIndex = 6,
    PointSizeAttributeIndex = 7,
};


inline QString toQString(const wchar_t *s)
{
    return QString::fromWCharArray(s);
}


inline QString toQString(const char *s)
{
    return QString::fromLocal8Bit(s);
}


// Calculate the matrix used to render the model from the
// perspective of the light.
Eigen::Matrix4f
directionalLightMatrix(const Eigen::Vector3f& lightDirection)
{
    Eigen::Vector3f viewDir = lightDirection;
    Eigen::Vector3f upDir = viewDir.unitOrthogonal();
    Eigen::Vector3f rightDir = upDir.cross(viewDir);
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.row(0).head(3) = rightDir;
    m.row(1).head(3) = upDir;
    m.row(2).head(3) = viewDir;

    return m;
}


Eigen::Matrix4f
parallelProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
    // Duplicates OpenGL's glOrtho() function
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.diagonal() = Eigen::Vector4f(2.0f / (right - left),
                                   2.0f / (top - bottom),
                                   -2.0f / (zFar - zNear),
                                   1.0f);
    m.col(3) = Eigen::Vector4f(-(right + left) / (right - left),
                               -(top + bottom) / (top - bottom),
                               -(zFar + zNear) / (zFar - zNear),
                               1.0f);

    return m;
}


Eigen::Matrix4f
shadowProjectionMatrix(float objectRadius)
{
    return parallelProjectionMatrix(-objectRadius, objectRadius,
                                    -objectRadius, objectRadius,
                                    -objectRadius, objectRadius);
}

} // end unnamed namespace


class MaterialLibrary
{
public:
    MaterialLibrary(QGLWidget* glWidget,
                    const QString& modelDirPath) :
       m_glWidget(glWidget),
       m_modelDirPath(modelDirPath)
    {
    }

    ~MaterialLibrary()
    {
        flush();
    }

    GLuint loadTexture(const QString& fileName)
    {
        QFileInfo info(fileName);
        if (!info.exists())
        {
            return 0;
        }

        QString ext = info.suffix().toLower();
        if (ext == "dds" || ext == "dxt5nm")
        {
            GLuint texId = m_glWidget->bindTexture(fileName);
            // Qt doesn't seem to enable mipmap filtering automatically
            // TODO: Check whether the texture has mipmaps:
            //    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            return texId;
        }


        QPixmap texturePixmap(fileName);

        // Mipmaps and linear filtering enabled by default.
#if QT_VERSION >= 0x040600
        return m_glWidget->bindTexture(texturePixmap, GL_TEXTURE_2D, GL_RGBA, QGLContext::MipmapBindOption | QGLContext::LinearFilteringBindOption);
#else
        return m_glWidget->bindTexture(texturePixmap, GL_TEXTURE_2D, GL_RGBA);
#endif
    }


    GLuint getTexture(const QString& resourceName)
    {
        if (m_textures.contains(resourceName))
        {
            return m_textures[resourceName];
        }
        else
        {
            if (m_glWidget)
            {
                GLuint texId = loadTexture(m_modelDirPath + "/" + resourceName);
                if (texId == 0)
                {
                    // Try textures/medres...
                    texId = loadTexture(m_modelDirPath + "/../textures/medres/" + resourceName);
                }

                std::cout << "Load " << resourceName.toStdString() << ", texId = " << texId << std::endl;
                m_textures[resourceName] = texId;
                return texId;
            }
        }

        return 0;
    }

    void flush()
    {
        foreach (GLuint texId, m_textures)
        {
            if (texId != 0 && m_glWidget)
            {
                m_glWidget->deleteTexture(texId);
            }
        }
    }

private:
    QGLWidget* m_glWidget;
    QString m_modelDirPath;
    QMap<QString, GLuint> m_textures;
};


ShaderKey
ShaderKey::Create(const cmod::Material* material, const LightingEnvironment* lighting, const cmod::VertexDescription* vertexDesc)
{
    // Compute the shader key for a particular material and lighting setup
    unsigned int info = 0;

    bool hasTangents = false;
    bool hasTexCoords = false;
    if (vertexDesc)
    {
        hasTangents = vertexDesc->getAttribute(cmod::VertexAttributeSemantic::Tangent).format == cmod::VertexAttributeFormat::Float3;
        hasTexCoords = vertexDesc->getAttribute(cmod::VertexAttributeSemantic::Texture0).format == cmod::VertexAttributeFormat::Float2;
    }

    // Bits 0-3 are the number of light sources
    info |= lighting->lightCount;

    // Bits 16-19 are the number of shadows (always less than or equal to the
    // light source count.)
    info |= lighting->shadowCount << 16;

    // Bit 4 is set if specular lighting is enabled
    if (material->specular.red()   != 0.0f ||
        material->specular.green() != 0.0f ||
        material->specular.blue()  != 0.0f)
    {
        info |= SpecularMask;
    }

    // Bits 8-15 are texture map info
    if (hasTexCoords)
    {
        if (material->getMap(cmod::TextureSemantic::DiffuseMap) != InvalidResource)
        {
            info |= DiffuseMapMask;
        }

        if (material->getMap(cmod::TextureSemantic::SpecularMap) != InvalidResource)
        {
            info |= SpecularMapMask;
        }

        if (material->getMap(cmod::TextureSemantic::NormalMap) != InvalidResource)
        {
            info |= NormalMapMask;
        }

        if (material->getMap(cmod::TextureSemantic::EmissiveMap) != InvalidResource)
        {
            info |= EmissiveMapMask;
        }

        // Bit 16 is set if the normal map is compressed
        if (material->getMap(cmod::TextureSemantic::NormalMap) != InvalidResource && hasTangents)
        {
            if (GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::NormalMap)).extension() == ".dxt5nm")
            {
                info |= CompressedNormalMapMask;
            }
        }
    }

    return {info};
}


ModelViewWidget::ModelViewWidget(QWidget *parent) :
   QGLWidget(parent),
   m_model(nullptr),
   m_modelBoundingRadius(1.0),
   m_cameraPosition(Eigen::Vector3d::Zero()),
   m_cameraOrientation(Eigen::Quaterniond::Identity()),
   m_renderStyle(NormalStyle),
   m_renderPath(FixedFunctionPath),
   m_materialLibrary(nullptr),
   m_lightOrientation(Eigen::Quaterniond::Identity()),
   m_lightingEnabled(true),
   m_ambientLightEnabled(true),
   m_shadowsEnabled(false)
{
    setupDefaultLightSources();
}


ModelViewWidget::~ModelViewWidget()
{
    delete m_materialLibrary;
}


void
ModelViewWidget::setModel(std::unique_ptr<cmod::Model>&& model, const QString& modelDirPath)
{
    m_model = std::move(model);
    delete m_materialLibrary;

    m_materialLibrary = new MaterialLibrary(this, modelDirPath);

    m_selection.clear();

    // Load materials
    if (m_model != nullptr)
    {
        for (unsigned int i = 0; i < m_model->getMaterialCount(); ++i)
        {
            const cmod::Material* material = m_model->getMaterial(i);
            if (material->getMap(cmod::TextureSemantic::DiffuseMap) != InvalidResource)
            {
                m_materialLibrary->getTexture(
                    toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::DiffuseMap)).c_str()));
            }
            if (material->getMap(cmod::TextureSemantic::NormalMap) != InvalidResource)
            {
                m_materialLibrary->getTexture(
                    toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::NormalMap)).c_str()));
            }
            if (material->getMap(cmod::TextureSemantic::SpecularMap) != InvalidResource)
            {
                m_materialLibrary->getTexture(
                    toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::SpecularMap)).c_str()));
            }
            if (material->getMap(cmod::TextureSemantic::EmissiveMap) != InvalidResource)
            {
                m_materialLibrary->getTexture(
                    toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::EmissiveMap)).c_str()));
            }
        }
    }

    update();

    emit selectionChanged();
}


void
ModelViewWidget::resetCamera()
{
    Eigen::AlignedBox<float, 3> bbox;
    if (m_model != nullptr)
    {
        for (unsigned int i = 0; i < m_model->getMeshCount(); ++i)
        {
            bbox.extend(m_model->getMesh(i)->getBoundingBox());
        }
    }

    m_modelBoundingRadius = std::max(bbox.max().norm(), bbox.min().norm());
    m_cameraPosition = m_modelBoundingRadius * Eigen::Vector3d::UnitZ() * 2.0;
    m_cameraOrientation = Eigen::Quaterniond::Identity();
}


void
ModelViewWidget::setRenderStyle(RenderStyle style)
{
    if (style != m_renderStyle)
    {
        m_renderStyle = style;
        update();
    }
}


void
ModelViewWidget::setRenderPath(RenderPath path)
{
    if (path != m_renderPath)
    {
        m_renderPath = path;
        update();
    }
}


void
ModelViewWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePosition = event->pos();
    m_mouseDownPosition = event->pos();
}


void
ModelViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
    int moveDistance = (event->pos() - m_mouseDownPosition).manhattanLength();
    if (moveDistance < 3)
    {
        float x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width()) * 2.0f - 1.0f;
        float y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height()) * -2.0f + 1.0f;
        select(Eigen::Vector2f(x, y));
    }
}


void
ModelViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Left drag rotates the camera
    // Right drag or Alt+Left drag rotates the lights
    bool rotateCamera = false;
    bool rotateLights = false;

    if ((event->buttons() & Qt::LeftButton) != 0)
    {
        if ((event->modifiers() & Qt::AltModifier) != 0)
        {
            rotateLights = true;
        }
        else
        {
            rotateCamera = true;
        }
    }
    else if ((event->buttons() & Qt::RightButton) != 0)
    {
        rotateLights = true;
    }

    int dx = event->x() - m_lastMousePosition.x();
    int dy = event->y() - m_lastMousePosition.y();

    double xrotation = static_cast<double>(dy) / 100.0;
    double yrotation = static_cast<double>(dx) / 100.0;
    Eigen::Quaterniond q = Eigen::AngleAxis<double>(-xrotation, Eigen::Vector3d::UnitX()) *
                           Eigen::AngleAxis<double>(-yrotation, Eigen::Vector3d::UnitY());

    if (rotateLights)
    {
        Eigen::Quaterniond r = m_lightOrientation * q * m_lightOrientation.conjugate();
        r.normalize();
        m_lightOrientation = r * m_lightOrientation;
    }
    else if (rotateCamera)
    {
        Eigen::Quaterniond r = m_cameraOrientation * q * m_cameraOrientation.conjugate();
        r.normalize();  // guard against accumulating rounding errors

        m_cameraPosition    = r * m_cameraPosition;
        m_cameraOrientation = r * m_cameraOrientation;
    }

    m_lastMousePosition = event->pos();

    update();
}


void
ModelViewWidget::wheelEvent(QWheelEvent* event)
{
    QPoint numDegrees = event->angleDelta();
    if (numDegrees.isNull() || numDegrees.y() == 0)
        return;

    // Mouse wheel controls camera dolly
#if LINEAR_DOLLY
    double adjust = m_modelBoundingRadius * numDegrees.y() / 1000.0;
    double newDistance = m_cameraPosition.norm() + adjust;
    m_cameraPosition = m_cameraPosition.normalized() * newDistance;
#else
    double adjust = std::pow(2.0, numDegrees.y() / 1000.0);
    double newDistance = m_cameraPosition.norm() * adjust;
    m_cameraPosition = m_cameraPosition.normalized() * newDistance;
#endif

    update();
}


void
ModelViewWidget::select(const Eigen::Vector2f& viewportPoint)
{
    if (!m_model)
    {
        return;
    }

    float aspectRatio = static_cast<float>(size().width()) / static_cast<float>(size().height());
    auto fovRad = celmath::degToRad(VIEWPORT_FOV);
    float h = std::tan(fovRad / 2.0f);
    Eigen::Vector3d direction(h * aspectRatio * viewportPoint.x(), h * viewportPoint.y(), -1.0f);
    direction.normalize();
    Eigen::Vector3d origin = Eigen::Vector3d::Zero();
    Eigen::Affine3d camera(cameraTransform().inverse());

    cmod::Mesh::PickResult pickResult;
    bool hit = m_model->pick(camera * origin, camera.linear() * direction, &pickResult);
    if (hit)
    {
        m_selection.clear();
        m_selection.insert(pickResult.group);
        update();
    }
    else
    {
        m_selection.clear();
        update();
    }

    emit selectionChanged();
}


Eigen::Affine3d
ModelViewWidget::cameraTransform() const
{
    Eigen::Affine3d t(m_cameraOrientation.conjugate());
    t.translate(-m_cameraPosition);
    return t;
}


void
ModelViewWidget::setMaterial(unsigned int index, const cmod::Material& material)
{
    if (!m_model || index >= m_model->getMaterialCount())
    {
        return;
    }

    // Copy material parameters
    m_model->setMaterial(index, material.clone());
    update();
}

void
ModelViewWidget::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    update();
}


void
ModelViewWidget::initializeGL()
{
    celestia::gl::init();
    emit contextCreated();
}


void
ModelViewWidget::paintGL()
{
    // Generate the shadow buffers for each light source
    if (m_shadowsEnabled && m_shadowBuffers.size() > 0)
    {
        cmod::Material defaultMaterial;
        defaultMaterial.diffuse = cmod::Color(1.0f, 1.0f, 1.0f);
        LightingEnvironment lightingOff;
        bindMaterial(&defaultMaterial, &lightingOff, nullptr);
        glEnable(GL_CULL_FACE);

        for (int lightIndex = 0; lightIndex < m_lightSources.size(); ++lightIndex)
        {
            GLFrameBufferObject* shadowBuffer = m_shadowBuffers[lightIndex];
            if (shadowBuffer && shadowBuffer->isValid())
            {
                renderShadow(lightIndex);
            }
        }
        glViewport(0, 0, width(), height());
    }

    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), 0.0f);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    double distanceToOrigin = m_cameraPosition.norm();
    double nearDistance = std::max(m_modelBoundingRadius * 0.001, distanceToOrigin - m_modelBoundingRadius);
    double farDistance = m_modelBoundingRadius + distanceToOrigin;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspectRatio = (double) size().width() / (double) size().height();
    gluPerspective(VIEWPORT_FOV, aspectRatio, nearDistance, farDistance);

    glEnable(GL_LIGHTING);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    float ambientLightLevel = 0.0f;
    if (m_ambientLightEnabled)
    {
        ambientLightLevel = 0.2f;
    }
    Eigen::Vector4f ambientLight = Eigen::Vector4f::Constant(ambientLightLevel);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight.data());
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);

    for (unsigned int i = 0; i < 8; i++)
    {
        glDisable(GL_LIGHT0 + i);
    }

    unsigned int lightIndex = 0;
    foreach (LightSource lightSource, m_lightSources)
    {
        GLenum glLight = GL_LIGHT0 + lightIndex;
        lightIndex++;

        Eigen::Vector3d direction = m_lightOrientation * lightSource.direction;
        Eigen::Vector4f lightColor = Eigen::Vector4f::Zero();
        lightColor.head(3) = lightSource.color * lightSource.intensity;
        Eigen::Vector4f lightPosition = Eigen::Vector4f::Zero();
        lightPosition.head(3) = direction.cast<float>();

        glEnable(glLight);
        glLightfv(glLight, GL_POSITION, lightPosition.data());
        glLightfv(glLight, GL_DIFFUSE,  lightColor.data());
        glLightfv(glLight, GL_SPECULAR, lightColor.data());
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(cameraTransform().data());

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (m_model)
    {
        renderModel(m_model.get());
        if (!m_selection.isEmpty())
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-0.0f, -1.0f);
            renderSelection(m_model.get());
        }
    }

#if DEBUG_SHADOWS
    // Debugging for shadows: show the first shadow buffer in the corner of the
    // viewport.
    if (m_shadowsEnabled && m_shadowBuffers.size() > 0)
    {
        GLFrameBufferObject* shadowBuffer = m_shadowBuffers[0];
        if (shadowBuffer && shadowBuffer->isValid())
        {
            glDisable(GL_DEPTH_TEST);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluOrtho2D(0, width(), 0, height());
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glDisable(GL_LIGHTING);
            glUseProgram(0);
            glColor4f(1, 1, 1, 1);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, shadowBuffer->depthTexture());

            // Disable texture compare temporarily--we just want to see the
            // stored depth values.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glBegin(GL_QUADS);
            float side = 300.0f;
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(side, 0.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(side, side);
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(0.0f, side);
            glEnd();

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        }
    }
#endif

    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR)
    {
        std::cout << "OpenGL error: " << gluErrorString(errorCode) << std::endl;
    }
}


void
ModelViewWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}


static GLenum GLComponentTypes[static_cast<std::size_t>(cmod::VertexAttributeFormat::FormatMax)] =
{
     GL_FLOAT,          // Float1
     GL_FLOAT,          // Float2
     GL_FLOAT,          // Float3
     GL_FLOAT,          // Float4,
     GL_UNSIGNED_BYTE,  // UByte4
};

static int GLComponentCounts[static_cast<std::size_t>(cmod::VertexAttributeFormat::FormatMax)] =
{
     1,  // Float1
     2,  // Float2
     3,  // Float3
     4,  // Float4,
     4,  // UByte4
};


static void
setVertexArrays(const cmod::VertexDescription& desc, const cmod::VWord* vertexData)
{
    const cmod::VertexAttribute& position  = desc.getAttribute(cmod::VertexAttributeSemantic::Position);
    const cmod::VertexAttribute& normal    = desc.getAttribute(cmod::VertexAttributeSemantic::Normal);
    const cmod::VertexAttribute& color0    = desc.getAttribute(cmod::VertexAttributeSemantic::Color0);
    const cmod::VertexAttribute& texCoord0 = desc.getAttribute(cmod::VertexAttributeSemantic::Texture0);
    const cmod::VertexAttribute& tangent   = desc.getAttribute(cmod::VertexAttributeSemantic::Tangent);

    // Can't render anything unless we have positions
    if (position.format != cmod::VertexAttributeFormat::Float3)
        return;

    // Set up the vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, desc.strideBytes, vertexData + position.offsetWords);

    // Set up the normal array
    switch (normal.format)
    {
    case cmod::VertexAttributeFormat::Float3:
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GLComponentTypes[static_cast<std::size_t>(normal.format)],
                        desc.strideBytes, vertexData + normal.offsetWords);
        break;
    default:
        glDisableClientState(GL_NORMAL_ARRAY);
        break;
    }

    // Set up the color array
    switch (color0.format)
    {
    case cmod::VertexAttributeFormat::Float3:
    case cmod::VertexAttributeFormat::Float4:
    case cmod::VertexAttributeFormat::UByte4:
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GLComponentCounts[static_cast<std::size_t>(color0.format)],
                       GLComponentTypes[static_cast<std::size_t>(color0.format)],
                       desc.strideBytes, vertexData + color0.offsetWords);
        break;
    default:
        glDisableClientState(GL_COLOR_ARRAY);
        break;
    }

    // Set up the texture coordinate array
    switch (texCoord0.format)
    {
    case cmod::VertexAttributeFormat::Float1:
    case cmod::VertexAttributeFormat::Float2:
    case cmod::VertexAttributeFormat::Float3:
    case cmod::VertexAttributeFormat::Float4:
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(GLComponentCounts[static_cast<std::size_t>(texCoord0.format)],
                          GLComponentTypes[static_cast<std::size_t>(texCoord0.format)],
                          desc.strideBytes, vertexData + texCoord0.offsetWords);
        break;
    default:
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        break;
    }

    switch (tangent.format)
    {
    case cmod::VertexAttributeFormat::Float3:
        glEnableVertexAttribArrayARB(TangentAttributeIndex);
        glVertexAttribPointerARB(TangentAttributeIndex,
                                      GLComponentCounts[static_cast<std::size_t>(tangent.format)],
                                      GLComponentTypes[static_cast<std::size_t>(tangent.format)],
                                      GL_FALSE,
                                      desc.strideBytes,
                                      vertexData + tangent.offsetWords);
        break;
    default:
        glDisableVertexAttribArrayARB(TangentAttributeIndex);
        break;
    }

}


// Set just the vertex pointer
void
setVertexPointer(const cmod::VertexDescription& desc, const cmod::VWord* vertexData)
{
    const cmod::VertexAttribute& position  = desc.getAttribute(cmod::VertexAttributeSemantic::Position);

    // Can't render anything unless we have positions
    if (position.format != cmod::VertexAttributeFormat::Float3)
        return;

    // Set up the vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, desc.strideBytes,
                    vertexData + position.offsetWords);

    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArrayARB(TangentAttributeIndex);
}


static GLenum
getGLMode(cmod::PrimitiveGroupType primitive)
{
    switch (primitive)
    {
    case cmod::PrimitiveGroupType::TriList:
        return GL_TRIANGLES;
    case cmod::PrimitiveGroupType::TriStrip:
        return GL_TRIANGLE_STRIP;
    case cmod::PrimitiveGroupType::TriFan:
        return GL_TRIANGLE_FAN;
    case cmod::PrimitiveGroupType::LineList:
        return GL_LINES;
    case cmod::PrimitiveGroupType::LineStrip:
        return GL_LINE_STRIP;
    case cmod::PrimitiveGroupType::PointList:
        return GL_POINTS;
    default:
        return GL_POINTS;
    }
}


static bool gl2Fail = false;


void
ModelViewWidget::setLighting(bool enable)
{
    m_lightingEnabled = enable;
    if (m_lightingEnabled)
    {
        glEnable(GL_LIGHTING);
    }
    else
    {
        glDisable(GL_LIGHTING);
    }
}


void
ModelViewWidget::setAmbientLight(bool enable)
{
    if (enable != m_ambientLightEnabled)
    {
        m_ambientLightEnabled = enable;
        update();
    }
}


void
ModelViewWidget::setShadows(bool enable)
{
    if (!celestia::gl::EXT_framebuffer_object)
    {
        return;
    }

    if (enable != m_shadowsEnabled)
    {
        m_shadowsEnabled = enable;
        if (m_shadowsEnabled && m_shadowBuffers.size() < 2)
        {
            auto* fb0 = new GLFrameBufferObject(ShadowBufferSize, ShadowBufferSize, GLFrameBufferObject::DepthAttachment);
            auto* fb1 = new GLFrameBufferObject(ShadowBufferSize, ShadowBufferSize, GLFrameBufferObject::DepthAttachment);
            m_shadowBuffers << fb0 << fb1;
            if (!fb0->isValid() || !fb1->isValid())
            {
                qWarning("Error creating shadow buffers.");
            }
        }

        update();
    }
}


void
ModelViewWidget::bindMaterial(const cmod::Material* material,
                              const LightingEnvironment* lighting,
                              const cmod::VertexDescription* vertexDesc)
{
    GLShaderProgram* shader = nullptr;

    ShaderKey shaderKey;
    if (renderPath() == OpenGL2Path && !gl2Fail)
    {
        shaderKey = ShaderKey::Create(material, lighting, vertexDesc);

        // Lookup the shader in the shader cache
        shader = m_shaderCache.value(shaderKey);
        if (!shader)
        {
            shader = createShader(shaderKey);
            if (shader)
            {
                m_shaderCache.insert(shaderKey, shader);
            }
            else
            {
                gl2Fail = true;
            }
        }
    }

    if (shader)
    {
        shader->bind();

        shader->setUniformValue("modelView", cameraTransform().matrix().cast<float>());

        shader->setUniformValue("diffuseColor", Eigen::Vector3f(material->diffuse.red(), material->diffuse.green(), material->diffuse.blue()));
        shader->setUniformValue("specularColor", Eigen::Vector3f(material->specular.red(), material->specular.green(), material->specular.blue()));
        shader->setUniformValue("opacity", material->opacity);
        shader->setUniformValue("specularPower", material->specularPower);

        if (shaderKey.hasDiffuseMap())
        {
            GLuint diffuseMapId = m_materialLibrary->getTexture(
                toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::DiffuseMap)).c_str()));
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, diffuseMapId);
            shader->setSampler("diffuseMap", 0);
        }

        if (shaderKey.hasNormalMap())
        {
            GLuint normalMapId = m_materialLibrary->getTexture(
                toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::NormalMap)).c_str()));
            glActiveTexture(GL_TEXTURE1);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, normalMapId);
            shader->setSampler("normalMap", 1);
            glActiveTexture(GL_TEXTURE0);
        }

        if (shaderKey.hasSpecularMap())
        {
            GLuint specularMapId = m_materialLibrary->getTexture(
                toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::SpecularMap)).c_str()));
            glActiveTexture(GL_TEXTURE2);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, specularMapId);
            shader->setSampler("specularMap", 2);
            glActiveTexture(GL_TEXTURE0);
        }

        if (shaderKey.hasEmissiveMap())
        {
            GLuint emissiveMapId = m_materialLibrary->getTexture(
                toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::EmissiveMap)).c_str()));
            glActiveTexture(GL_TEXTURE3);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, emissiveMapId);
            shader->setSampler("emissiveMap", 3);
            glActiveTexture(GL_TEXTURE0);
        }

        unsigned int lightIndex = 0;
        Eigen::Matrix3d lightMatrix = m_lightOrientation.toRotationMatrix();

        Eigen::Vector3f lightDirections[8];
        Eigen::Vector3f lightColors[8];

        foreach (LightSource lightSource, m_lightSources)
        {
            Eigen::Vector3d direction = lightMatrix * lightSource.direction;
            Eigen::Vector3f color = lightSource.color * lightSource.intensity;

            lightDirections[lightIndex] = direction.cast<float>();
            lightColors[lightIndex] = color;

            lightIndex++;
        }

        if (m_lightSources.size() > 0)
        {
            shader->setUniformValueArray("lightDirection", lightDirections, m_lightSources.size());
            shader->setUniformValueArray("lightColor", lightColors, m_lightSources.size());
        }

        float ambientLightLevel = 0.0f;
        if (m_ambientLightEnabled)
        {
            ambientLightLevel = 0.2f;
        }
        shader->setUniformValue("ambientLightColor", Eigen::Vector3f::Constant(ambientLightLevel));

        // Get the eye position in model space
        Eigen::Vector4f eyePosition = cameraTransform().inverse().cast<float>() * Eigen::Vector4f::UnitW();
        shader->setUniformValue("eyePosition", eyePosition.head(3));

        // Set all shadow related values
        if (shaderKey.shadowCount() > 0)
        {
            const unsigned int MaxShadows = 8;

            // Assign the shadow textures; arrays of samplers are not allowed, so
            // we need to use this loop to set them.
            for (unsigned int i = 0; i < shaderKey.shadowCount(); ++i)
            {
                char samplerName[64];
                sprintf(samplerName, "shadowTexture%d", i);

                glActiveTexture(GL_TEXTURE4 + i);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, m_shadowBuffers[i]->depthTexture());
                shader->setSampler(samplerName, 4 + i);
                glActiveTexture(GL_TEXTURE0);
            }

            Eigen::Matrix4f shadowMatrixes[MaxShadows];
            Eigen::Matrix4f bias = Eigen::Matrix4f::Zero();
            bias.diagonal() = Eigen::Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
            bias.col(3) = Eigen::Vector4f(0.5f, 0.5f, 0.5f, 1.0f);

            for (unsigned int i = 0; i < shaderKey.shadowCount(); ++i)
            {
                Eigen::Matrix4f modelView = directionalLightMatrix(lightDirections[i]);
                Eigen::Matrix4f projection = shadowProjectionMatrix(m_modelBoundingRadius);
                shadowMatrixes[i] = bias * projection * modelView;

                // TESTING ONLY:
                //shadowMatrixes[i] = bias * (1.0f / m_modelBoundingRadius);
                //shadowMatrixes[i].col(3).w() = 1.0f;
            }

            shader->setUniformValueArray("shadowMatrix", shadowMatrixes, shaderKey.shadowCount());
        }
    }
    else
    {
        if (GLShaderProgram::hasOpenGLShaderPrograms())
        {
            glUseProgram(0);
        }

        Eigen::Vector4f diffuse(material->diffuse.red(), material->diffuse.green(), material->diffuse.blue(), material->opacity);
        Eigen::Vector4f specular(material->specular.red(), material->specular.green(), material->specular.blue(), 1.0f);
        Eigen::Vector4f emissive(material->emissive.red(), material->emissive.green(), material->emissive.blue(), 1.0f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse.data());
        glMaterialfv(GL_FRONT, GL_AMBIENT, diffuse.data());
        glColor4fv(diffuse.data());
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular.data());
        glMaterialfv(GL_FRONT, GL_SHININESS, &material->specularPower);
        glMaterialfv(GL_FRONT, GL_EMISSION, emissive.data());

        // Set up the diffuse (base) texture
        GLuint baseTexId = 0;
        if (material->getMap(cmod::TextureSemantic::DiffuseMap) != InvalidResource)
        {
            baseTexId = m_materialLibrary->getTexture(
                toQString(GetPathManager()->getSource(material->getMap(cmod::TextureSemantic::DiffuseMap)).c_str()));
        }

        if (baseTexId != 0)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, baseTexId);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }
    }

    if (material->opacity < 1.0f)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }
    else
    {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
}


void
ModelViewWidget::renderModel(cmod::Model* model)
{
    cmod::Material defaultMaterial;
    defaultMaterial.diffuse = cmod::Color(1.0f, 1.0f, 1.0f);

    glEnable(GL_CULL_FACE);
    if (m_renderStyle == WireFrameStyle)
    {
        glPolygonMode(GL_FRONT, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT, GL_FILL);
    }

    // Disable all texture units
    for (unsigned int i = 0; i < 8; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);

    enum {
        Opaque = 0,
        Translucent = 1,
    };

    LightingEnvironment lighting;
    lighting.lightCount = m_lightingEnabled ? m_lightSources.size() : 0;
    lighting.shadowCount = m_shadowsEnabled ? std::min(lighting.lightCount, (unsigned int) m_shadowBuffers.size()) : 0;

    // Render opaque objects first, translucent objects last
    for (int pass = 0; pass < 2; ++pass)
    {
        // Render all meshes
        for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
        {
            const cmod::Mesh* mesh = model->getMesh(meshIndex);

            setVertexArrays(mesh->getVertexDescription(), mesh->getVertexData());
            if (mesh->getVertexDescription().getAttribute(cmod::VertexAttributeSemantic::Normal).format == cmod::VertexAttributeFormat::Float3)
            {
                setLighting(true);
            }
            else
            {
                setLighting(false);
            }

            for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
            {
                const cmod::PrimitiveGroup* group = mesh->getGroup(groupIndex);
                const cmod::Material* material = &defaultMaterial;
                if (group->materialIndex < model->getMaterialCount())
                {
                     material = model->getMaterial(group->materialIndex);
                }
                bool isOpaque = material->opacity == 1.0f;

                // Only draw opaque objects on the first pass, and only draw translucent
                // ones on the second pass.
                if ((isOpaque && pass == Opaque) || (!isOpaque && pass == Translucent))
                {
                    bindMaterial(material, &lighting, &mesh->getVertexDescription());

                    GLenum primitiveMode = getGLMode(group->prim);
                    glDrawElements(primitiveMode, group->indices.size(), GL_UNSIGNED_INT, group->indices.data());
                }
            }
        }
    }

    bindMaterial(&defaultMaterial, &lighting, nullptr);
}


void
ModelViewWidget::renderSelection(cmod::Model* model)
{
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT, GL_LINE);
    setLighting(false);
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    if (renderPath() == OpenGL2Path)
    {
        cmod::Material selectionMaterial;
        selectionMaterial.diffuse = cmod::Color(0.0f, 1.0f, 0.0f);
        selectionMaterial.opacity = 0.5f;

        LightingEnvironment lightsOff;
        bindMaterial(&selectionMaterial, &lightsOff, nullptr);
    }

    for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
    {
        cmod::Mesh* mesh = model->getMesh(meshIndex);
        setVertexPointer(mesh->getVertexDescription(), mesh->getVertexData());

        for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
        {
            cmod::PrimitiveGroup* group = mesh->getGroup(groupIndex);
            if (m_selection.contains(group))
            {
                GLenum primitiveMode = getGLMode(group->prim);
                glDrawElements(primitiveMode, group->indices.size(), GL_UNSIGNED_INT, group->indices.data());
            }
        }
    }

    setLighting(true);
    glPolygonMode(GL_FRONT, GL_FILL);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}


void
ModelViewWidget::renderDepthOnly(cmod::Model* model)
{
    glDepthMask(GL_TRUE);

    for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
    {
        cmod::Mesh* mesh = model->getMesh(meshIndex);
        setVertexPointer(mesh->getVertexDescription(), mesh->getVertexData());

        for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
        {
            cmod::PrimitiveGroup* group = mesh->getGroup(groupIndex);
            GLenum primitiveMode = getGLMode(group->prim);
            glDrawElements(primitiveMode, group->indices.size(), GL_UNSIGNED_INT, group->indices.data());
        }
    }
}


void
ModelViewWidget::setupDefaultLightSources()
{
    m_lightSources.clear();

    LightSource light1;
    light1.color = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
    light1.intensity = 1.0f;
    light1.direction = Eigen::Vector3d(1.0, 1.0, 5.0).normalized();

    LightSource light2;
    light2.color = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
    light2.intensity = 1.0f;
    light2.direction = Eigen::Vector3d(3.0, -3.0, -1.0).normalized();

    m_lightSources << light1;// << light2;
}


GLShaderProgram*
ModelViewWidget::createShader(const ShaderKey& shaderKey)
{
    QString vertexShaderSource;
    QString fragmentShaderSource;

    QTextStream vout(&vertexShaderSource, QIODevice::WriteOnly);
    QTextStream fout(&fragmentShaderSource, QIODevice::WriteOnly);

    if (shaderKey.lightSourceCount() == 0)
    {
        /*** Vertex shader ***/
        vout << "void main(void)\n";
        vout << "{\n";
        vout << "    gl_Position = ftransform();\n";
        vout << "}";
        /*** End vertex shader ***/


        /*** Fragment shader ***/
        fout << "uniform vec3 diffuseColor;\n";
        fout << "uniform float opacity;\n";
        fout << "void main(void)\n";
        fout << "{\n";
        fout << "   gl_FragColor = vec4(diffuseColor, opacity);\n";
        fout << "}\n";
        /*** End fragment shader ***/
    }
    else
    {
        vout << "varying vec3 normal;\n";
        fout << "varying vec3 normal;\n";
        vout << "varying vec3 position;\n";
        fout << "varying vec3 position;\n";
        if (shaderKey.hasMaps())
        {
            vout << "varying vec2 texCoord;\n";
            fout << "varying vec2 texCoord;\n";
        }

        if (shaderKey.hasNormalMap())
        {
            vout << "attribute vec3 tangentAtt;\n";
            vout << "varying vec3 tangent;\n";
            fout << "varying vec3 tangent;\n";
        }

        /*** Vertex shader ***/
        //vout << "attribute vec3 position;\n";
        //vout << "attribute vec3 normal;\n";
        vout << "uniform mat4 modelView;\n";

        // Shadow matrices and shadow texture coordinates
        if (shaderKey.shadowCount() > 0)
        {
            vout << "uniform mat4 shadowMatrix[" << shaderKey.shadowCount() << "];\n";
            vout << "varying vec4 shadowCoord[" << shaderKey.shadowCount() << "];\n";
            fout << "varying vec4 shadowCoord[" << shaderKey.shadowCount() << "];\n";
        }

        vout << "void main(void)\n";
        vout << "{\n";
        vout << "    normal = gl_Normal;\n";
        vout << "    position = gl_Vertex.xyz;\n";
        if (shaderKey.hasMaps())
        {
            vout << "    texCoord = gl_MultiTexCoord0.xy;\n";
        }
        if (shaderKey.hasNormalMap())
        {
            vout << "    tangent = tangentAtt;\n";
        }

        // Compute shadow texture coordinates
        for (unsigned int i = 0; i < shaderKey.shadowCount(); ++i)
        {
            vout << "    shadowCoord[" << i << "] = shadowMatrix[" << i << "] * gl_Vertex;\n";
        }

        vout << "    gl_Position = ftransform();\n";
        //vout << "   gl_Position = modelView * gl_Vertex;\n";
        vout << "}";
        /*** End vertex shader ***/


        /*** Fragment shader ***/
        fout << "uniform vec3 eyePosition;\n";
        fout << "uniform vec3 lightDirection[" << shaderKey.lightSourceCount() << "];\n";
        fout << "uniform vec3 lightColor[" << shaderKey.lightSourceCount() << "];\n";
        fout << "uniform vec3 ambientLightColor;\n";
        fout << "uniform vec3 diffuseColor;\n";
        fout << "uniform vec3 specularColor;\n";
        fout << "uniform float specularPower;\n";
        fout << "uniform float opacity;\n";
        if (shaderKey.hasDiffuseMap())
        {
            fout << "uniform sampler2D diffuseMap;\n";
        }
        if (shaderKey.hasSpecularMap())
        {
            fout << "uniform sampler2D specularMap;\n";
        }
        if (shaderKey.hasEmissiveMap())
        {
            fout << "uniform sampler2D emissiveMap;\n";
        }
        if (shaderKey.hasNormalMap())
        {
            fout << "uniform sampler2D normalMap;\n";
        }

        for (unsigned int i = 0; i < shaderKey.shadowCount(); ++i)
        {
            fout << "uniform sampler2DShadow shadowTexture" << i << ";\n";
        }

        fout << "void main(void)\n";
        fout << "{\n";
        fout << "   vec3 baseColor = diffuseColor;\n";

        if (shaderKey.hasSpecular())
        {
            fout << "   vec3 specularLight = vec3(0.0);\n";
            fout << "   vec3 V = normalize(eyePosition - position);\n"; // view vector
        }

        if (shaderKey.hasDiffuseMap())
        {
            fout << "    baseColor *= texture2D(diffuseMap, texCoord).rgb;\n";
        }

        // Compute the surface normal N
        if (shaderKey.hasNormalMap())
        {
            // Fetch the normal from the normal map
            if (shaderKey.hasCompressedNormalMap())
            {
                // For compressed normal maps, we compute z from the x and y
                // components, guaranteeing that we have a unit normal.
                fout << "vec3 n;\n";
                fout << "n.xy = texture2D(normalMap, texCoord).ag * 2.0 - 1.0;\n";
                fout << "n.z = sqrt(1.0 - n.x * n.x - n.y * n.y);\n";
            }
            else
            {
                // NOTE: the extra normalize is not present in Celestia, but
                // probably should be. Without it, interpolation will produce
                // non-unit normals that incorrectly reduce the intensity of
                // specular highlights. The effects of non-unit normals is not
                // as noticeable with diffuse lighting.
                fout << "    vec3 n = normalize(texture2D(normalMap, texCoord).xyz * 2.0 - 1.0);";
            }

            // The normal map contains normals in 'tangent space.' Transform them
            // into model space.
            fout << "    vec3 N0 = normalize(normal);\n";
            fout << "    vec3 T = normalize(tangent);\n";
            fout << "    vec3 B = cross(T, N0);\n";
            fout << "    vec3 N = n.x * T + n.y * B + n.z * N0;\n";
        }
        else
        {
            fout << "   vec3 N = normalize(normal);\n";
        }

        fout << "   vec3 light = ambientLightColor;\n";
        //fout << "   for (int i = 0; i < " << shaderKey.lightSourceCount() << "; ++i)\n";
        for (unsigned int lightIndex = 0; lightIndex < shaderKey.lightSourceCount(); ++lightIndex)
        {
            fout << "   {\n";
            fout << "       float d = max(0.0, dot(lightDirection[" << lightIndex << "], N));\n";

            // Self shadowing term required for normal maps and specular materials
            if (shaderKey.hasNormalMap())
            {
                // IMPORTANT: Use the surface normal from the geometry, not the one
                // retrieved from the normal map.
                fout << "        float selfShadow = clamp(dot(lightDirection[" << lightIndex << "], N0) * 8.0, 0.0, 1.0);\n";
            }
            else if (shaderKey.hasSpecular())
            {
                fout << "        float selfShadow = clamp(d * 8.0, 0.0, 1.0);\n";
            }
            else
            {
                fout << "        float selfShadow = 1.0;\n";
            }

            if (shaderKey.shadowCount() > 0)
            {
                // Normal 2D texture
                //fout << "        float lightDistance = texture2D(shadowTexture" << lightIndex << ", shadowCoord[" << lightIndex << "].xy).z;\n";
                //fout << "        if (lightDistance < shadowCoord[" << lightIndex << "].z + 0.0005) selfShadow = 0.0;\n";

                // Depth texture
                // fout << "        selfShadow = shadow2D(shadowTexture" << lightIndex << ", shadowCoord[" << lightIndex << "].xyz + vec3(0.0, 0.0, 0.0005)).z;\n";

                // Box filter PCF with depth texture
                fout << "        float texelSize = " << 1.0f / (float) ShadowBufferSize << ";\n";
                fout << "        float s = 0.0;\n";
                float boxFilterWidth = (float) ShadowSampleKernelWidth - 1.0f;
                float firstSample = -boxFilterWidth / 2.0f;
                float lastSample = firstSample + boxFilterWidth;
                float sampleWeight = 1.0f / (float) (ShadowSampleKernelWidth * ShadowSampleKernelWidth);
                fout << "        for (float y = " << firstSample << "; y <= " << lastSample << "; y += 1.0)";
                fout << "            for (float x = " << firstSample << "; x <= " << lastSample << "; x += 1.0)";
                fout << "                s += shadow2D(shadowTexture" << lightIndex << ", shadowCoord[" << lightIndex << "].xyz + vec3(x * texelSize, y * texelSize, 0.0005)).z;\n";
                fout << "        selfShadow *= s * " << sampleWeight << ";\n";
            }

            fout << "       light += lightColor[" << lightIndex << "] * (d * selfShadow);\n";
            if (shaderKey.hasSpecular())
            {
                fout << "       vec3 H = normalize(lightDirection[" << lightIndex << "] + V);\n"; // half angle vector
                fout << "       float spec = pow(max(0.0, dot(H, N)), specularPower);\n";
                fout << "       if (d == 0.0) spec = 0.0;\n";
                fout << "       specularLight += lightColor[" << lightIndex << "] * (spec * selfShadow);\n";
            }
            fout << "   }\n";
        }

        fout << "   vec3 color = light * baseColor;\n";
        if (shaderKey.hasSpecular())
        {
            if (shaderKey.hasSpecularMap())
            {
                fout << "    color += specularLight * specularColor * texture2D(specularMap, texCoord).xyz;\n";
            }
            else
            {
                fout << "    color += specularLight * specularColor;\n";
            }
        }

        if (shaderKey.hasEmissiveMap())
        {
            fout << "    color += texture2D(emissiveMap, texCoord).xyz;\n";
        }

        fout << "   gl_FragColor = vec4(color, opacity);\n";

        fout << "}\n";
        /*** End fragment shader ***/
    }

    auto* glShader = new GLShaderProgram();
    auto* vertexShader = new GLVertexShader();
    if (!vertexShader->compile(vertexShaderSource.toStdString()))
    {
        qWarning("Vertex shader error: %s", vertexShader->log().c_str());
        std::cerr << vertexShaderSource.toStdString() << std::endl;
        delete vertexShader;
        delete glShader;
        return nullptr;
    }

    auto* fragmentShader = new GLFragmentShader();
    if (!fragmentShader->compile(fragmentShaderSource.toStdString()))
    {
        qWarning("Fragment shader error: %s", fragmentShader->log().c_str());
        std::cerr << fragmentShaderSource.toStdString() << std::endl;
        delete vertexShader;
        delete fragmentShader;
        delete glShader;
        return nullptr;
    }

    glShader->addVertexShader(vertexShader);
    glShader->addFragmentShader(fragmentShader);
    if (shaderKey.hasNormalMap())
    {
        glShader->bindAttributeLocation("tangentAtt", TangentAttributeIndex);
    }

    if (!glShader->link())
    {
        qWarning("Shader link error: %s", glShader->log().c_str());
        delete vertexShader;
        delete fragmentShader;
        delete glShader;
        return nullptr;
    }

    return glShader;
}


void
ModelViewWidget::renderShadow(unsigned int lightIndex)
{
    if ((int) lightIndex >= m_lightSources.size() ||
        (int) lightIndex >= m_shadowBuffers.size() ||
        !m_shadowBuffers[lightIndex])
    {
        return;
    }

    GLFrameBufferObject* shadowBuffer = m_shadowBuffers[lightIndex];
    Eigen::Vector3f lightDirection = (m_lightOrientation * m_lightSources[lightIndex].direction).cast<float>();

    shadowBuffer->bind();
    glViewport(0, 0, shadowBuffer->width(), shadowBuffer->height());

    // Write only to the depth buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glClear(GL_DEPTH_BUFFER_BIT);

    // Render backfaces only in order to reduce self-shadowing artifacts
    glCullFace(GL_FRONT);

    glUseProgram(0);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(shadowProjectionMatrix(m_modelBoundingRadius).data());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(directionalLightMatrix(lightDirection).data());

    renderDepthOnly(m_model.get());

    shadowBuffer->unbind();

    // Re-enable the color buffer and culling
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glCullFace(GL_BACK);
}

