/***************************************************************************
                          kdeglwidget.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 2002
    copyright            : (C) 2002 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qtglwidget.h"

#include <cstdlib>
#include <utility>

#include <config.h>

#include <Qt>
#include <QtGlobal>
#include <QCursor>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QString>
#include <QWheelEvent>

#include <celengine/body.h>
#include <celengine/render.h>
#include <celengine/starcolors.h>
#include <celestia/configfile.h>
#include <celutil/gettext.h>
#include "qtdraghandler.h"

namespace celestia::qt
{

namespace
{

constexpr auto DEFAULT_ORBIT_MASK = static_cast<int>(BodyClassification::Planet | BodyClassification::Moon | BodyClassification::Stellar);
constexpr int DEFAULT_LABEL_MODE = Renderer::LocationLabels | Renderer::I18nConstellationLabels;
constexpr float DEFAULT_AMBIENT_LIGHT_LEVEL = 0.1f;
constexpr float DEFAULT_TINT_SATURATION = 0.5f;
constexpr int DEFAULT_STARS_COLOR = static_cast<int>(ColorTableType::Blackbody_D65);
constexpr float DEFAULT_VISUAL_MAGNITUDE = 8.0f;
constexpr Renderer::StarStyle DEFAULT_STAR_STYLE = Renderer::FuzzyPointStars;
constexpr unsigned int DEFAULT_TEXTURE_RESOLUTION = medres;

std::pair<float, float>
mousePosition(const QMouseEvent& m, qreal scale)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return { static_cast<float>(m.x() * scale), static_cast<float>(m.y() * scale) };
#else
    auto position = m.position();
    return { static_cast<float>(position.x() * scale), static_cast<float>(position.y() * scale) };
#endif
}

} // end unnamed namespace

CelestiaGlWidget::CelestiaGlWidget(QWidget* parent, const char* /* name */, CelestiaCore* core) :
    QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus);

    appCore = core;
    appRenderer = appCore->getRenderer();

    setCursor(QCursor(Qt::CrossCursor));
    currentCursor = CelestiaCore::CrossCursor;
    setMouseTracking(true);

    dragHandler = createDragHandler(this, appCore);
    cursorVisible = true;

    // We use glClear directly, so we don't need it called by Qt.
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
}

CelestiaGlWidget::~CelestiaGlWidget() = default;

/*!
  Paint the box. The actual openGL commands for drawing the box are
  performed here.
*/

void
CelestiaGlWidget::paintGL()
{
    appCore->draw();
}

/*!
  Set up the OpenGL rendering state, and define display list
*/

void
CelestiaGlWidget::initializeGL()
{
    using namespace celestia;
#ifdef GL_ES
    if (!gl::init(appCore->getConfig()->renderDetails.ignoreGLExtensions) ||
        !gl::checkVersion(gl::GLES_2))
    {
        QMessageBox::critical(nullptr, "Celestia", _("Celestia was unable to initialize OpenGLES 2.0."));
        std::exit(1);
    }
#else
    if (!gl::init(appCore->getConfig()->renderDetails.ignoreGLExtensions) ||
        !gl::checkVersion(gl::GL_2_1))
    {
        QMessageBox::critical(nullptr, "Celestia", _("Celestia was unable to initialize OpenGL 2.1."));
        std::exit(1);
    }
#endif

    appCore->setScreenDpi(logicalDpiY() * devicePixelRatioF());

    if (!appCore->initRenderer(false))
    {
        // cerr << "Failed to initialize renderer.\n";
        exit(1);
    }

    appCore->tick();

    // Read saved settings
    QSettings settings;
    appRenderer->setRenderFlags(settings.value("RenderFlags", static_cast<quint64>(Renderer::DefaultRenderFlags)).toULongLong());
    appRenderer->setOrbitMask(static_cast<BodyClassification>(settings.value("OrbitMask", DEFAULT_ORBIT_MASK).toInt()));
    appRenderer->setLabelMode(settings.value("LabelMode", DEFAULT_LABEL_MODE).toInt());
    appRenderer->setAmbientLightLevel((float) settings.value("AmbientLightLevel", DEFAULT_AMBIENT_LIGHT_LEVEL).toDouble());
    appRenderer->setTintSaturation(static_cast<float>(settings.value("TintSaturation", DEFAULT_TINT_SATURATION).toDouble()));
    appRenderer->setStarStyle((Renderer::StarStyle) settings.value("StarStyle", DEFAULT_STAR_STYLE).toInt());
    appRenderer->setResolution(settings.value("TextureResolution", DEFAULT_TEXTURE_RESOLUTION).toUInt());

    appRenderer->setStarColorTable(static_cast<ColorTableType>(settings.value("StarsColor", DEFAULT_STARS_COLOR).toInt()));

    appCore->getSimulation()->getActiveObserver()->setLocationFilter(settings.value("LocationFilter", static_cast<quint64>(Observer::DefaultLocationFilter)).toULongLong());

    appCore->getSimulation()->setFaintestVisible((float) settings.value("Preferences/VisualMagnitude", DEFAULT_VISUAL_MAGNITUDE).toDouble());

    appRenderer->setSolarSystemMaxDistance(appCore->getConfig()->renderDetails.SolarSystemMaxDistance);
    appRenderer->setShadowMapSize(appCore->getConfig()->renderDetails.ShadowMapSize);
}

void
CelestiaGlWidget::resizeGL(int w, int h)
{
    qreal scale = devicePixelRatioF();
    auto width = static_cast<int>(w * scale);
    auto height = static_cast<int>(h * scale);
    appCore->resize(width, height);
}

void
CelestiaGlWidget::mouseMoveEvent(QMouseEvent* m)
{
    qreal scale = devicePixelRatioF();
    auto [x, y] = mousePosition(*m, scale);

    int buttons = 0;
    if (m->buttons() & Qt::LeftButton)
        buttons |= CelestiaCore::LeftButton;
    if (m->buttons() & Qt::MiddleButton)
        buttons |= CelestiaCore::MiddleButton;
    if (m->buttons() & Qt::RightButton)
        buttons |= CelestiaCore::RightButton;
    if (m->modifiers() & Qt::ShiftModifier)
        buttons |= CelestiaCore::ShiftKey;
    if (m->modifiers() & Qt::ControlModifier)
        buttons |= CelestiaCore::ControlKey;

#ifdef __APPLE__
    // On the Mac, right dragging is be simulated with Option+left drag.
    // We may want to enable this on other platforms, though it's mostly only helpful
    // for users with single button mice.
    if (m->modifiers() & Qt::AltModifier)
        buttons |= CelestiaCore::AltKey;
#endif

    if ((m->buttons() & (Qt::LeftButton | Qt::RightButton)) != 0)
    {
        if (cursorVisible)
        {
            // Hide the cursor.
            setCursor(QCursor(Qt::BlankCursor));
            cursorVisible = false;

            dragHandler->begin(*m, scale, buttons);
        }

        dragHandler->move(*m, scale);
    }
    else
    {
        appCore->mouseMove(x, y);
    }
}

void
CelestiaGlWidget::mousePressEvent(QMouseEvent* m)
{
    qreal scale = devicePixelRatioF();
    auto [x, y] = mousePosition(*m, scale);

    if (m->button() == Qt::LeftButton)
    {
        dragHandler->setButton(CelestiaCore::LeftButton);
        appCore->mouseButtonDown(x, y, CelestiaCore::LeftButton);
    }
    else if (m->button() == Qt::MiddleButton)
    {
        dragHandler->setButton(CelestiaCore::MiddleButton);
        appCore->mouseButtonDown(x, y, CelestiaCore::MiddleButton);
    }
    else if (m->button() == Qt::RightButton)
    {
        dragHandler->setButton(CelestiaCore::RightButton);
        appCore->mouseButtonDown(x, y, CelestiaCore::RightButton);
    }
}

void
CelestiaGlWidget::mouseReleaseEvent(QMouseEvent* m)
{
    qreal scale = devicePixelRatioF();
    auto [x, y] = mousePosition(*m, scale);

    if (m->button() == Qt::LeftButton)
    {
        if (!cursorVisible)
        {
            // Restore the cursor position and make it visible again.
            setCursor(QCursor(Qt::CrossCursor));
            cursorVisible = true;
            dragHandler->finish();
        }
        dragHandler->clearButton(CelestiaCore::LeftButton);
        appCore->mouseButtonUp(x, y, CelestiaCore::LeftButton);
    }
    else if (m->button() == Qt::MiddleButton)
    {
        dragHandler->clearButton(CelestiaCore::MiddleButton);
        appCore->mouseButtonUp(x, y, CelestiaCore::MiddleButton);
    }
    else if (m->button() == Qt::RightButton)
    {
        if (!cursorVisible)
        {
            // Restore the cursor position and make it visible again.
            setCursor(QCursor(Qt::CrossCursor));
            cursorVisible = true;
            dragHandler->finish();
        }
        dragHandler->clearButton(CelestiaCore::RightButton);
        appCore->mouseButtonUp(x, y, CelestiaCore::RightButton);
    }
}

void
CelestiaGlWidget::wheelEvent(QWheelEvent* w)
{
    QPoint numDegrees = w->angleDelta();
    if (numDegrees.isNull() || numDegrees.y() == 0)
        return;

    if (numDegrees.y() > 0 )
    {
        appCore->mouseWheel(-1.0f, 0);
    }
    else
    {
        appCore->mouseWheel(1.0f, 0);
    }
}

bool
CelestiaGlWidget::handleSpecialKey(QKeyEvent* e, bool down)
{
    int k = -1;
    switch (e->key())
    {
    case Qt::Key_Up:
        k = CelestiaCore::Key_Up;
        break;
    case Qt::Key_Down:
        k = CelestiaCore::Key_Down;
        break;
    case Qt::Key_Left:
        k = CelestiaCore::Key_Left;
        break;
    case Qt::Key_Right:
        k = CelestiaCore::Key_Right;
        break;
    case Qt::Key_Home:
        k = CelestiaCore::Key_Home;
        break;
    case Qt::Key_End:
        k = CelestiaCore::Key_End;
        break;
    case Qt::Key_F1:
        k = CelestiaCore::Key_F1;
        break;
    case Qt::Key_F2:
        k = CelestiaCore::Key_F2;
        break;
    case Qt::Key_F3:
        k = CelestiaCore::Key_F3;
        break;
    case Qt::Key_F4:
        k = CelestiaCore::Key_F4;
        break;
    case Qt::Key_F5:
        k = CelestiaCore::Key_F5;
        break;
    case Qt::Key_F6:
        k = CelestiaCore::Key_F6;
        break;
    case Qt::Key_F7:
        k = CelestiaCore::Key_F7;
        break;
    case Qt::Key_F11:
        k = CelestiaCore::Key_F11;
        break;
    case Qt::Key_F12:
        k = CelestiaCore::Key_F12;
        break;
    case Qt::Key_PageDown:
        k = CelestiaCore::Key_PageDown;
        break;
    case Qt::Key_PageUp:
        k = CelestiaCore::Key_PageUp;
        break;
/*    case Qt::Key_F10:
        if (e->modifiers()& ShiftModifier)
            k = CelestiaCore::Key_F10;
        break;*/
    case Qt::Key_0:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad0;
        break;
    case Qt::Key_1:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad1;
        break;
    case Qt::Key_2:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad2;
        break;
    case Qt::Key_3:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad3;
        break;
    case Qt::Key_4:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad4;
        break;
    case Qt::Key_5:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad5;
        break;
    case Qt::Key_6:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad6;
        break;
    case Qt::Key_7:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad7;
        break;
    case Qt::Key_8:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad8;
        break;
    case Qt::Key_9:
        if (e->modifiers() & Qt::KeypadModifier)
            k = CelestiaCore::Key_NumPad9;
        break;
    case Qt::Key_A:
        if (e->modifiers() == Qt::NoModifier)
            k = 'A';
        break;
    case Qt::Key_Z:
        if (e->modifiers() == Qt::NoModifier)
            k = 'Z';
        break;
    }

    if (k >= 0)
    {
        int buttons = 0;
        if (e->modifiers() & Qt::ShiftModifier)
            buttons |= CelestiaCore::ShiftKey;

        if (down)
            appCore->keyDown(k, buttons);
        else
            appCore->keyUp(k);
        return (k < 'A' || k > 'Z');
    }

    return false;
}

void
CelestiaGlWidget::keyPressEvent(QKeyEvent* e)
{
    int modifiers = 0;
    if (e->modifiers() & Qt::ShiftModifier)
    {
        modifiers |= CelestiaCore::ShiftKey;
    }
    if (e->modifiers() & Qt::ControlModifier)
    {
        modifiers |= CelestiaCore::ControlKey;
    }

#ifdef __APPLE__
    // Mac Option+left drag
    if (e->modifiers() & Qt::AltModifier)
        dragHandler->setButton(modifiers | CelestiaCore::AltKey);
    else
        dragHandler->setButton(modifiers);
#else
    dragHandler->setButton(modifiers);
#endif
    switch (e->key())
    {
    case Qt::Key_Escape:
        appCore->charEntered('\033');
        break;
    case Qt::Key_Backtab:
        appCore->charEntered(CelestiaCore::Key_BackTab);
        break;
    default:
        if (!handleSpecialKey(e, true))
        {
            if (!e->text().isEmpty())
            {
                QString input = e->text();
#ifdef __APPLE__
                // Taken from the macOS project
                if (input.length() == 1)
                {
                    uint16_t c = input.at(0).unicode();
                    if (c == 0x7f /* NSDeleteCharacter */)
                        input.replace(0, 1, QChar((uint16_t)0x08) /* NSBackspaceCharacter */); // delete = backspace
                    else if (c == 0x19 /* NSBackTabCharacter */)
                        input.replace(0, 1, QChar((uint16_t)0x7f) /* NSDeleteCharacter */);
                }
#endif
                appCore->charEntered(input.toUtf8().data(), modifiers);
            }
        }
    }
}

void
CelestiaGlWidget::keyReleaseEvent(QKeyEvent* e)
{
    int modifiers = 0;
    if (!(e->modifiers() & Qt::ShiftModifier))
        modifiers |= CelestiaCore::ShiftKey;
    if (!(e->modifiers() & Qt::ControlModifier))
        modifiers |= CelestiaCore::ControlKey;
#ifdef __APPLE__
    if (!(e->modifiers() & Qt::AltModifier))
        modifiers |= CelestiaCore::AltKey;
#endif
    dragHandler->clearButton(modifiers);
    handleSpecialKey(e, false);
}

void
CelestiaGlWidget::setCursorShape(CelestiaCore::CursorShape shape)
{
    Qt::CursorShape cursor;
    if (currentCursor != shape)
    {
        switch(shape)
        {
        case CelestiaCore::ArrowCursor:
            cursor = Qt::ArrowCursor;
            break;
        case CelestiaCore::UpArrowCursor:
            cursor = Qt::UpArrowCursor;
            break;
        case CelestiaCore::CrossCursor:
            cursor = Qt::CrossCursor;
            break;
        case CelestiaCore::InvertedCrossCursor:
            cursor = Qt::CrossCursor;
            break;
        case CelestiaCore::WaitCursor:
            cursor = Qt::WaitCursor;
            break;
        case CelestiaCore::BusyCursor:
            cursor = Qt::WaitCursor;
            break;
        case CelestiaCore::IbeamCursor:
            cursor = Qt::IBeamCursor;
            break;
        case CelestiaCore::SizeVerCursor:
            cursor = Qt::SizeVerCursor;
            break;
        case CelestiaCore::SizeHorCursor:
            cursor = Qt::SizeHorCursor;
            break;
        case CelestiaCore::SizeBDiagCursor:
            cursor = Qt::SizeBDiagCursor;
            break;
        case CelestiaCore::SizeFDiagCursor:
            cursor = Qt::SizeFDiagCursor;
            break;
        case CelestiaCore::SizeAllCursor:
            cursor = Qt::SizeAllCursor;
            break;
        case CelestiaCore::SplitVCursor:
            cursor = Qt::SplitVCursor;
            break;
        case CelestiaCore::SplitHCursor:
            cursor = Qt::SplitHCursor;
            break;
        case CelestiaCore::PointingHandCursor:
            cursor = Qt::PointingHandCursor;
            break;
        case CelestiaCore::ForbiddenCursor:
            cursor = Qt::ForbiddenCursor;
            break;
        case CelestiaCore::WhatsThisCursor:
            cursor = Qt::WhatsThisCursor;
            break;
        default:
            cursor = Qt::CrossCursor;
            break;
        }
        setCursor(QCursor(cursor));
        currentCursor = shape;
    }
}

CelestiaCore::CursorShape
CelestiaGlWidget::getCursorShape() const
{
    return currentCursor;
}

QSize
CelestiaGlWidget::sizeHint() const
{
    return QSize(640, 480);
}

} // end namespace celestia::qt
