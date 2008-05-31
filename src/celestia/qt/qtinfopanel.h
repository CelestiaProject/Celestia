// qtcelestiaactions.h
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Dockable information panel for Qt4 UI.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _INFOPANEL_H_
#define _INFOPANEL_H_

#include <QDockWidget>
#include <QTextStream>
#include "celengine/selection.h"

class QTextBrowser;
class Universe;

class InfoPanel : public QDockWidget
{
 public:
    InfoPanel(const QString& title, QWidget* parent);
    ~InfoPanel();

    void buildInfoPage(Selection sel, Universe*, double tdb);

 private:
    void pageHeader(QTextStream&);
    void pageFooter(QTextStream&);
    void buildSolarSystemBodyPage(const Body* body, double tdb, QTextStream&);
    void buildStarPage(const Star* star, const Universe* u, double tdb, QTextStream&);
    void buildDSOPage(const DeepSkyObject* dso, const Universe* u, QTextStream&);

 public:
    QTextBrowser* textBrowser;
};

#endif // _INFOPANEL_H_
