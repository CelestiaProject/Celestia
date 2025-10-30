// qtcelestialbrowser.h
//
// Copyright (C) 2007-present, the Celestia Development Team
//
// Dockable celestial browser widget.
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
class QLabel;
class QLineEdit;
class QPoint;
class QRadioButton;
class QTreeView;

class CelestiaCore;
class Selection;

namespace celestia::qt
{

class ColorSwatchWidget;
class InfoPanel;

class CelestialBrowser : public QWidget
{
    Q_OBJECT

public:
    CelestialBrowser(CelestiaCore* _appCore, QWidget* parent, InfoPanel* infoPanel);
    ~CelestialBrowser() = default;

public slots:
    void slotUncheckMultipleFilterBox();
    void slotUncheckBarycentersFilterBox();
    void slotRefreshTable();
    void slotContextMenu(const QPoint& pos);
    void slotMarkSelected();
    void slotUnmarkSelected();
    void slotClearMarkers();
    void slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel);

signals:
    void selectionContextMenuRequested(const QPoint& pos, Selection& sel);

private:
    class StarTableModel;

    CelestiaCore* appCore;

    StarTableModel* starModel{nullptr};
    QTreeView* treeView{nullptr};

    QLabel* searchResultLabel{nullptr};

    QRadioButton* closestButton{nullptr};
    QRadioButton* brightestButton{nullptr};

    QCheckBox* withPlanetsFilterBox{nullptr};
    QCheckBox* multipleFilterBox{nullptr};
    QCheckBox* barycentersFilterBox{nullptr};
    QLineEdit* spectralTypeFilterBox{nullptr};

    QComboBox* markerSymbolBox{nullptr};
    QComboBox* markerSizeBox{nullptr};
    QCheckBox* labelMarkerBox{nullptr};

    ColorSwatchWidget* colorSwatch{nullptr};
    InfoPanel* infoPanel{nullptr};
};

} // end namespace celestia::qt
