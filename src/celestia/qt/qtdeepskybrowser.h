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
class QTreeView;
class QRadioButton;
class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class ColorSwatchWidget;
class CelestiaCore;

class DSOTableModel;

class DeepSkyBrowser : public QWidget
{
Q_OBJECT

 public:
    DeepSkyBrowser(CelestiaCore* _appCore, QWidget* parent);
    ~DeepSkyBrowser();

 public slots:
    void slotRefreshTable();
    void slotContextMenu(const QPoint& pos);
    void slotMarkSelected();
    void slotClearMarkers();

 signals:
    void selectionContextMenuRequested(const QPoint& pos, Selection& sel);

 private:
    CelestiaCore* appCore;

    DSOTableModel* dsoModel;
    QTreeView* treeView;

    QLabel* searchResultLabel;

    QRadioButton* globularsButton;
    QRadioButton* galaxiesButton;
    QRadioButton* nebulaeButton;
    QRadioButton* openClustersButton;

    QLineEdit* objectTypeFilterBox;

    QComboBox* markerSymbolBox;
    QComboBox* markerSizeBox;
    QCheckBox* labelMarkerBox;

    ColorSwatchWidget* colorSwatch;
};

#endif // _QTDEEPSKYBROWSER_H_
