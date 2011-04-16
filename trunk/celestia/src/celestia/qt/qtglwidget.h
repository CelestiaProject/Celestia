// qtpreferencesdialog.h
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Preferences dialog for Celestia's Qt front-end. Based on
// kdeglwidget.h by Christophe Teyssier.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef QTGLWIDGET_H
#define QTGLWIDGET_H

#include <GL/glew.h>

#include <QGLWidget>

#include "celestia/celestiacore.h"
#include "celengine/simulation.h"
#include <celengine/starbrowser.h>
#include <string>
#include <vector>

/**
  *@author Christophe Teyssier
  */

class CelestiaGlWidget : public QGLWidget, public CelestiaCore::CursorHandler
{
    Q_OBJECT

public:
    CelestiaGlWidget(QWidget* parent, const char* name, CelestiaCore* core);
    ~CelestiaGlWidget();

    void setCursorShape(CelestiaCore::CursorShape);
    CelestiaCore::CursorShape getCursorShape() const;
    
protected:
    void initializeGL();
    void paintGL();
    void resizeGL( int w, int h );
    virtual void mouseMoveEvent(QMouseEvent* m );
    virtual void mousePressEvent(QMouseEvent* m );
    virtual void mouseReleaseEvent(QMouseEvent* m );
    virtual void wheelEvent(QWheelEvent* w );
    virtual void keyPressEvent(QKeyEvent* e );
    virtual void keyReleaseEvent(QKeyEvent* e );
    bool handleSpecialKey(QKeyEvent* e, bool down);

    virtual QSize sizeHint() const;

private:

    CelestiaCore* appCore;
    Renderer* appRenderer;
    Simulation* appSim;
    int lastX;
    int lastY;
    bool cursorVisible;
    QPoint saveCursorPos;
    CelestiaCore::CursorShape currentCursor;

    //KActionCollection* actionColl;

};

#endif
