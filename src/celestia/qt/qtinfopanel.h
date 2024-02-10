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

#pragma once

#include <QDockWidget>

class QItemSelection;
class QModelIndex;
class QString;
class QTextBrowser;
class QTextStream;
class QWidget;

class Body;
class CelestiaCore;
class DeepSkyObject;
class Selection;
class Star;
class Universe;

namespace celestia::qt
{

class ModelHelper
{
 public:
    virtual ~ModelHelper() = default;
    virtual Selection itemForInfoPanel(const QModelIndex&) = 0;
};


class InfoPanel : public QDockWidget
{
 public:
    InfoPanel(CelestiaCore* appCore, const QString& title, QWidget* parent);
    ~InfoPanel() = default;

    void buildInfoPage(Selection sel, Universe*, double tdb);

    void updateHelper(ModelHelper*, const QItemSelection&, const QItemSelection&);

 private:
    void pageHeader(QTextStream&);
    void pageFooter(QTextStream&);
    void buildSolarSystemBodyPage(const Body* body, double tdb, QTextStream&);
    void buildStarPage(const Star* star, const Universe* u, double tdb, QTextStream&);
    void buildDSOPage(const DeepSkyObject* dso, const Universe* u, QTextStream&);

    CelestiaCore* appCore;

 public:
    QTextBrowser* textBrowser{nullptr};
};

} // end namespace celestia::qt
