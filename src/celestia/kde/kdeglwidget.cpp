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

#include "kdeglwidget.h"
#include <kaccel.h>

#include <unistd.h> 
#include <celengine/gl.h>
//#include <celengine/glext.h>
#include <celengine/celestia.h>
#include <celengine/starbrowser.h>
#include <kmainwindow.h>
#include <kconfig.h>
#include <qpaintdevicemetrics.h>

#ifndef DEBUG
#  define G_DISABLE_ASSERT
#endif

#include "celmath/vecmath.h"
#include "celmath/quaternion.h"
#include "celmath/mathlib.h"
#include "celengine/astro.h"
#include "celutil/util.h"
#include "celutil/filetype.h"
#include "celutil/debug.h"
#include "imagecapture.h"
#include "celestiacore.h"
#include "celengine/simulation.h"
#include "celengine/glcontext.h"

#include "kdeapp.h"

#include <math.h>

KdeGlWidget::KdeGlWidget(  QWidget* parent, const char* name, CelestiaCore* core )
    : QGLWidget( parent, name )
{
    if (chdir(CONFIG_DATA_DIR) == -1)
    {
//        cerr << "Cannot chdir to '" << CONFIG_DATA_DIR <<
//            "', probably due to improper installation\n";
    }
    
    actionColl = ((KdeApp*)parent)->actionCollection();

    setFocusPolicy(QWidget::ClickFocus);

    appCore = core;
    appRenderer=appCore->getRenderer();
    appSim = appCore->getSimulation();


    lastX = lastY = 0;

    if (!appCore->initSimulation())
    {
        exit(1);
    }

}


/*!
  Release allocated resources
*/

KdeGlWidget::~KdeGlWidget()
{
}


/*!
  Paint the box. The actual openGL commands for drawing the box are
  performed here.
*/

void KdeGlWidget::paintGL()
{
    appCore->draw();
}


/*!
  Set up the OpenGL rendering state, and define display list
*/

void KdeGlWidget::initializeGL()
{
    if (!appCore->initRenderer())
    {
//        cerr << "Failed to initialize renderer.\n";
        exit(1);
    }

    time_t curtime=time(NULL);
    appCore->start((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
    localtime(&curtime); /* Only doing this to set timezone as a side effect*/
    appCore->setTimeZoneBias(-timezone+3600*daylight);
    appCore->setTimeZoneName(tzname[daylight?0:1]);

    KGlobal::config()->setGroup("Preferences");
    if (KGlobal::config()->hasKey("RendererFlags"))
        appRenderer->setRenderFlags(KGlobal::config()->readNumEntry("RendererFlags"));
    if (KGlobal::config()->hasKey("LabelMode"))
        appRenderer->setLabelMode(KGlobal::config()->readNumEntry("LabelMode"));
    if (KGlobal::config()->hasKey("AmbientLightLevel"))
        appRenderer->setAmbientLightLevel(KGlobal::config()->readDoubleNumEntry("AmbientLightLevel"));
    if (KGlobal::config()->hasKey("FaintestVisible"))
        appCore->setFaintest(KGlobal::config()->readDoubleNumEntry("FaintestVisible"));
    if (KGlobal::config()->hasKey("HudDetail"))
        appCore->setHudDetail(KGlobal::config()->readNumEntry("HudDetail"));
    if (KGlobal::config()->hasKey("TimeZoneBias"))
        appCore->setTimeZoneBias(KGlobal::config()->readNumEntry("TimeZoneBias"));

    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_Basic))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathBasic")))->setEnabled(false);
    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_Multitexture))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathMultitexture")))->setEnabled(false);
    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNvCombiner")))->setEnabled(false);
    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_DOT3_ARBVP))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathDOT3ARBVP")))->setEnabled(false);
    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner_NvVP))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNvCombinerNvVP")))->setEnabled(false);
    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner_ARBVP))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNvCombinerARBVP")))->setEnabled(false);
    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_ARBFP_ARBVP))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathARBFPARBVP")))->setEnabled(false);
    if (!appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NV30))
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNV30")))->setEnabled(false);

    if (KGlobal::config()->hasKey("RenderPath")) {
       GLContext::GLRenderPath path = (GLContext::GLRenderPath)KGlobal::config()->readNumEntry("RenderPath");
       if (appCore->getRenderer()->getGLContext()->renderPathSupported(path)) {
            appCore->getRenderer()->getGLContext()->setRenderPath(path);
       }
    }

    switch (appCore->getRenderer()->getGLContext()->getRenderPath()) {
    case GLContext::GLPath_Basic:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathBasic")))->setChecked(true);
        break;
    case GLContext::GLPath_Multitexture:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathMultitexture")))->setChecked(true);
        break;
    case GLContext::GLPath_NvCombiner:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNvCombiner")))->setChecked(true);
        break;
    case GLContext::GLPath_DOT3_ARBVP:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathDOT3ARBVP")))->setChecked(true);
        break;
    case GLContext::GLPath_NvCombiner_NvVP:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNvCombinerNvVP")))->setChecked(true);
        break;
    case GLContext::GLPath_NvCombiner_ARBVP:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNvCombinerARBVP")))->setChecked(true);
        break;
    case GLContext::GLPath_ARBFP_ARBVP:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathARBFPARBVP")))->setChecked(true);
        break;
    case GLContext::GLPath_NV30:
        ((KToggleAction*)(((KdeApp*)parentWidget())->action("renderPathNV30")))->setChecked(true);
        break;
    }

    KGlobal::config()->setGroup(0);

    QPaintDeviceMetrics pdm(this);
    appCore->setScreenDpi(pdm.logicalDpiY());
}

void KdeGlWidget::resizeGL( int w, int h )
{
/*    QPaintDeviceMetrics pdm(this);
    double fov = 2 * atan(h / (pdm.logicalDpiY() / 25.4) / 2. / 400.);
    appCore->getSimulation()->getActiveObserver()->setFOV(fov);
*/
    appCore->resize(w, h);
}

void KdeGlWidget::mouseMoveEvent( QMouseEvent* m )
{
    int x = (int) m->x();
    int y = (int) m->y();

    int buttons = 0;
    if (m->state() & LeftButton)
        buttons |= CelestiaCore::LeftButton;
    if (m->state() & MidButton)
        buttons |= CelestiaCore::MiddleButton;
    if (m->state() & RightButton)
        buttons |= CelestiaCore::RightButton;
    if (m->state() & ShiftButton)
        buttons |= CelestiaCore::ShiftKey;
    if (m->state() & ControlButton)
        buttons |= CelestiaCore::ControlKey;

    appCore->mouseMove(x - lastX, y - lastY, buttons);

    lastX = x;
    lastY = y;
}

void KdeGlWidget::mousePressEvent( QMouseEvent* m )
{
    lastX = (int) m->x();
    lastY = (int) m->y();

    if (m->button() == LeftButton)
        appCore->mouseButtonDown(m->x(), m->y(), CelestiaCore::LeftButton);
    else if (m->button() == MidButton)
        appCore->mouseButtonDown(m->x(), m->y(), CelestiaCore::MiddleButton);
    else if (m->button() == RightButton)
        appCore->mouseButtonDown(m->x(), m->y(), CelestiaCore::RightButton);

}

void KdeGlWidget::mouseReleaseEvent( QMouseEvent* m )
{
    lastX = (int) m->x();
    lastY = (int) m->y();
    if (m->button() == LeftButton)
        appCore->mouseButtonUp(m->x(), m->y(), CelestiaCore::LeftButton);
    else if (m->button() == MidButton)
        appCore->mouseButtonUp(m->x(), m->y(), CelestiaCore::MiddleButton);
    else if (m->button() == RightButton)
        appCore->mouseButtonUp(m->x(), m->y(), CelestiaCore::RightButton);
}

void KdeGlWidget::wheelEvent( QWheelEvent* w )
{
    if (w->delta() > 0 )
    {
        appCore->mouseWheel(1.0f, 0);
    }
    else if (w->delta() < 0)
    {
        appCore->mouseWheel(-1.0f, 0);
    }
}


bool KdeGlWidget::handleSpecialKey(QKeyEvent* e, bool down)
{
    int k = -1;
    switch (e->key())
    {
    case Key_Up:
        k = CelestiaCore::Key_Up;
        break;
    case Key_Down:
        k = CelestiaCore::Key_Down;
        break;
    case Key_Left:
        k = CelestiaCore::Key_Left;
        break;
    case Key_Right:
        k = CelestiaCore::Key_Right;
        break;
    case Key_Home:
        k = CelestiaCore::Key_Home;
        break;
    case Key_End:
        k = CelestiaCore::Key_End;
        break;
    case Key_F1:
        k = CelestiaCore::Key_F1;
        break;
    case Key_F2:
        k = CelestiaCore::Key_F2;
        break;
    case Key_F3:
        k = CelestiaCore::Key_F3;
        break;
    case Key_F4:
        k = CelestiaCore::Key_F4;
        break;
    case Key_F5:
        k = CelestiaCore::Key_F5;
        break;
    case Key_F6:
        k = CelestiaCore::Key_F6;
        break;
    case Key_F7:
        k = CelestiaCore::Key_F7;
        break;
    case Key_PageDown:
        k = CelestiaCore::Key_PageDown;
        break;
    case Key_PageUp:
        k = CelestiaCore::Key_PageUp;
        break;
/*    case Key_F10:
        if (down)
            menuCaptureImage();
        break;     */
    case Key_0:
        if (e->state() & Qt::Keypad) 
            k = CelestiaCore::Key_NumPad0;
        break;
    case Key_1:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad1;
        break;
    case Key_2:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad2;
        break;
    case Key_3:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad3;
        break;
    case Key_4:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad4;
        break;
    case Key_5:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad5;
        break;
    case Key_6:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad6;
        break;
    case Key_7:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad7;
        break;
    case Key_8:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad8;
        break;    
    case Key_9:
        if (e->state() & Qt::Keypad)
            k = CelestiaCore::Key_NumPad9;
        break;   
    case Key_A:
        k = 'A';
        break;
    case Key_Z:
        k = 'Z';
        break;
    }

    if (k >= 0)
    {
        int buttons = 0;
        if (e->state() & ShiftButton)
            buttons |= CelestiaCore::ShiftKey;

        if (down)
            appCore->keyDown(k, buttons);
        else
            appCore->keyUp(k);
        return (k < 'A' || k > 'Z');
    }
    else
    {
        return false;
    }
}


void KdeGlWidget::keyPressEvent( QKeyEvent* e )
{
    static bool inputMode = false;
    switch (e->key())
    {
    case Key_Escape:
        if (inputMode)
        {
            for (unsigned int n=0; n<actionColl->count(); n++) {
                if (actionColl->action(n)->shortcut().seq(0).key(0).modFlags()==0)
                    actionColl->action(n)->setEnabled(true);
            }
            inputMode = !inputMode;
        }
        appCore->charEntered('\033');
        break;
    case Key_BackTab:
        appCore->charEntered(CelestiaCore::Key_BackTab);
        break;
    case Key_Q:
        if( e->state() == ControlButton )
        {
            parentWidget()->close();
        }
        // Intentional fallthrough if *not* Ctrl-Q

    default:
        if (!handleSpecialKey(e, true))
        {
            if ((e->text() != 0) && (e->text() != ""))
            {
                for (unsigned int i=0; i<e->text().length(); i++)
                {
                    char c = e->text().at(i).latin1();
                    if (c == 0x0D || c=='\033') {
                    	if (!inputMode) { // entering input mode
                        	for (unsigned int n=0; n<actionColl->count(); n++) {
                                	if (actionColl->action(n)->shortcut().count() > 0
                                        	&& actionColl->action(n)->shortcut().seq(0).key(0).modFlags()==0 )
                                		actionColl->action(n)->setEnabled(false);
                                }
                        } else { // living input mode
                        	for (unsigned int n=0; n<actionColl->count(); n++) {
                                	if (actionColl->action(n)->shortcut().seq(0).key(0).modFlags()==0)
	                                	actionColl->action(n)->setEnabled(true);
                                }
                        }
                    	inputMode = !inputMode;
                    }
                    if (c >= 0x20 || c == 0x0D || c== 0x08 || c=='\011') appCore->charEntered(c);
                }
            }
        }
    }


}


void KdeGlWidget::keyReleaseEvent( QKeyEvent* e )
{
    handleSpecialKey(e, false);
}
