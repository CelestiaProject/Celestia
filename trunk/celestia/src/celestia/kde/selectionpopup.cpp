/***************************************************************************
                          selectionpopup.cpp  -  description
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

#include "selectionpopup.h"

#include <fstream>
#include <sstream>

#include <qlabel.h>

#include <klocale.h>
#include <kurl.h>
#include <krun.h>

#include "celutil/utf8.h"

SelectionPopup::SelectionPopup(QWidget* parent, CelestiaCore* _appCore, Selection _sel)
:KPopupMenu(parent),
 appCore(_appCore),
 sel(_sel)
{

}

void SelectionPopup::init()
{
    QLabel *lab = new QLabel("", this);
    QPalette pal( lab->palette() );
    pal.setColor( QPalette::Normal, QColorGroup::Background, QColor( "White" ) );
    pal.setColor( QPalette::Normal, QColorGroup::Foreground, QColor( "Black" ) );
    pal.setColor( QPalette::Inactive, QColorGroup::Foreground, QColor( "Black" ) );

    QFont rsFont = lab->font();
    rsFont.setPointSize( rsFont.pointSize() - 2 );

    Simulation *sim = appCore->getSimulation();
    Vec3d v = sel.getPosition(sim->getTime()) - sim->getObserver().getPosition();

    if (sel.body() != NULL)
    {
        insertTitle(QString::fromUtf8(sel.body()->getName().c_str()), 0, 0);
        insert(this, sel, MENUMAXSIZE);
    }
    else if (sel.star() != NULL)
    {
        std::string name = sim->getUniverse()->getStarCatalog()->getStarName(*sel.star());

        double distance = v.length() * 1e-6;
        char buff[50];

        ostringstream o;

        if (abs(distance) >= astro::AUtoLightYears(1000.0f))
            sprintf(buff, "%.3f ly", distance);
        else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
            sprintf(buff, "%.3f au", astro::lightYearsToAU(distance));
        else if (abs(distance) > astro::kilometersToLightYears(1.0f))
            sprintf(buff, "%.3f km", astro::lightYearsToKilometers(distance));
        else
            sprintf(buff, "%.3f m", astro::lightYearsToKilometers(distance) * 1000.0f);

        o << i18n("Distance: ") << buff << "\n";

        o << i18n("Abs (app) mag: ");

        sprintf(buff, "%.2f (%.2f)",
                   sel.star()->getAbsoluteMagnitude(),
                   astro::absToAppMag(sel.star()->getAbsoluteMagnitude(),
                                      (float) distance));
        o << buff << "\n";

        o << i18n("Class: ");
        sprintf(buff, "%s", sel.star()->getStellarClass().str().c_str());
        o << buff << "\n";

        o << i18n("Surface Temp: ");
        sprintf(buff, "%.0f K", sel.star()->getTemperature());
        o << buff << "\n";

        o << i18n("Radius: ");
        sprintf(buff, "%.2f Rsun", sel.star()->getRadius() / 696000.0f);
        o << buff;

        QLabel *starDetails = new QLabel(QString(o.str().c_str()), this);
        starDetails->setPalette(pal);
        starDetails->setFont(rsFont);

        insertTitle(QString::fromUtf8(ReplaceGreekLetterAbbr(name).c_str()), 0, 0);
        insertItem(starDetails);
        insertSeparator();
        insert(this, sel, MENUMAXSIZE);
    }
    else if (sel.deepsky() != NULL)
    {
        insertTitle(QString::fromUtf8(sel.deepsky()->getName().c_str()), 0);
        insert(this, sel, MENUMAXSIZE);
    }

    if (sim->getUniverse() != NULL)
    {
        MarkerList* markers = sim->getUniverse()->getMarkers();
        if (markers->size() > 0)
        {
            KPopupMenu *markMenu = new KPopupMenu(this);
            int j=1;
            for (std::vector<Marker>::iterator i = markers->begin(); i < markers->end() && j < MENUMAXSIZE; i++)
            {
                KPopupMenu *objMenu = new KPopupMenu(this);
                insert(objMenu, (*i).getObject(), (2 * MENUMAXSIZE + j) * MENUMAXSIZE, false);
                markMenu->insertItem(getSelectionName((*i).getObject()), objMenu);
                j++;
            }
            insertItem(i18n("Marked objects"), markMenu);
        }
    }
}

void SelectionPopup::process(int id)
{
    Simulation *sim = appCore->getSimulation();

    int actionId = id;
    actionId = id - (id / MENUMAXSIZE) * MENUMAXSIZE;

    int subId = id;
    int level = 1;
    while (id > MENUMAXSIZE) {
        id /= MENUMAXSIZE;
        level *= MENUMAXSIZE;
    }
    subId -= id * level;

    if (id == 1)
    {
        int selId = subId / MENUMAXSIZE;
        sel = getSelectionFromId(sel, selId);
    }
    else if (id == 2) // marked objects sub-menu
    {
        int subsubId = subId;
        int level = 1;
        while (subId > MENUMAXSIZE) {
            subId /= MENUMAXSIZE;
            level *= MENUMAXSIZE;
        }
        subsubId -= subId * level;
        MarkerList* markers = sim->getUniverse()->getMarkers();
        sel = (*markers)[subId-1].getObject();
        int selId = subsubId / MENUMAXSIZE;
        sel = getSelectionFromId(sel, selId);
    }

    if (actionId == 1) {
        sim->setSelection(sel);
        return;
    }
    if (actionId == 2) {
        sim->setSelection(sel);
        appCore->charEntered('c');
        return;
    }
    if (actionId == 3) {
        sim->setSelection(sel);
        appCore->charEntered('g');
        return;
    }
    if (actionId == 4) {
        sim->setSelection(sel);
        appCore->charEntered('f');
        return;
    }
    if (actionId == 5) {
        sim->setSelection(sel);
        appCore->charEntered('y');
        return;
    }
    if (actionId == 6) {
        QString url;
        if (sel.body() != NULL) {
            url = QString(sel.body()->getInfoURL().c_str());
            if (url == "") {
                QString name = QString(sel.body()->getName().c_str()).lower();
                url = QString("http://www.nineplanets.org/") + name + ".html";
            }
        } else if (sel.star() != NULL) {
            if (sel.star()->getCatalogNumber() != 0) {
                url = QString("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=HIP %1")
                      .arg(sel.star()->getCatalogNumber() & ~0xf0000000);
            } else {
                url = QString("http://www.nineplanets.org/sun.html");
            }
        } else if (sel.deepsky() != NULL) {
                url = QString("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=%1")
                      .arg(sel.deepsky()->getName().c_str());
        }
        KRun::runURL(url, "text/html");
        return;
    }
    if (actionId == 7)
    {
        if (sim->getUniverse() != NULL)
            sim->getUniverse()->unmarkObject(sel, 1);
        return;
    }
    if (actionId == 8)
    {
        sim->getUniverse()->unmarkAll();
        return;
    }
    if (actionId >= 10 && actionId <= 14)
    {
        if (sim->getUniverse() != NULL)
        {
            sim->getUniverse()->markObject(sel,
                                           10.0f,
                                           Color(0.0f, 1.0f, 0.0f, 0.9f),
                                           (Marker::Symbol)(actionId - 10),
                                           1);
        }
        return;
    }
    if (actionId == 20) {
        sim->getActiveObserver()->setDisplayedSurface("");
        return;
    }
    if (actionId > 20) {
        std::vector<std::string>* altSurfaces = sel.body()->getAlternateSurfaceNames();
        if (altSurfaces != NULL && altSurfaces->size() > actionId - 21)
        {
            sim->getActiveObserver()->setDisplayedSurface((*altSurfaces)[actionId - 21]);
        }
    }
}

const char* SelectionPopup::getSelectionName(const Selection& sel) const
{
    if (sel.body() != NULL)
    {
        return sel.body()->getName().c_str();
    }
    else if (sel.star() != NULL)
    {
        Simulation *sim = appCore->getSimulation();
        return sim->getUniverse()->getStarCatalog()->getStarName(*(sel.star())).c_str();
    }
    else if (sel.deepsky() != NULL)
    {
        return sel.deepsky()->getName().c_str();
    }

    return "";
}

Selection SelectionPopup::getSelectionFromId(Selection sel, int id) const
{
    if (id == 0) return sel;

    int subId = id;
    int level = 1;
    while (id > MENUMAXSIZE) {
        id /= MENUMAXSIZE;
        level *= MENUMAXSIZE;
    }
    subId -= id * level;
    if (subId < 0) subId = 0;

    if (sel.body() != NULL)
    {
        const PlanetarySystem* satellites = sel.body()->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            Body* body = satellites->getBody(id - 1);
            Selection satSel(body);
            return getSelectionFromId(satSel, subId);
        }
    }
    else if (sel.star() != NULL)
    {
        Simulation *sim = appCore->getSimulation();
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star()->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            Body* body = solarSys->getPlanets()->getBody(id - 1);
            Selection satSel(body);
            return getSelectionFromId(satSel, subId);
        }
    }

    return sel;
}

void SelectionPopup::insert(KPopupMenu *popup, Selection sel, int baseId, bool showSubObjects)
{
    popup->insertItem(i18n("&Select"), baseId + 1);
    popup->insertItem(i18n("&Center"), baseId + 2);
    popup->insertItem(i18n("&Goto"), baseId + 3);
    popup->insertItem(i18n("&Follow"), baseId + 4);
    popup->insertItem(i18n("S&ynch Orbit"), baseId + 5);
    popup->insertItem(i18n("&Info"), baseId + 6);
    popup->insertItem(i18n("Unmark &All"), baseId + 8);
    if (appCore->getSimulation()->getUniverse()->isMarked(sel, 1))
    {
        popup->insertItem(i18n("&Unmark"), baseId + 7);
    }
    else
    {
        KPopupMenu *markMenu = new KPopupMenu(this);
        markMenu->insertItem(i18n("Diamond"), baseId + 10);
        markMenu->insertItem(i18n("Triangle"), baseId + 11);
        markMenu->insertItem(i18n("Square"), baseId + 12);
        markMenu->insertItem(i18n("Plus"), baseId + 13);
        markMenu->insertItem(i18n("X"), baseId + 14);
        popup->insertItem(i18n("&Mark"), markMenu);
    }

    if (showSubObjects && sel.body() != NULL)
    {
        std::vector<std::string>* altSurfaces = sel.body()->getAlternateSurfaceNames();
        if (altSurfaces != NULL)
        {
            if (altSurfaces->size() != NULL)
            {
                KPopupMenu *surfaces = new KPopupMenu(this);
                surfaces->insertItem(i18n("Normal"), baseId + 20);
                int j=0;
                for (std::vector<std::string>::const_iterator i = altSurfaces->begin();
                     i < altSurfaces->end() && j < MENUMAXSIZE - 1; i++, j++)
                {
                    surfaces->insertItem(QString((*i).c_str()), baseId + 21 + j);
                }
                popup->insertItem(i18n("&Alternate Surfaces"), surfaces);
            }
            delete altSurfaces;
        }

        const PlanetarySystem* satellites = sel.body()->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            popup->insertSeparator();
            KPopupMenu *planetaryMenu = new KPopupMenu(this);
            for (int i = 0; i < satellites->getSystemSize() && i < MENUMAXSIZE; i++)
            {
                Body* body = satellites->getBody(i);
                Selection satSel(body);
                KPopupMenu *satMenu = new KPopupMenu(this);
                insert(satMenu, satSel, baseId * MENUMAXSIZE + (i + 1) * MENUMAXSIZE);
                planetaryMenu->insertItem(body->getName().c_str(), satMenu);
            }
            popup->insertItem(i18n("Satellites"), planetaryMenu);
        }
    }
    else if (showSubObjects && sel.star() != NULL)
    {
        popup->setItemEnabled(baseId + 5, false);
        Simulation *sim = appCore->getSimulation();
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star()->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            popup->insertSeparator();
            SolarSystem* solarSys = iter->second;
            KPopupMenu* planetsMenu = new KPopupMenu(this);
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize() && i < MENUMAXSIZE; i++)
            {
                Body* body = solarSys->getPlanets()->getBody(i);
                Selection satSel(body);
                KPopupMenu *satMenu = new KPopupMenu(this);
                insert(satMenu, satSel, baseId * MENUMAXSIZE + (i + 1) * MENUMAXSIZE);
                planetsMenu->insertItem(body->getName().c_str(), satMenu);
            }
            popup->insertItem(i18n("Planets"), planetsMenu);
        }
    }
    else if (sel.deepsky() != NULL)
    {
        popup->setItemEnabled(baseId + 5, false);
    }
}

