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

class QTreeView;
class QRadioButton;
class QDateEdit;
class QComboBox;
class QProgressDialog;
class EventTableModel;
class Universe;


class EclipseFinderWatcher
{
public:
    virtual ~EclipseFinderWatcher() {};
    
    enum Status
    {
        ContinueOperation = 0,
        AbortOperation = 1,
    };

    virtual Status eclipseFinderProgressUpdate(double t) = 0;
};


class EventFinder : public QDockWidget, EclipseFinderWatcher
{
Q_OBJECT

 public:
    EventFinder(Universe* u, const QString& title, QWidget* parent);
    ~EventFinder();

    EclipseFinderWatcher::Status eclipseFinderProgressUpdate(double t);
    
 public slots:
    void slotFindEclipses();

 private:
    Universe* universe;

    QRadioButton* solarOnlyButton;
    QRadioButton* lunarOnlyButton;
    QRadioButton* allEclipsesButton;

    QDateEdit* startDateEdit;
    QDateEdit* endDateEdit;

    QComboBox* planetSelect;

    EventTableModel* model;
    QTreeView* eventTable;

    QProgressDialog* progress;
    double searchSpan;
    double lastProgressUpdate;
};

#endif // _QTEVENTFINDER_H_
