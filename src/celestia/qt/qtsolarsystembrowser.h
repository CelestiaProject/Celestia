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
#include "celengine/selection.h"

class QAbstractItemModel;
class QTreeView;
class QCheckBox;
class QComboBox;
class QItemSelection;
class ColorSwatchWidget;
class CelestiaCore;
class InfoPanel;

class SolarSystemTreeModel;

class SolarSystemBrowser : public QWidget
{
Q_OBJECT

 public:
    SolarSystemBrowser(CelestiaCore* _appCore, QWidget* parent, InfoPanel* infoPanel);
    ~SolarSystemBrowser() = default;

 public slots:
    void slotRefreshTree();
    void slotContextMenu(const QPoint& pos);
    void slotMarkSelected();
    void slotUnmarkSelected();
    //void slotChooseMarkerColor();
    void slotClearMarkers();
    void slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel);

 signals:
    void selectionContextMenuRequested(const QPoint& pos, Selection& sel);

 private:
    void setMarkerColor(QColor color);

 private:
    CelestiaCore* appCore;

    SolarSystemTreeModel* solarSystemModel{nullptr};
    QTreeView* treeView{nullptr};

    QCheckBox* planetsButton{nullptr};
    QCheckBox* asteroidsButton{nullptr};
    QCheckBox* spacecraftsButton{nullptr};
    QCheckBox* cometsButton{nullptr};

    QCheckBox* groupCheckBox{nullptr};

    QComboBox* markerSymbolBox{nullptr};
    QComboBox* markerSizeBox{nullptr};
    QCheckBox* labelMarkerBox{nullptr};

    ColorSwatchWidget* colorSwatch{nullptr};

    InfoPanel* infoPanel{nullptr};
};

#endif // _QTSOLARSYSTEMBROWSER_H_
