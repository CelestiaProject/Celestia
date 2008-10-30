// qttimetoolbar.h
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

#ifndef _QTTIMETOOLBAR_H_
#define _QTTIMETOOLBAR_H_

#include <QToolBar>

class CelestiaCore;

class TimeToolBar : public QToolBar
{
Q_OBJECT

 public:
    TimeToolBar(CelestiaCore* _appCore,
                const QString& title,
                QWidget* parent = NULL);
    ~TimeToolBar();

 public slots:
    void slotPauseTime();
    void slotReverseTime();
    void slotRealTime();
    void slotDoubleTime();
    void slotHalfTime();
    void slotFaster();
    void slotSlower();
    void slotCurrentTime();

 private:
    CelestiaCore* appCore;
};

#endif // _QTTIMETOOLBAR_H_
