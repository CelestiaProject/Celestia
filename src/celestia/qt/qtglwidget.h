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

#pragma once

#include <memory>

#include <celengine/glsupport.h>

#include <QOpenGLWidget>

#include <celestia/celestiacore.h>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QWidget;

class Renderer;

namespace celestia::qt
{

class DragHandler;

/**
  *@author Christophe Teyssier
  */

class CelestiaGlWidget : public QOpenGLWidget, public CelestiaCore::CursorHandler
{
    Q_OBJECT

public:
    CelestiaGlWidget(QWidget* parent, const char* name, CelestiaCore* core);
    ~CelestiaGlWidget() override;

    void setCursorShape(CelestiaCore::CursorShape) override;
    CelestiaCore::CursorShape getCursorShape() const override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseMoveEvent(QMouseEvent* m) override;
    void mousePressEvent(QMouseEvent* m) override;
    void mouseReleaseEvent(QMouseEvent* m) override;
    void wheelEvent(QWheelEvent* w) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e ) override;

    QSize sizeHint() const override;

private:
    bool handleSpecialKey(QKeyEvent* e, bool down);

    CelestiaCore* appCore;
    Renderer* appRenderer;
    int lastX{ 0 };
    int lastY{ 0 };
    bool cursorVisible;
    std::unique_ptr<DragHandler> dragHandler;
    CelestiaCore::CursorShape currentCursor;
};

} // end namespace celestia::qt
