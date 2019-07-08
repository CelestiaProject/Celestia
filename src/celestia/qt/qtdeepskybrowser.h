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

#ifndef _QTDEEPSKYBROWSER_H_
#define _QTDEEPSKYBROWSER_H_

#include <QWidget>
#include "celengine/selection.h"

class QAbstractItemModel;
class QItemSelection;
class QTreeView;
class QRadioButton;
class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class ColorSwatchWidget;
class CelestiaCore;
class InfoPanel;

class DSOTableModel;

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
    CelestiaCore* appCore;

    DSOTableModel* dsoModel{nullptr};
    QTreeView* treeView{nullptr};

    QLabel* searchResultLabel{nullptr};

    QCheckBox* globularsButton{nullptr};
    QCheckBox* galaxiesButton{nullptr};
    QCheckBox* nebulaeButton{nullptr};
    QCheckBox* openClustersButton{nullptr};

    QLineEdit* objectTypeFilterBox{nullptr};

    QComboBox* markerSymbolBox{nullptr};
    QComboBox* markerSizeBox{nullptr};
    QCheckBox* labelMarkerBox{nullptr};

    ColorSwatchWidget* colorSwatch{nullptr};

    InfoPanel* infoPanel{nullptr};
};

#endif // _QTDEEPSKYBROWSER_H_
