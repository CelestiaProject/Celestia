/***************************************************************************
                          selectionpopup.h  -  description
                             -------------------
    begin                : 2003-05-06
    copyright            : (C) 2002 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SELECTIONPOPUP_H
#define SELECTIONPOPUP_H

#include <kpopupmenu.h>
#include "celestia/celestiacore.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>

class SelectionPopup : public KPopupMenu {
Q_OBJECT
public:
    SelectionPopup(QWidget* parent, CelestiaCore* appCore, Selection sel);
    ~SelectionPopup();
    void init();
    void process(int id);

protected:
    CelestiaCore* appCore;
    const char* getSelectionName(const Selection& sel) const;
    Selection getSelectionFromId(int id);
    void insert(KPopupMenu* popup, Selection sel, bool showSubObjects = true);
    Selection sel;
    std::vector< std::pair<int, Selection> > baseIds;
    int baseId;
    void insertPlanetaryMenu(KPopupMenu* popup, const string& parentName, const PlanetarySystem* psys);
};

#endif // SELECTIONPOPUP_H
