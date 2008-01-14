// qtsolarsystembrowser.h
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Solar system browser widget for Qt4 front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTSOLARSYSTEMBROWSER_H_
#define _QTSOLARSYSTEMBROWSER_H_

#include <QWidget>

class QAbstractItemModel;
class QTreeView;
class CelestiaCore;

class SolarSystemTreeModel;

class SolarSystemBrowser : public QWidget
{
Q_OBJECT

 public:
    SolarSystemBrowser(CelestiaCore* _appCore, QWidget* parent);
    ~SolarSystemBrowser();

 public slots:
    void slotRefreshTree();
    void slotContextMenu(const QPoint& pos);
    void slotMarkSelected();
    //void slotChooseMarkerColor();

 private:
    void setMarkerColor(QColor color);

 private:
    CelestiaCore* appCore;

    SolarSystemTreeModel* solarSystemModel;
    QTreeView* treeView;
};

#endif // _QTSOLARSYSTEMBROWSER_H_
