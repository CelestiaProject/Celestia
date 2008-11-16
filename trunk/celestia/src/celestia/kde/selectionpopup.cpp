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


#include <libintl.h>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "selectionpopup.h"

#include <qlabel.h>

#include <klocale.h>
#include <kurl.h>
#include <krun.h>

#include "celutil/utf8.h"
#include "celestia/celestiacore.h"
#include "celengine/axisarrow.h"
#include "celengine/planetgrid.h"


SelectionPopup::SelectionPopup(QWidget* parent, CelestiaCore* _appCore, Selection _sel):
KPopupMenu(parent),
 appCore(_appCore),
 sel(_sel),
 baseId(0)
{
}

SelectionPopup::~SelectionPopup() {

}

void SelectionPopup::init()
{
    QPalette pal( palette() );
    pal.setColor( QPalette::Normal, QColorGroup::Background, QColor( "White" ) );
    pal.setColor( QPalette::Normal, QColorGroup::Foreground, QColor( "Black" ) );
    pal.setColor( QPalette::Inactive, QColorGroup::Foreground, QColor( "Black" ) );

    QFont rsFont = font();
    rsFont.setPointSize( rsFont.pointSize() - 2 );

    Simulation *sim = appCore->getSimulation();
    Vec3d v = sel.getPosition(sim->getTime()) - sim->getObserver().getPosition();

    if (sel.body() != NULL)
    {
        insertTitle(QString::fromUtf8(sel.body()->getName(true).c_str()), 0, 0);
        insert(this, sel);
    }
    else if (sel.star() != NULL)
    {
        std::string name = sim->getUniverse()->getStarCatalog()->getStarName(*sel.star());

        double distance = v.length() * 1e-6;
        char buff[50];

        ostringstream o;
        setlocale(LC_NUMERIC, "");

        if (abs(distance) >= astro::AUtoLightYears(1000.0f))
            sprintf(buff, "%'.3f %s", distance, _("ly"));
        else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
            sprintf(buff, "%'.3f %s", astro::lightYearsToAU(distance), _("au"));
        else if (abs(distance) > astro::kilometersToLightYears(1.0f))
            sprintf(buff, "%'.3f km", astro::lightYearsToKilometers(distance));
        else
            sprintf(buff, "%'.3f m", astro::lightYearsToKilometers(distance) * 1000.0f);

        o << i18n("Distance: ") << QString::fromUtf8(buff) << "\n";

        o << i18n("Abs (app) mag: ");

        sprintf(buff, "%'.2f (%.2f)",
                   sel.star()->getAbsoluteMagnitude(),
                   astro::absToAppMag(sel.star()->getAbsoluteMagnitude(),
                                      (float) distance));
        o << QString::fromUtf8(buff) << "\n";

        o << i18n("Class: ");
        sprintf(buff, "%s", sel.star()->getSpectralType());
        o << QString::fromUtf8(buff) << "\n";

        o << i18n("Surface Temp: ");
        sprintf(buff, "%'.0f K", sel.star()->getTemperature());
        o << QString::fromUtf8(buff) << "\n";

        o << i18n("Radius: ");
        sprintf(buff, "%'.2f %s", sel.star()->getRadius() / 696000.0f, _("Rsun"));
        o << QString::fromUtf8(buff);

        setlocale(LC_NUMERIC, "C");

        QLabel *starDetails = new QLabel(QString(o.str().c_str()), this);
        starDetails->setPalette(pal);
        starDetails->setFont(rsFont);

        insertTitle(QString::fromUtf8(ReplaceGreekLetterAbbr(name).c_str()), 0, 0);
        insertItem(starDetails);
        insertSeparator();
        insert(this, sel);
    }
    else if (sel.deepsky() != NULL)
    {
        insertTitle(QString::fromUtf8(sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky()).c_str()), 0);
        insert(this, sel);
    }

    if (sim->getUniverse() != NULL)
    {
        MarkerList* markers = sim->getUniverse()->getMarkers();
        if (markers->size() > 0)
        {
            KPopupMenu *markMenu = new KPopupMenu(this);
            int j=1;
            for (std::vector<Marker>::iterator i = markers->begin(); i < markers->end(); i++)
            {
                KPopupMenu *objMenu = new KPopupMenu(this);
                insert(objMenu, (*i).object(), false);
                markMenu->insertItem(QString::fromUtf8(getSelectionName((*i).object())), objMenu);
                j++;
            }
            insertItem(i18n("Marked objects"), markMenu);
        }
    }
}

void SelectionPopup::process(int id)
{
    if (id < 0) return;

    Simulation *sim = appCore->getSimulation();

    sel = getSelectionFromId(id);
    int actionId = id - baseId;

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
        if (sel.body() != NULL)
        {
            url = QString(sel.body()->getInfoURL().c_str());
            if (url == "")
            {
                QString name = QString(sel.body()->getName().c_str()).lower();
                url = QString("http://www.nineplanets.org/") + name + ".html";
            }
        }
        else if (sel.star() != NULL)
        {
            if (sel.star()->getCatalogNumber() != 0)
                url = QString("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=HIP %1")
                      .arg(sel.star()->getCatalogNumber() & ~0xf0000000);
            else
                url = QString("http://www.nineplanets.org/sun.html");
        }
        else if (sel.deepsky() != NULL)
        {
            url = QString(sel.deepsky()->getInfoURL().c_str());
            if (url == "")
                url = QString("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=%1")
                      .arg(sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky()).c_str());
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
    if (actionId >= 10 && actionId < 25)
    {
        if (sim->getUniverse() != NULL)
        {
            MarkerRepresentation markerRep((MarkerRepresentation::Symbol) (actionId - 1), 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f));

            sim->getUniverse()->markObject(sel, markerRep, 1);
        }
        return;
    }
    if (actionId >=25 && actionId < 31 && sel.body() != NULL) 
    {

        switch(actionId)
        {
        case 25:
		appCore->toggleReferenceMark("body axes");
            break;
        case 26:
		appCore->toggleReferenceMark("frame axes");
            break;
        case 27:
		appCore->toggleReferenceMark("sun direction");
            break;
        case 28:
		appCore->toggleReferenceMark("velocity vector");
            break;
	case 29:
		appCore->toggleReferenceMark("planetographic grid");
	    break;
	case 30:
		appCore->toggleReferenceMark("terminator");
	    break;
        }

    }
    if (actionId == 31) {
        sim->getActiveObserver()->setDisplayedSurface("");
        return;
    }
    if (actionId > 31) {
        std::vector<std::string>* altSurfaces = sel.body()->getAlternateSurfaceNames();
        if (altSurfaces != NULL && (int) altSurfaces->size() > actionId - 32)
        {
            sim->getActiveObserver()->setDisplayedSurface((*altSurfaces)[actionId - 32]);
        }
    }
}

const char* SelectionPopup::getSelectionName(const Selection& sel) const
{
    if (sel.body() != NULL)
        return sel.body()->getName(true).c_str();
    else if (sel.star() != NULL)
        return ReplaceGreekLetterAbbr(appCore->getSimulation()->getUniverse()->getStarCatalog()->getStarName(*(sel.star()))).c_str();
    else if (sel.deepsky() != NULL)
        return appCore->getSimulation()->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky()).c_str();

    return "";
}

Selection SelectionPopup::getSelectionFromId(int id)
{
    for(std::vector< std::pair<int, Selection> >::const_iterator i = baseIds.begin() + 1;
        i != baseIds.end();
        ++i) {
        if ((*i).first > id) {
            baseId = (*(i-1)).first;
            return (*(i-1)).second;
        }
    }
    baseId = baseIds.back().first;
    return baseIds.back().second;
}

void SelectionPopup::insert(KPopupMenu *popup, Selection sel, bool showSubObjects)
{
    baseIds.push_back(make_pair(baseId, sel));
    int locBaseId = baseId;

    popup->insertItem(i18n("&Select"), baseId + 1);
    popup->insertItem(i18n("&Center"), baseId + 2);
    popup->insertItem(i18n("&Goto"), baseId + 3);
    popup->insertItem(i18n("&Follow"), baseId + 4);
    if (sel.star() == NULL && sel.deepsky() == NULL)
        popup->insertItem(i18n("S&ynch Orbit"), baseId + 5);
    popup->insertItem(i18n("&Info"), baseId + 6);
    if (baseId == 0)
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
        markMenu->insertItem(i18n("Filled Square"), baseId + 13);
        markMenu->insertItem(i18n("Plus"), baseId + 14);
        markMenu->insertItem(i18n("X"), baseId + 15);
        markMenu->insertItem(i18n("Left Arrow"), baseId + 16);
        markMenu->insertItem(i18n("Right Arrow"), baseId + 17);
        markMenu->insertItem(i18n("Up Arrow"), baseId + 18);
        markMenu->insertItem(i18n("Down Arrow"), baseId + 19);
        markMenu->insertItem(i18n("Circle"), baseId + 20);
        markMenu->insertItem(i18n("Disk"), baseId + 21);
        popup->insertItem(i18n("&Mark"), markMenu);
    }

    if (sel.body() != NULL) {
        KPopupMenu *refVectorMenu = new KPopupMenu(this);
        refVectorMenu->setCheckable(true);
        popup->insertItem(i18n("&Reference Vectors"), refVectorMenu);
        refVectorMenu->insertItem(i18n("Show Body Axes"), baseId + 25);
        refVectorMenu->setItemChecked(baseId + 25, sel.body()->findReferenceMark("body axes"));
        refVectorMenu->insertItem(i18n("Show Frame Axes"), baseId + 26);
        refVectorMenu->setItemChecked(baseId + 26, sel.body()->findReferenceMark("frame axes"));
        refVectorMenu->insertItem(i18n("Show Sun Direction"), baseId + 27);
        refVectorMenu->setItemChecked(baseId + 27, sel.body()->findReferenceMark("sun direction"));
        refVectorMenu->insertItem(i18n("Show Velocity Vector"), baseId + 28);
        refVectorMenu->setItemChecked(baseId + 28, sel.body()->findReferenceMark("velocity vector"));
        refVectorMenu->insertItem(i18n("Show Planetographic Grid"), baseId + 29);
        refVectorMenu->setItemChecked(baseId + 29, sel.body()->findReferenceMark("planetographic grid"));
        refVectorMenu->insertItem(i18n("Show Terminator"), baseId + 30);
        refVectorMenu->setItemChecked(baseId + 30, sel.body()->findReferenceMark("terminator"));
    }

    baseId += 31;

    if (showSubObjects && sel.body() != NULL)
    {
        std::vector<std::string>* altSurfaces = sel.body()->getAlternateSurfaceNames();
        if (altSurfaces != NULL)
        {
            if (!altSurfaces->empty())
            {
                KPopupMenu *surfaces = new KPopupMenu(this);
                surfaces->insertItem(i18n("Normal"), locBaseId + 31);
                int j=0;
                for (std::vector<std::string>::const_iterator i = altSurfaces->begin();
                     i < altSurfaces->end(); i++, j++)
                {
                    surfaces->insertItem(QString((*i).c_str()), locBaseId + 32 + j);
                }
                baseId += 7 + j;
                popup->insertItem(i18n("&Alternate Surfaces"), surfaces);
            }
            delete altSurfaces;
        }

        const PlanetarySystem* satellites = sel.body()->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            insertPlanetaryMenu(popup, getSelectionName(sel), satellites);
        }
    }
    else if (showSubObjects && sel.star() != NULL)
    {
        Simulation *sim = appCore->getSimulation();
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star()->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            insertPlanetaryMenu(popup, getSelectionName(sel), solarSys->getPlanets());
        }
    }
}

struct BodyComparePredicate
{
    bool operator()(Body* a, Body* b)
    {
        return (a->getName(true).compare(b->getName(true)) < 0);
    }
};


void SelectionPopup::insertPlanetaryMenu(KPopupMenu* popup, const string& parentName, const PlanetarySystem* psys) {
    std::vector<Body*> asteroids;
    std::vector<Body*> comets;
    std::vector<Body*> invisibles;
    std::vector<Body*> moons;
    std::vector<Body*> planets;
    std::vector<Body*> spacecraft;
    std::vector<Body*> smallBodies;
    std::vector<Body*> dwarfPlanets;

    std::vector<std::vector<Body*>* > objects;
    std::vector<std::string> menuNames;

    // Add each vector of PlanetarySystem bodies to a vector to iterate over
    objects.push_back(&asteroids);
    menuNames.push_back(QString::fromUtf8(_("Asteroids")));
    objects.push_back(&comets);
    menuNames.push_back(QString::fromUtf8(_("Comets")));
    objects.push_back(&moons);
    menuNames.push_back(QString::fromUtf8(_("Moons")));
    objects.push_back(&planets);
    menuNames.push_back(QString::fromUtf8(_("Planets")));
    objects.push_back(&spacecraft);
    menuNames.push_back(QString::fromUtf8(_("Spacecraft")));
    objects.push_back(&smallBodies);
    menuNames.push_back(QString::fromUtf8(_("Small Bodies")));
    objects.push_back(&dwarfPlanets);
    menuNames.push_back(QString::fromUtf8(_("Dwarf Planets")));

    for (int i = 0; i < psys->getSystemSize(); i++) {
        Body* body = psys->getBody(i);
        switch(body->getClassification()) {
        case Body::Asteroid:
            asteroids.push_back(body);
            break;
        case Body::Comet:
            comets.push_back(body);
            break;
        case Body::Moon:
            moons.push_back(body);
            break;
        case Body::Planet:
            planets.push_back(body);
            break;
        case Body::Spacecraft:
            spacecraft.push_back(body);
            break;
        case Body::SmallBody:
            smallBodies.push_back(body);
            break;
        case Body::DwarfPlanet:
            dwarfPlanets.push_back(body);
            break;
        case Body::Invisible:
        case Body::Barycenter:
            break;
        }
    }

    // Count how many submenus we need to create

    int numSubMenus = 0;
    int nonEmpty = 0;
    for (std::vector<std::vector<Body*>* >::const_iterator obj = objects.begin();
         obj != objects.end();
         ++obj) {
        if ((*obj)->size() > 0) {
            numSubMenus++;
            nonEmpty = obj - objects.begin();
        }
    }

    if (numSubMenus == 0) return;

    KPopupMenu *submenu = new KPopupMenu(this);
    popup->insertSeparator();

    if (numSubMenus > 1)
        popup->insertItem(i18n("Orbiting Bodies"), submenu);
    else
        popup->insertItem(menuNames[nonEmpty], submenu);

    std::vector<std::string>::const_iterator menuName = menuNames.begin();
    BodyComparePredicate pred;

    for (std::vector<std::vector<Body*>* >::const_iterator obj = objects.begin();
         obj != objects.end();
         ++obj) {
        // Only generate a submenu if the vector is not empty
        if ((*obj)->size() > 0) {
            // Don't create a submenu for a single item
            if ((*obj)->size() == 1) {
                std::vector<Body*>::const_iterator it = (*obj)->begin();
                Selection s(*it);
                KPopupMenu *objMenu = new KPopupMenu(this);
                insert(objMenu, s);
                submenu->insertItem(QString::fromUtf8((*it)->getName(true).c_str()), objMenu);
            } else {
                // Skip sorting if we are dealing with the planets in our own Solar System.
                 if (parentName != "Sol" || *menuName != QString::fromUtf8(_("Planets")))
                     std::sort((*obj)->begin(), (*obj)->end(), pred);

                if (numSubMenus > 1) {
                    // Add items to submenu
                    KPopupMenu* objMenu = new KPopupMenu(this);
                    for(std::vector<Body*>::const_iterator it = (*obj)->begin(); it != (*obj)->end(); ++it) {
                        Selection s(*it);
                        KPopupMenu *menu = new KPopupMenu(this);
                        insert(menu, s);
                        objMenu->insertItem(QString::fromUtf8((*it)->getName(true).c_str()), menu);
                    }
                    submenu->insertItem(*menuName, objMenu);
                } else {
                    // Just add items to the popup
                    for(std::vector<Body*>::const_iterator it = (*obj)->begin(); it != (*obj)->end(); ++it) {
                        Selection s(*it);
                        KPopupMenu *menu = new KPopupMenu(this);
                        insert(menu, s);
                        submenu->insertItem(QString::fromUtf8((*it)->getName(true).c_str()), menu);
                    }
                }
            }
        }
        menuName++;
    }
}

