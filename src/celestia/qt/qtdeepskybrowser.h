// qtdeepskybrowser.h
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Deep sky browser widget for Qt4 front-end.
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

class DeepSkyBrowser : public QWidget
{
    Q_OBJECT

public:
    DeepSkyBrowser(CelestiaCore* _appCore, QWidget* parent, InfoPanel* infoPanel);
    ~DeepSkyBrowser() = default;

public slots:
    void slotRefreshTable();
    void slotContextMenu(const QPoint& pos);
    void slotMarkSelected();
    void slotUnmarkSelected();
    void slotClearMarkers();
    void slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel);

signals:
    void selectionContextMenuRequested(const QPoint& pos, Selection& sel);

private:
    class DSOTableModel;

    CelestiaCore* appCore;

    DSOTableModel* dsoModel{nullptr};
    QTreeView* treeView{nullptr};

    QLabel* searchResultLabel{nullptr};

    QRadioButton* globularsButton{nullptr};
    QRadioButton* galaxiesButton{nullptr};
    QRadioButton* nebulaeButton{nullptr};
    QRadioButton* openClustersButton{nullptr};

    QLineEdit* objectTypeFilterBox{nullptr};

    QComboBox* markerSymbolBox{nullptr};
    QComboBox* markerSizeBox{nullptr};
    QCheckBox* labelMarkerBox{nullptr};

    ColorSwatchWidget* colorSwatch{nullptr};

    InfoPanel* infoPanel{nullptr};
};

} // end namespace celestia::qt
