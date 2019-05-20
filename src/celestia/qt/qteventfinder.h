// qteventfinder.h
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Qt4 interface for the celestial event finder.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTEVENTFINDER_H_
#define _QTEVENTFINDER_H_

#include <QDockWidget>
#include <QTime>
#include <celestia/eclipsefinder.h>

class QTreeView;
class QRadioButton;
class QDateEdit;
class QComboBox;
class QProgressDialog;
class QMenu;
class EventTableModel;
class CelestiaCore;

class EventFinder : public QDockWidget, EclipseFinderWatcher
{
    Q_OBJECT

 public:
    EventFinder(CelestiaCore* _appCore, const QString& title, QWidget* parent);
    ~EventFinder() = default;

    EclipseFinderWatcher::Status eclipseFinderProgressUpdate(double t);

 public slots:
    void slotFindEclipses();
    void slotContextMenu(const QPoint&);

    void slotSetEclipseTime();
    void slotViewNearEclipsed();
    void slotViewEclipsedSurface();
    void slotViewOccluderSurface();
    void slotViewBehindOccluder();

 private:
    CelestiaCore* appCore;

    QRadioButton* solarOnlyButton{ nullptr };
    QRadioButton* lunarOnlyButton{ nullptr };
    QRadioButton* allEclipsesButton{ nullptr };

    QDateEdit* startDateEdit{ nullptr };
    QDateEdit* endDateEdit{ nullptr };

    QComboBox* planetSelect{ nullptr };

    EventTableModel* model{ nullptr };
    QTreeView* eventTable{ nullptr };
    QMenu* contextMenu{ nullptr };

    QProgressDialog* progress{ nullptr };
    double searchSpan{ 0.0 };
    double lastProgressUpdate{ 0.0 };

    QTime searchTimer;

    const Eclipse* activeEclipse{ nullptr };
};

#endif // _QTEVENTFINDER_H_
