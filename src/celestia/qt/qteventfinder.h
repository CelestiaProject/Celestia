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

#pragma once

#include <QDockWidget>
#include <QElapsedTimer>

#include <celestia/eclipsefinder.h>

class QComboBox;
class QDateEdit;
class QMenu;
class QPoint;
class QProgressDialog;
class QRadioButton;
class QString;
class QTreeView;
class QWidget;

class CelestiaCore;

namespace celestia::qt
{

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
    class EventTableModel;

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

    QElapsedTimer searchTimer;

    const Eclipse* activeEclipse{ nullptr };
};

} // end namespace celestia::qt
