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
#include "qtselectionpopup.h"

class QAbstractItemModel;
class QTreeView;
class QRadioButton;
class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class ColorSwatchWidget;
class CelestiaCore;

class StarTableModel;

class CelestialBrowser : public QWidget
{
Q_OBJECT

 public:
    CelestialBrowser(CelestiaCore* _appCore, QWidget* parent);
    ~CelestialBrowser();

 public slots:
    void slotRefreshTable();
    void slotContextMenu(const QPoint& pos);
    void slotMarkSelected();
    void slotClearMarkers();

 signals:
    void selectionContextMenuRequested(const QPoint& pos, Selection& sel);

 private:
    CelestiaCore* appCore;

    StarTableModel* starModel;
    QTreeView* treeView;

    QLabel* searchResultLabel;

    QRadioButton* closestButton;
    QRadioButton* brightestButton;

    QCheckBox* withPlanetsFilterBox;
    QCheckBox* multipleFilterBox;
    QLineEdit* spectralTypeFilterBox;

    QComboBox* markerSymbolBox;
    QComboBox* markerSizeBox;
    QCheckBox* labelMarkerBox;

    ColorSwatchWidget* colorSwatch;
};

#endif // _QTCELESTIALBROWSER_H_
