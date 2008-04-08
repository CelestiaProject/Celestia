// qtselectionpopup.h
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Context menu for objects in the 3D view.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTSELECTIONPOPUP_H_
#define _QTSELECTIONPOPUP_H_

#include <QMenu>
#include <celengine/selection.h>

class CelestiaCore;

class SelectionPopup : public QMenu
{
Q_OBJECT

 public:
    SelectionPopup(const Selection& sel,
                   CelestiaCore* _appCore,
                   QWidget* parent);
    ~SelectionPopup();

 public slots:
    void slotSelect();
    void slotCenterSelection();
    void slotGotoSelection();
    void slotFollowSelection();
    void slotSyncOrbitSelection();
    void slotSelectAlternateSurface();
    void slotSelectChildObject();
    void slotMark();
    void slotToggleBodyAxes();
    void slotToggleFrameAxes();
    void slotToggleSunDirection();
    void slotToggleVelocityVector();
    void slotToggleSpinVector();
    void slotToggleFrameCenterDirection();
    void slotTogglePlanetographicGrid();
    void slotToggleTerminator();
    void slotGotoStartDate();
    void slotGotoEndDate();
    void slotInfo();
	void slotToggleVisibility(bool);

    void popupAtGoto(const QPoint& p);
    void popupAtCenter(const QPoint& p);

 signals:
    void selectionInfoRequested(Selection& sel);

 private:
    QMenu* createMarkMenu();
    QMenu* createReferenceVectorMenu();
    QMenu* createAlternateSurfacesMenu();
    QMenu* createObjectMenu(PlanetarySystem* sys, unsigned int classification);
    void addObjectMenus(PlanetarySystem* sys);

 private:
    Selection selection;
    CelestiaCore* appCore;
    QAction* centerAction;
    QAction* gotoAction;
};

#endif // _QTSELECTIONPOPUP_H_
