/***************************************************************************
                          kdeglwidget.h  -  description
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

#ifndef KDEGLWIDGET_H
#define KDEGLWIDGET_H

#include <GL/glew.h>
#include <qgl.h>
#include <qevent.h>
#include <kaction.h>

#include "celestia/celestiacore.h"
#include "celengine/simulation.h"
#include <celengine/starbrowser.h>
#include <string>
#include <vector>


/**
  *@author Christophe Teyssier
  */

class KdeGlWidget : public QGLWidget, public CelestiaCore::CursorHandler {
    Q_OBJECT

public:
    KdeGlWidget( QWidget* parent, const char* name, CelestiaCore* core);
    ~KdeGlWidget();

    void setCursorShape(CelestiaCore::CursorShape);
    CelestiaCore::CursorShape getCursorShape() const;
    
protected:

    void initializeGL();
    void paintGL();
    void resizeGL( int w, int h );
    virtual void mouseMoveEvent( QMouseEvent* m );
    virtual void mousePressEvent( QMouseEvent* m );
    virtual void mouseReleaseEvent( QMouseEvent* m );
    virtual void wheelEvent( QWheelEvent* w );
    virtual void keyPressEvent( QKeyEvent* e );
    virtual void keyReleaseEvent( QKeyEvent* e );
    bool handleSpecialKey(QKeyEvent* e, bool down);

private:

    CelestiaCore* appCore;
    Renderer* appRenderer;
    Simulation* appSim;
    int lastX;
    int lastY;
    CelestiaCore::CursorShape currentCursor;

    KActionCollection* actionColl;

};

#endif
