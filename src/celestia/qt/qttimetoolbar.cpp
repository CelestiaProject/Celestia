// qttimetoolbar.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Time control toolbar for Qt4 front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QAction>
#include <QIcon>
#include "celestia/celestiacore.h"
#include "qttimetoolbar.h"


TimeToolBar::TimeToolBar(CelestiaCore* _appCore,
                         const QString& title,
                         QWidget* parent) :
    QToolBar(title, parent),
    appCore(_appCore)
{
#if 0
    // Text-only buttons
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    QAction* reverseTimeAction = new QAction(QString("< >"), this);
    QAction* slowTimeAction = new QAction(QString("<<|"), this);
    QAction* halfTimeAction = new QAction(QString("<|"), this);
    QAction* pauseAction = new QAction(QString("||"), this);
    QAction* realTimeAction = new QAction(QString(">"), this);
    QAction* doubleTimeAction = new QAction(QString(">>"), this);
    QAction* fastTimeAction = new QAction(QString(">>>"), this);
#endif

    QAction* reverseTimeAction = new QAction(QIcon("icons/qt/media-eject.png"),
                                             tr("Reverse time"), this);
    connect(reverseTimeAction, SIGNAL(triggered()), this, SLOT(slotReverseTime()));
    addAction(reverseTimeAction);

    QAction* slowTimeAction = new QAction(QIcon("icons/qt/media-skip-backward.png"),
                                          tr("10x slower"), this);
    connect(slowTimeAction, SIGNAL(triggered()), this, SLOT(slotSlower()));
    addAction(slowTimeAction);
    
    QAction* halfTimeAction = new QAction(QIcon("icons/qt/media-seek-backward.png"),
                                          tr("2x slower"), this);
    connect(halfTimeAction, SIGNAL(triggered()), this, SLOT(slotHalfTime()));
    addAction(halfTimeAction);

    QAction* pauseAction = new QAction(QIcon("icons/qt/media-playback-pause.png"),
                                       tr("Pause time"), this);
    connect(pauseAction, SIGNAL(triggered()), this, SLOT(slotPauseTime()));
    addAction(pauseAction);

    QAction* realTimeAction = new QAction(QIcon("icons/qt/media-playback-start.png"),
                                          tr("Real time"), this);
    connect(realTimeAction, SIGNAL(triggered()), this, SLOT(slotRealTime()));
    addAction(realTimeAction);

    QAction* doubleTimeAction = new QAction(QIcon("icons/qt/media-seek-forward.png"),
                                            tr("2x faster"), this);
    connect(doubleTimeAction, SIGNAL(triggered()), this, SLOT(slotDoubleTime()));
    addAction(doubleTimeAction);

    QAction* fastTimeAction = new QAction(QIcon("icons/qt/media-skip-forward.png"),
                                          tr("10x faster"), this);
    connect(fastTimeAction, SIGNAL(triggered()), this, SLOT(slotFaster()));
    addAction(fastTimeAction);
}


TimeToolBar::~TimeToolBar()
{
}


void TimeToolBar::slotPauseTime()
{
    appCore->getSimulation()->setPauseState(!appCore->getSimulation()->getPauseState());
}


void TimeToolBar::slotReverseTime()
{
    appCore->getSimulation()->setTimeScale(-appCore->getSimulation()->getTimeScale());
}


void TimeToolBar::slotRealTime()
{
    appCore->getSimulation()->setTimeScale(1.0);
}


void TimeToolBar::slotDoubleTime()
{
    appCore->getSimulation()->setTimeScale(2.0 * appCore->getSimulation()->getTimeScale());
}


void TimeToolBar::slotHalfTime()
{
    appCore->getSimulation()->setTimeScale(0.5 * appCore->getSimulation()->getTimeScale());
}


void TimeToolBar::slotFaster()
{
    appCore->getSimulation()->setTimeScale(10.0 * appCore->getSimulation()->getTimeScale());
}


void TimeToolBar::slotSlower()
{
    appCore->getSimulation()->setTimeScale(0.1 * appCore->getSimulation()->getTimeScale());
}



