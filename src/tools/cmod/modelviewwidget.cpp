// cmoddview - An application for previewing cmod and other 3D file formats
// supported by Celestia.
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <GL/glew.h>
#include "modelviewwidget.h"
#include <QtOpenGL>
#include <Eigen/LU>

using namespace cmod;
using namespace Eigen;

static const float VIEWPORT_FOV = 45.0;
static const double PI = 3.1415926535897932;

enum {
    TangentAttributeIndex = 6,
    PointSizeAttributeIndex = 7,
};


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
        else
        {
            QPixmap texturePixmap(fileName);
            return m_glWidget->bindTexture(texturePixmap, GL_TEXTURE_2D, GL_RGBA,
                                           QGLContext::LinearFilteringBindOption | QGLContext::MipmapBindOption);
        }
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

                std::cout << "Load " << resourceName.toAscii().data() << ", texId = " << texId << std::endl;
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


static unsigned int
computeShaderKey(const Material* material, unsigned int lightSourceCount)
{
    // Compute the shader key for a particular material and lighting setup

    // Bits 0-3 are the number of light sources
    unsigned int shaderKey = 0;
    shaderKey |= lightSourceCount;

    // Bit 4 is set if specular lighting is enabled
    if (material->specular.red() != 0.0f ||
        material->specular.green() != 0.0f ||
        material->specular.blue() != 0.0f)
    {
        shaderKey |= 0x10;
    }

    // Bits 8-15 are texture map info
    if (material->maps[Material::DiffuseMap])
    {
        shaderKey |= 0x100;
    }

    if (material->maps[Material::SpecularMap])
    {
        shaderKey |= 0x200;
    }

    if (material->maps[Material::NormalMap])
    {
        shaderKey |= 0x400;
    }

    if (material->maps[Material::EmissiveMap])
    {
        shaderKey |= 0x800;
    }

    return shaderKey;
}


ModelViewWidget::ModelViewWidget(QWidget *parent) :
   QGLWidget(parent),
   m_model(NULL),
   m_modelBoundingRadius(1.0),
   m_cameraPosition(Vector3d::Zero()),
   m_cameraOrientation(Quaterniond::Identity()),
   m_renderStyle(NormalStyle),
   m_renderPath(FixedFunctionPath),
   m_materialLibrary(NULL),
   m_lightOrientation(Quaterniond::Identity()),
   m_lightingEnabled(true),
   m_ambientLightEnabled(true)
{
    setupDefaultLightSources();
}


ModelViewWidget::~ModelViewWidget()
{
    delete m_materialLibrary;
    delete m_model;
}


void
ModelViewWidget::setModel(cmod::Model* model, const QString& modelDirPath)
{
    if (m_model && m_model != model)
    {
        delete m_model;
    }
    m_model = model;

    if (m_materialLibrary)
    {
        delete m_materialLibrary;
    }
    m_materialLibrary = new MaterialLibrary(this, modelDirPath);

    m_selection.clear();

    // Load materials
    if (m_model != NULL)
    {
        for (unsigned int i = 0; i < m_model->getMaterialCount(); ++i)
        {
            const Material* material = m_model->getMaterial(i);
            if (material->maps[Material::DiffuseMap])
            {
                m_materialLibrary->getTexture(QString(material->maps[Material::DiffuseMap]->source().c_str()));
            }
            if (material->maps[Material::NormalMap])
            {
                m_materialLibrary->getTexture(QString(material->maps[Material::NormalMap]->source().c_str()));
            }
            if (material->maps[Material::SpecularMap])
            {
                m_materialLibrary->getTexture(QString(material->maps[Material::SpecularMap]->source().c_str()));
            }
            if (material->maps[Material::EmissiveMap])
            {
                m_materialLibrary->getTexture(QString(material->maps[Material::EmissiveMap]->source().c_str()));
            }
        }
    }

    update();

    emit selectionChanged();
}


void
ModelViewWidget::resetCamera()
{
    AlignedBox<float, 3> bbox;
    if (m_model != NULL)
    {
        for (unsigned int i = 0; i < m_model->getMeshCount(); ++i)
        {
            bbox.extend(m_model->getMesh(i)->getBoundingBox());
        }
    }

    m_modelBoundingRadius = std::max(bbox.max().norm(), bbox.min().norm());
    m_cameraPosition = m_modelBoundingRadius * Vector3d::UnitZ() * 2.0;
    m_cameraOrientation = Quaterniond::Identity();
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
        float x = (float) event->pos().x() / (float) size().width() * 2.0f - 1.0f;
        float y = (float) event->pos().y() / (float) size().height() * -2.0f + 1.0f;
        select(Vector2f(x, y));
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

    double xrotation = (double) dy / 100.0;
    double yrotation = (double) dx / 100.0;
    Quaterniond q = AngleAxis<double>(-xrotation, Vector3d::UnitX()) *
                    AngleAxis<double>(-yrotation, Vector3d::UnitY());

    if (rotateLights)
    {
        Quaterniond r = m_lightOrientation * q * m_lightOrientation.conjugate();
        r.normalize();
        m_lightOrientation = r * m_lightOrientation;
    }
    else if (rotateCamera)
    {
        Quaterniond r = m_cameraOrientation * q * m_cameraOrientation.conjugate();
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
    if (event->orientation() != Qt::Vertical)
    {
        return;
    }

    // Mouse wheel controls camera dolly
#if LINEAR_DOLLY
    double adjust = m_modelBoundingRadius * event->delta() / 1000.0;
    double newDistance = m_cameraPosition.norm() + adjust;
    m_cameraPosition = m_cameraPosition.normalized() * newDistance;
#else
    double adjust = std::pow(2.0, event->delta() / 1000.0);
    double newDistance = m_cameraPosition.norm() * adjust;
    m_cameraPosition = m_cameraPosition.normalized() * newDistance;
#endif

    update();
}


void
ModelViewWidget::select(const Vector2f& viewportPoint)
{
    if (!m_model)
    {
        return;
    }

    float aspectRatio = (float) size().width() / (float) size().height();
    float fovRad = float(VIEWPORT_FOV * PI / 180.0f);
    float h = (float) tan(fovRad / 2.0f);
    Vector3d direction(h * aspectRatio * viewportPoint.x(), h * viewportPoint.y(), -1.0f);
    direction.normalize();
    Vector3d origin = Vector3d::Zero();
    Transform3d camera(cameraTransform().inverse());

    Mesh::PickResult pickResult;
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


Transform3d
ModelViewWidget::cameraTransform() const
{
    Transform3d t(m_cameraOrientation.conjugate());
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
    // TODO: eliminate const cast when Model::setMaterial() is implemented
    Material* modelMaterial = const_cast<Material*>(m_model->getMaterial(index));
    modelMaterial->diffuse = material.diffuse;
    modelMaterial->specular = material.specular;
    modelMaterial->emissive = material.emissive;
    modelMaterial->opacity = material.opacity;
    modelMaterial->specularPower = material.specularPower;

    delete modelMaterial->maps[Material::DiffuseMap];
    modelMaterial->maps[Material::DiffuseMap] = NULL;
    delete modelMaterial->maps[Material::SpecularMap];
    modelMaterial->maps[Material::SpecularMap] = NULL;
    delete modelMaterial->maps[Material::NormalMap];
    modelMaterial->maps[Material::NormalMap] = NULL;
    delete modelMaterial->maps[Material::EmissiveMap];
    modelMaterial->maps[Material::EmissiveMap] = NULL;

    if (material.maps[Material::DiffuseMap])
    {
        modelMaterial->maps[Material::DiffuseMap] = new Material::DefaultTextureResource(material.maps[Material::DiffuseMap]->source());
    }
    if (material.maps[Material::SpecularMap])
    {
        modelMaterial->maps[Material::SpecularMap] = new Material::DefaultTextureResource(material.maps[Material::SpecularMap]->source());
    }
    if (material.maps[Material::NormalMap])
    {
        modelMaterial->maps[Material::NormalMap] = new Material::DefaultTextureResource(material.maps[Material::NormalMap]->source());
    }
    if (material.maps[Material::EmissiveMap])
    {
        modelMaterial->maps[Material::EmissiveMap] = new Material::DefaultTextureResource(material.maps[Material::EmissiveMap]->source());
    }

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
    glewInit();
    emit contextCreated();
}


void
ModelViewWidget::paintGL()
{
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
    Vector4f ambientLight = Vector4f::Constant(ambientLightLevel);
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

        Vector3d direction = m_lightOrientation * lightSource.direction;
        Vector4f lightColor = Vector4f::Zero();
        lightColor.start<3>() = lightSource.color * lightSource.intensity;
        Vector4f lightPosition = Vector4f::Zero();
        lightPosition.start<3>() = direction.cast<float>();

        glEnable(glLight);
        glLightfv(glLight, GL_POSITION, lightPosition.data());
        glLightfv(glLight, GL_DIFFUSE,  lightColor.data());
        glLightfv(glLight, GL_SPECULAR, lightColor.data());
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixd(cameraTransform().data());

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (m_model)
    {
        renderModel(m_model);
        if (!m_selection.isEmpty())
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-0.0f, -1.0f);
            renderSelection(m_model);
        }
    }

    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR)
    {
        std::cout << gluErrorString(errorCode) << std::endl;
    }
}


void
ModelViewWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}


static GLenum GLComponentTypes[Mesh::FormatMax] =
{
     GL_FLOAT,          // Float1
     GL_FLOAT,          // Float2
     GL_FLOAT,          // Float3
     GL_FLOAT,          // Float4,
     GL_UNSIGNED_BYTE,  // UByte4
};

static int GLComponentCounts[Mesh::FormatMax] =
{
     1,  // Float1
     2,  // Float2
     3,  // Float3
     4,  // Float4,
     4,  // UByte4
};


static void
setVertexArrays(const Mesh::VertexDescription& desc, const void* vertexData)
{
    const Mesh::VertexAttribute& position  = desc.getAttribute(Mesh::Position);
    const Mesh::VertexAttribute& normal    = desc.getAttribute(Mesh::Normal);
    const Mesh::VertexAttribute& color0    = desc.getAttribute(Mesh::Color0);
    const Mesh::VertexAttribute& texCoord0 = desc.getAttribute(Mesh::Texture0);
    const Mesh::VertexAttribute& tangent   = desc.getAttribute(Mesh::Tangent);

    // Can't render anything unless we have positions
    if (position.format != Mesh::Float3)
        return;

    // Set up the vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, desc.stride,
                    reinterpret_cast<const char*>(vertexData) + position.offset);

    // Set up the normal array
    switch (normal.format)
    {
    case Mesh::Float3:
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GLComponentTypes[(int) normal.format],
                        desc.stride,
                        reinterpret_cast<const char*>(vertexData) + normal.offset);
        break;
    default:
        glDisableClientState(GL_NORMAL_ARRAY);
        break;
    }

    // Set up the color array
    switch (color0.format)
    {
    case Mesh::Float3:
    case Mesh::Float4:
    case Mesh::UByte4:
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GLComponentCounts[color0.format],
                       GLComponentTypes[color0.format],
                       desc.stride,
                       reinterpret_cast<const char*>(vertexData) + color0.offset);
        break;
    default:
        glDisableClientState(GL_COLOR_ARRAY);
        break;
    }

    // Set up the texture coordinate array
    switch (texCoord0.format)
    {
    case Mesh::Float1:
    case Mesh::Float2:
    case Mesh::Float3:
    case Mesh::Float4:
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(GLComponentCounts[(int) texCoord0.format],
                          GLComponentTypes[(int) texCoord0.format],
                          desc.stride,
                          reinterpret_cast<const char*>(vertexData) + texCoord0.offset);
        break;
    default:
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        break;
    }

    switch (tangent.format)
    {
    case Mesh::Float3:
        glEnableVertexAttribArrayARB(TangentAttributeIndex);
        glVertexAttribPointerARB(TangentAttributeIndex,
                                      GLComponentCounts[(int) tangent.format],
                                      GLComponentTypes[(int) tangent.format],
                                      GL_FALSE,
                                      desc.stride,
                                      reinterpret_cast<const char*>(vertexData) + tangent.offset);
        break;
    default:
        glDisableVertexAttribArrayARB(TangentAttributeIndex);
        break;
    }

}


// Set just the vertex pointer
void
setVertexPointer(const Mesh::VertexDescription& desc, const void* vertexData)
{
    const Mesh::VertexAttribute& position  = desc.getAttribute(Mesh::Position);

    // Can't render anything unless we have positions
    if (position.format != Mesh::Float3)
        return;

    // Set up the vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, desc.stride,
                    reinterpret_cast<const char*>(vertexData) + position.offset);

    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


static GLenum
getGLMode(Mesh::PrimitiveGroupType primitive)
{
    switch (primitive)
    {
    case Mesh::TriList:
        return GL_TRIANGLES;
    case Mesh::TriStrip:
        return GL_TRIANGLE_STRIP;
    case Mesh::TriFan:
        return GL_TRIANGLE_FAN;
    case Mesh::LineList:
        return GL_LINES;
    case Mesh::LineStrip:
        return GL_LINE_STRIP;
    case Mesh::PointList:
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
ModelViewWidget::bindMaterial(const Material* material)
{
    QGLShaderProgram* shader = NULL;

    if (renderPath() == OpenGL2Path && !gl2Fail)
    {
        // Lookup the shader in the shader cache
        unsigned lightSourceCount = m_lightingEnabled ? m_lightSources.size() : 0;

        unsigned int shaderKey = computeShaderKey(material, lightSourceCount);
        shader = m_shaderCache.value(shaderKey);
        if (!shader)
        {
            shader = createShader(material, lightSourceCount);
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

        // Get the transpose, because Qt's matrix classes expect row-major data
        Matrix4d modelView = cameraTransform().matrix().transpose();
        shader->setUniformValue("modelView", QMatrix4x4(modelView.data(), 4, 4));

        shader->setUniformValue("diffuseColor", QVector3D(material->diffuse.red(), material->diffuse.green(), material->diffuse.blue()));
        shader->setUniformValue("specularColor", QVector3D(material->specular.red(), material->specular.green(), material->specular.blue()));
        shader->setUniformValue("opacity", material->opacity);
        shader->setUniformValue("specularPower", material->specularPower);

        // TODO: Disable maps when the necessary vertex data is missing
        bool hasDiffuseMap = (material->maps[Material::DiffuseMap] != 0);
        bool hasNormalMap = (material->maps[Material::NormalMap] != 0);
        bool hasSpecularMap = (material->maps[Material::SpecularMap] != 0);
        bool hasEmissiveMap = (material->maps[Material::EmissiveMap] != 0);

        if (hasDiffuseMap)
        {
            GLuint diffuseMapId = m_materialLibrary->getTexture(material->maps[Material::DiffuseMap]->source().c_str());
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, diffuseMapId);
            shader->setUniformValue("diffuseMap", 0);
        }

        if (hasNormalMap)
        {
            GLuint normalMapId = m_materialLibrary->getTexture(material->maps[Material::NormalMap]->source().c_str());
            glActiveTexture(GL_TEXTURE1);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, normalMapId);
            shader->setUniformValue("normalMap", 1);
            glActiveTexture(GL_TEXTURE0);
        }

        if (hasSpecularMap)
        {
            GLuint specularMapId = m_materialLibrary->getTexture(material->maps[Material::SpecularMap]->source().c_str());
            glActiveTexture(GL_TEXTURE2);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, specularMapId);
            shader->setUniformValue("specularMap", 2);
            glActiveTexture(GL_TEXTURE0);
        }

        if (hasEmissiveMap)
        {
            GLuint emissiveMapId = m_materialLibrary->getTexture(material->maps[Material::EmissiveMap]->source().c_str());
            glActiveTexture(GL_TEXTURE3);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, emissiveMapId);
            shader->setUniformValue("emissiveMap", 3);
            glActiveTexture(GL_TEXTURE0);
        }

        unsigned int lightIndex = 0;
        Matrix3d lightMatrix = m_lightOrientation.toRotationMatrix();

        QVector3D lightDirections[8];
        QVector3D lightColors[8];

        foreach (LightSource lightSource, m_lightSources)
        {
            Vector3d direction = lightMatrix * lightSource.direction;
            Vector3f color = lightSource.color * lightSource.intensity;

            lightDirections[lightIndex] = QVector3D(direction.x(), direction.y(), direction.z());
            lightColors[lightIndex] = QVector3D(color.x(), color.y(), color.z());

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
        shader->setUniformValue("ambientLightColor", ambientLightLevel, ambientLightLevel, ambientLightLevel);

        // Get the eye position in model space
        Vector4f eyePosition = cameraTransform().inverse().cast<float>() * Vector4f::UnitW();
        shader->setUniformValue("eyePosition", eyePosition.x(), eyePosition.y(), eyePosition.z());
    }
    else
    {
        if (QGLShaderProgram::hasOpenGLShaderPrograms())
        {
            glUseProgram(0);
        }

        Vector4f diffuse(material->diffuse.red(), material->diffuse.green(), material->diffuse.blue(), material->opacity);
        Vector4f specular(material->specular.red(), material->specular.green(), material->specular.blue(), 1.0f);
        Vector4f emissive(material->emissive.red(), material->emissive.green(), material->emissive.blue(), 1.0f);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse.data());
        glMaterialfv(GL_FRONT, GL_AMBIENT, diffuse.data());
        glColor4fv(diffuse.data());
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular.data());
        glMaterialfv(GL_FRONT, GL_SHININESS, &material->specularPower);
        glMaterialfv(GL_FRONT, GL_EMISSION, emissive.data());

        // Set up the diffuse (base) texture
        GLuint baseTexId = 0;
        if (material->maps[Material::DiffuseMap])
        {
            baseTexId = m_materialLibrary->getTexture(QString(material->maps[Material::DiffuseMap]->source().c_str()));
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
ModelViewWidget::renderModel(Model* model)
{
    Material defaultMaterial;
    defaultMaterial.diffuse = Material::Color(1.0f, 1.0f, 1.0f);

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

    // Render all meshes
    for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
    {
        const Mesh* mesh = model->getMesh(meshIndex);

        setVertexArrays(mesh->getVertexDescription(), mesh->getVertexData());
        if (mesh->getVertexDescription().getAttribute(Mesh::Normal).format == Mesh::Float3)
        {
            setLighting(true);
        }
        else
        {
            setLighting(false);
        }

        for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
        {
            const Mesh::PrimitiveGroup* group = mesh->getGroup(groupIndex);
            const Material* material = &defaultMaterial;
            if (group->materialIndex < model->getMaterialCount())
            {
                 material = model->getMaterial(group->materialIndex);
            }
            bindMaterial(material);

            GLenum primitiveMode = getGLMode(group->prim);
            glDrawElements(primitiveMode, group->nIndices, GL_UNSIGNED_INT, group->indices);
        }
    }

    bindMaterial(&defaultMaterial);
}


void
ModelViewWidget::renderSelection(Model* model)
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
        Material selectionMaterial;
        selectionMaterial.diffuse = Material::Color(0.0f, 1.0f, 0.0f);
        selectionMaterial.opacity = 0.5f;
        bindMaterial(&selectionMaterial);
    }

    for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
    {
        Mesh* mesh = model->getMesh(meshIndex);
        setVertexPointer(mesh->getVertexDescription(), mesh->getVertexData());

        for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
        {
            Mesh::PrimitiveGroup* group = mesh->getGroup(groupIndex);
            if (m_selection.contains(group))
            {
                GLenum primitiveMode = getGLMode(group->prim);
                glDrawElements(primitiveMode, group->nIndices, GL_UNSIGNED_INT, group->indices);
            }
        }
    }

    glPolygonMode(GL_FRONT, GL_FILL);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}


void
ModelViewWidget::setupDefaultLightSources()
{
    m_lightSources.clear();

    LightSource light1;
    light1.color = Vector3f(1.0f, 1.0f, 1.0f);
    light1.intensity = 1.0f;
    light1.direction = Vector3d(1.0, 1.0, 5.0).normalized();

    LightSource light2;
    light2.color = Vector3f(1.0f, 1.0f, 1.0f);
    light2.intensity = 1.0f;
    light2.direction = Vector3d(3.0, -3.0, -1.0).normalized();

    m_lightSources << light1 << light2;
}


QGLShaderProgram*
ModelViewWidget::createShader(const Material* material, unsigned int lightSourceCount)
{
    QString vertexShaderSource;
    QString fragmentShaderSource;

    QTextStream vout(&vertexShaderSource, QIODevice::WriteOnly);
    QTextStream fout(&fragmentShaderSource, QIODevice::WriteOnly);

    bool hasDiffuseMap = (material->maps[Material::DiffuseMap] != 0);
    bool hasSpecularMap = (material->maps[Material::SpecularMap] != 0);
    bool hasEmissiveMap = (material->maps[Material::EmissiveMap] != 0);
    bool hasNormalMap = (material->maps[Material::NormalMap] != 0);
    bool hasMaps = (hasDiffuseMap || hasSpecularMap || hasEmissiveMap || hasNormalMap);
    bool hasSpecular = (material->specular.red() != 0.0f ||
                        material->specular.green() != 0.0f ||
                        material->specular.blue() != 0.0f);

    bool compressedNormalMap = false;
    if (hasNormalMap)
    {
        compressedNormalMap = material->maps[Material::NormalMap]->source().rfind(".dxt5nm") != std::string::npos;
    }

    if (lightSourceCount == 0)
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
        if (hasMaps)
        {
            vout << "varying vec2 texCoord;\n";
            fout << "varying vec2 texCoord;\n";
        }

        if (hasNormalMap)
        {
            vout << "attribute vec3 tangentAtt;\n";
            vout << "varying vec3 tangent;\n";
            fout << "varying vec3 tangent;\n";
        }

        /*** Vertex shader ***/
        //vout << "attribute vec3 position;\n";
        //vout << "attribute vec3 normal;\n";
        vout << "uniform mat4 modelView;\n";

        vout << "void main(void)\n";
        vout << "{\n";
        vout << "    normal = gl_Normal;\n";
        vout << "    position = gl_Vertex.xyz;\n";
        if (hasMaps)
        {
            vout << "    texCoord = gl_MultiTexCoord0.xy;\n";
        }
        if (hasNormalMap)
        {
            vout << "    tangent = tangentAtt;\n";
        }
        vout << "    gl_Position = ftransform();\n";
        //vout << "   gl_Position = modelView * gl_Vertex;\n";
        vout << "}";
        /*** End vertex shader ***/


        /*** Fragment shader ***/
        fout << "uniform vec3 eyePosition;\n";
        fout << "uniform vec3 lightDirection[" << lightSourceCount << "];\n";
        fout << "uniform vec3 lightColor[" << lightSourceCount << "];\n";
        fout << "uniform vec3 ambientLightColor;\n";
        fout << "uniform vec3 diffuseColor;\n";
        fout << "uniform vec3 specularColor;\n";
        fout << "uniform float specularPower;\n";
        fout << "uniform float opacity;\n";
        if (material->maps[Material::DiffuseMap])
        {
            fout << "uniform sampler2D diffuseMap;\n";
        }
        if (material->maps[Material::SpecularMap])
        {
            fout << "uniform sampler2D specularMap;\n";
        }
        if (material->maps[Material::EmissiveMap])
        {
            fout << "uniform sampler2D emissiveMap;\n";
        }
        if (material->maps[Material::NormalMap])
        {
            fout << "uniform sampler2D normalMap;\n";
        }
        fout << "void main(void)\n";
        fout << "{\n";
        fout << "   vec3 baseColor = diffuseColor;\n";

        if (hasSpecular)
        {
            fout << "   vec3 specularLight = vec3(0.0);\n";
            fout << "   vec3 V = normalize(eyePosition - position);\n"; // view vector
        }

        if (material->maps[Material::DiffuseMap])
        {
            fout << "    baseColor *= texture2D(diffuseMap, texCoord).rgb;\n";
        }

        // Compute the surface normal N
        if (hasNormalMap)
        {
            // Fetch the normal from the normal map
            if (compressedNormalMap)
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
        fout << "   for (int i = 0; i < " << lightSourceCount << "; ++i)\n";
        fout << "   {\n";
        fout << "       float d = max(0.0, dot(lightDirection[i], N));\n";

        // Self shadowing term required for normal maps and specular materials
        if (hasNormalMap)
        {
            // IMPORTANT: Use the surface normal from the geometry, not the one
            // retrieved from the normal map.
            fout << "        float selfShadow = clamp(dot(lightDirection[i], N0) * 8.0, 0.0, 1.0);\n";
        }
        else if (hasSpecular)
        {
            fout << "        float selfShadow = clamp(d * 8.0, 0.0, 1.0);\n";
        }
        else
        {
            fout << "        float selfShadow = 1.0;\n";
        }

        fout << "       light += lightColor[i] * (d * selfShadow);\n";
        if (hasSpecular)
        {
            fout << "       vec3 H = normalize(lightDirection[i] + V);\n"; // half angle vector
            fout << "       float spec = pow(max(0.0, dot(H, N)), specularPower);\n";
            fout << "       if (d == 0.0) spec = 0.0;\n";
            fout << "       specularLight += lightColor[i] * (spec * selfShadow);\n";
        }
        fout << "   }\n";

        fout << "   vec3 color = light * baseColor;\n";
        if (hasSpecular)
        {
            if (hasSpecularMap)
            {
                fout << "    color += specularLight * specularColor * texture2D(specularMap, texCoord).xyz;\n";
            }
            else
            {
                fout << "    color += specularLight * specularColor;\n";
            }
        }

        if (hasEmissiveMap)
        {
            fout << "    color += texture2D(emissiveMap, texCoord).xyz;\n";
        }

        fout << "   gl_FragColor = vec4(color, opacity);\n";

        fout << "}\n";
        /*** End fragment shader ***/
    }

    QGLShaderProgram* glShader = new QGLShaderProgram(this);
    QGLShader* vertexShader = new QGLShader(QGLShader::Vertex, glShader);
    if (!vertexShader->compileSourceCode(vertexShaderSource))
    {
        std::cerr << "Vertex shader error!\n";
        std::cerr << vertexShader->log().toAscii().data() << std::endl;
        std::cerr << vertexShaderSource.toAscii().data() << std::endl;
        delete glShader;
        return NULL;
    }

    QGLShader* fragmentShader = new QGLShader(QGLShader::Fragment, glShader);
    if (!fragmentShader->compileSourceCode(fragmentShaderSource))
    {
        std::cerr << "Fragment shader error!\n";
        std::cerr << fragmentShader->log().toAscii().data() << std::endl;
        std::cerr << fragmentShaderSource.toAscii().data() << std::endl;
        delete glShader;
        return NULL;
    }

    glShader->addShader(vertexShader);
    glShader->addShader(fragmentShader);
    if (hasNormalMap)
    {
        glShader->bindAttributeLocation("tangentAtt", TangentAttributeIndex);
    }

    if (!glShader->link())
    {
        std::cerr << glShader->log().toAscii().data() << std::endl;
        delete glShader;
        return NULL;
    }

    return glShader;
}
