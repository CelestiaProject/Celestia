// qtcelestialbrowser.h
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Dockable celestial browser widget.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTCELESTIALBROWSER_H_
#define _QTCELESTIALBROWSER_H_

#include <QWidget>

class QAbstractItemModel;
class QTreeView;
class CelestiaCore;
class QRadioButton;

class CelestialBrowser : public QWidget
{
Q_OBJECT

 public:
    CelestialBrowser(CelestiaCore* _appCore, QWidget* parent);
    ~CelestialBrowser();

 public slots:
    void slotRefreshTable();

 private:
    CelestiaCore* appCore;

    QAbstractItemModel* starModel;
    QTreeView* treeView;

    QRadioButton* closestButton;
    QRadioButton* brightestButton;
    QRadioButton* withPlanetsButton;
};

#endif // _QTCELESTIALBROWSER_H_
