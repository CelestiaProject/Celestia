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

#include "celestia/celestiacore.h"
#include "qttimetoolbar.h"
#include <QAction>
#include <QIcon>
#include <QDateTime>


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
    reverseTimeAction->setToolTip(_("Reverse time"));
    QAction* slowTimeAction = new QAction(QString("<<|"), this);
    slowTimeAction->setToolTip(_("10x slower"));
    QAction* halfTimeAction = new QAction(QString("<|"), this);
    halfTimeAction->setToolTip(_("2x slower"));
    QAction* pauseAction = new QAction(QString("||"), this);
    pauseAction->setToolTip(_("Pause time"));
    QAction* realTimeAction = new QAction(QString(">"), this);
    realTimeAction->setToolTip(_("Real time"));
    QAction* doubleTimeAction = new QAction(QString(">>"), this);
    doubleTimeAction->setToolTip(_("2x faster"));
    QAction* fastTimeAction = new QAction(QString(">>>"), this);
    fastTimeAction->setToolTip(_("10x faster"));
#else
    QAction* reverseTimeAction = new QAction(QIcon(":/icons/time-reverse.png"),
                                             _("Reverse time"), this);
    QAction* slowTimeAction = new QAction(QIcon(":/icons/time-slower.png"),
                                          _("10x slower"), this);
    QAction* halfTimeAction = new QAction(QIcon(":/icons/time-half.png"),
                                          _("2x slower"), this);
    QAction* pauseAction = new QAction(QIcon(":/icons/time-pause.png"),
                                       _("Pause time"), this);
    QAction* realTimeAction = new QAction(QIcon(":/icons/time-realtime.png"),
                                          _("Real time"), this);
    QAction* doubleTimeAction = new QAction(QIcon(":/icons/time-double.png"),
                                            _("2x faster"), this);
    QAction* fastTimeAction = new QAction(QIcon(":/icons/time-faster.png"),
                                          _("10x faster"), this);
    QAction* currentTimeAction = new QAction(QIcon(":icons/time-currenttime.png"),
                                             _("Set to current time"), this);
#endif
    connect(reverseTimeAction, SIGNAL(triggered()), this, SLOT(slotReverseTime()));
    addAction(reverseTimeAction);

    connect(slowTimeAction, SIGNAL(triggered()), this, SLOT(slotSlower()));
    addAction(slowTimeAction);
    
    connect(halfTimeAction, SIGNAL(triggered()), this, SLOT(slotHalfTime()));
    addAction(halfTimeAction);

    connect(pauseAction, SIGNAL(triggered()), this, SLOT(slotPauseTime()));
    addAction(pauseAction);

    connect(realTimeAction, SIGNAL(triggered()), this, SLOT(slotRealTime()));
    addAction(realTimeAction);

    connect(doubleTimeAction, SIGNAL(triggered()), this, SLOT(slotDoubleTime()));
    addAction(doubleTimeAction);

    connect(fastTimeAction, SIGNAL(triggered()), this, SLOT(slotFaster()));
    addAction(fastTimeAction);

    connect(currentTimeAction, SIGNAL(triggered()), this, SLOT(slotCurrentTime()));
    addAction(currentTimeAction);
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


void TimeToolBar::slotCurrentTime()
{
    QDateTime now = QDateTime::currentDateTime().toUTC();
    QDate d = now.date();
    QTime t = now.time();
    astro::Date celDate(d.year(), d.month(), d.day());
    celDate.hour = t.hour();
    celDate.minute = t.minute();
    celDate.seconds = (double) t.second() + t.msec() / 1000.0;

    appCore->getSimulation()->setTime(astro::UTCtoTDB(celDate));
}
