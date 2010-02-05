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

using namespace cmod;
using namespace Eigen;


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
        QString ext = QFileInfo(fileName).suffix().toLower();
        if (ext == "dds")
        {
            return m_glWidget->bindTexture(fileName);
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


ModelViewWidget::ModelViewWidget(QWidget *parent) :
   QGLWidget(parent),
   m_model(NULL),
   m_modelBoundingRadius(1.0),
   m_cameraPosition(Vector3d::Zero()),
   m_cameraOrientation(Quaterniond::Identity()),
   m_renderStyle(NormalStyle),
   m_materialLibrary(NULL)
{
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
        }
    }

    update();
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

    m_modelBoundingRadius = bbox.max().norm();
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
ModelViewWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePosition = event->pos();
}


void
ModelViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_lastMousePosition.x();
    int dy = event->y() - m_lastMousePosition.y();

    if (event->buttons() & Qt::LeftButton)
    {
        double xrotation = (double) dy / 100.0;
        double yrotation = (double) dx / 100.0;
        Quaterniond q = AngleAxis<double>(-xrotation, Vector3d::UnitX()) *
                        AngleAxis<double>(-yrotation, Vector3d::UnitY());

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
    double adjust = m_modelBoundingRadius * event->delta() / 1000.0;
    double newDistance = m_cameraPosition.norm() + adjust;
    m_cameraPosition = m_cameraPosition.normalized() * newDistance;

    update();
}


void
ModelViewWidget::initializeGL()
{
}


void
ModelViewWidget::paintGL()
{
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspectRatio = (double) size().width() / (double) size().height();
    double nearDistance = m_modelBoundingRadius * 0.05;
    double farDistance = m_modelBoundingRadius * 20.0;
    gluPerspective(45.0, aspectRatio, nearDistance, farDistance);

    glEnable(GL_LIGHTING);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    Vector4f ambientLight(0.2f, 0.2f, 0.2f, 1.0f);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight.data());
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);

    glEnable(GL_LIGHT0);
    Vector4f lightDir0(100.0f, 100.0f, 500.0f, 0.0f);
    Vector4f lightDiffuse0(1.0f, 1.0f, 1.0f, 1.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, lightDir0.data());
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse0.data());
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightDiffuse0.data());

    glEnable(GL_LIGHT1);
    Vector4f lightDir1(300.0f, -300.0f, -100.0f, 0.0f);
    Vector4f lightDiffuse1(1.0f, 1.0f, 1.0f, 1.0f);
    glLightfv(GL_LIGHT1, GL_POSITION, lightDir1.data());
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  lightDiffuse1.data());
    glLightfv(GL_LIGHT1, GL_SPECULAR, lightDiffuse1.data());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Transform3d cameraRotation(m_cameraOrientation.conjugate());
    glMultMatrixd(cameraRotation.data());
    glTranslated(-m_cameraPosition.x(), -m_cameraPosition.y(), -m_cameraPosition.z());


    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (m_model)
    {
        renderModel(m_model);
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
}


void
ModelViewWidget::bindMaterial(const Material* material)
{
    Vector4f diffuse(material->diffuse.red(), material->diffuse.green(), material->diffuse.blue(), material->opacity);
    Vector4f specular(material->specular.red(), material->specular.green(), material->specular.blue(), 1.0f);
    Vector4f emissive(material->emissive.red(), material->emissive.green(), material->emissive.blue(), 1.0f);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse.data());
    glMaterialfv(GL_FRONT, GL_AMBIENT, diffuse.data());
    glColor4fv(diffuse.data());
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular.data());
    glMaterialfv(GL_FRONT, GL_SHININESS, &material->specularPower);
    glMaterialfv(GL_FRONT, GL_EMISSION, emissive.data());

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

    for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
    {
        const Mesh* mesh = model->getMesh(meshIndex);

        setVertexArrays(mesh->getVertexDescription(), mesh->getVertexData());
        if (mesh->getVertexDescription().getAttribute(Mesh::Normal).format == Mesh::Float3)
        {
            glEnable(GL_LIGHTING);
        }
        else
        {
            glDisable(GL_LIGHTING);
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

            GLenum primitiveMode = 0;
            bool validMode = true;
            switch (group->prim)
            {
            case Mesh::TriList:
                primitiveMode = GL_TRIANGLES;
                break;
            case Mesh::TriStrip:
                primitiveMode = GL_TRIANGLE_STRIP;
                break;
            case Mesh::TriFan:
                primitiveMode = GL_TRIANGLE_FAN;
                break;
            case Mesh::LineList:
                primitiveMode = GL_LINES;
                break;
            case Mesh::LineStrip:
                primitiveMode = GL_LINE_STRIP;
                break;
            case Mesh::PointList:
                primitiveMode = GL_POINTS;
                break;
            default:
                validMode = false;
                break;
            }

            if (validMode)
            {
                glDrawElements(primitiveMode, group->nIndices, GL_UNSIGNED_INT, group->indices);
            }
        }
    }
}
