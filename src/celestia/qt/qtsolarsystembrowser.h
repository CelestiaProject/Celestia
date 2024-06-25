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

#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QItemSelection;
class QPoint;
class QTreeView;

class CelestiaCore;
class Selection;

namespace celestia::qt
{

class ColorSwatchWidget;
class InfoPanel;

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
    void slotUnmarkSelected() const;
    void slotClearMarkers() const;
    void slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel);

signals:
    void selectionContextMenuRequested(const QPoint& pos, Selection& sel);

private:
    class SolarSystemTreeModel;

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

} // end namespace celestia::qt
