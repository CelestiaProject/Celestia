// qtselectionpopup.cpp
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

#include <cassert>
#include <sstream>
#include <celestia/celestiacore.h>
#include <celengine/axisarrow.h>
#include <celengine/planetgrid.h>
#include "qtselectionpopup.h"
#include "qtappwin.h"

using namespace Eigen;
using namespace std;


static QAction* boldTextItem(const QString& s)
{
    QAction* act = new QAction(s, NULL);
    QFont boldFont = act->font();
    boldFont.setBold(true);
    act->setFont(boldFont);

    return act;
}


static QAction* italicTextItem(const QString& s)
{
    QAction* act = new QAction(s, NULL);
    QFont italicFont = act->font();
    italicFont.setItalic(true);
    act->setFont(italicFont);

    return act;
}


SelectionPopup::SelectionPopup(const Selection& sel,
                               CelestiaCore* _appCore,
                               QWidget* parent) :
    QMenu(parent),
    selection(sel),
    appCore(_appCore),
    centerAction(NULL),
    gotoAction(NULL)
{
    Simulation* sim = appCore->getSimulation();
    Vector3d v = sel.getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());

    if (sel.body() != NULL)
    {
        addAction(boldTextItem(QString::fromUtf8(sel.body()->getName(true).c_str())));

        // Start and end dates
        double startTime = 0.0;
        double endTime = 0.0;
        sel.body()->getLifespan(startTime, endTime);
        
        if (startTime > -1.0e9 || endTime < 1.0e9)
        {
            addSeparator();

            if (startTime > -1.0e9)
            {
                ostringstream startDateStr;
                startDateStr << "Start: " << astro::TDBtoUTC(startTime);
                QAction* startDateAct = new QAction(startDateStr.str().c_str(), this);
                connect(startDateAct, SIGNAL(triggered()),
                        this, SLOT(slotGotoStartDate()));
                addAction(startDateAct);
            }

            if (endTime < 1.0e9)
            {
                ostringstream endDateStr;
                endDateStr << "End: " << astro::TDBtoUTC(endTime);
                QAction* endDateAct = new QAction(endDateStr.str().c_str(), this);
                connect(endDateAct, SIGNAL(triggered()),
                        this, SLOT(slotGotoEndDate()));
                addAction(endDateAct);
            }
        }

    }
    else if (sel.star() != NULL)
    {
        std::string name = ReplaceGreekLetterAbbr(sim->getUniverse()->getStarCatalog()->getStarName(*sel.star(), true));
        addAction(boldTextItem(QString::fromUtf8(name.c_str())));
        
        // Add some text items giving additional information about
        // the star.
        double distance = astro::kilometersToLightYears(v.norm());
        char buff[50];

        setlocale(LC_NUMERIC, "");

        if (abs(distance) >= astro::AUtoLightYears(1000.0f))
            sprintf(buff, "%.3f %s", distance, ("ly"));
        else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
            sprintf(buff, "%.3f %s", astro::lightYearsToAU(distance), ("au"));
        else if (abs(distance) > astro::kilometersToLightYears(1.0f))
            sprintf(buff, "%.3f km", astro::lightYearsToKilometers(distance));
        else
            sprintf(buff, "%.3f m", astro::lightYearsToKilometers(distance) * 1000.0f);

        addAction(italicTextItem(_("Distance: ") + QString::fromUtf8(buff)));

        sprintf(buff, "%.2f (%.2f)",
                sel.star()->getAbsoluteMagnitude(),
                astro::absToAppMag(sel.star()->getAbsoluteMagnitude(),
                                   (float) distance));
        addAction(italicTextItem(_("Abs (app) mag: ") + QString::fromUtf8(buff)));

        sprintf(buff, "%s", sel.star()->getSpectralType());
        addAction(italicTextItem(_("Class: ") + QString::fromUtf8(buff)));

        setlocale(LC_NUMERIC, "C");
    }
    else if (sel.deepsky() != NULL)
    {
        addAction(boldTextItem(QString::fromUtf8(sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky(), true).c_str())));
    }

    addSeparator();

    QAction* selectAction = new QAction(_("&Select"), this);
    connect(selectAction, SIGNAL(triggered()), this, SLOT(slotSelect()));
    addAction(selectAction);

    centerAction = new QAction(_("&Center"), this);
    connect(centerAction, SIGNAL(triggered()), this, SLOT(slotCenterSelection()));
    addAction(centerAction);

    gotoAction = new QAction(_("&Goto"), this);
    connect(gotoAction, SIGNAL(triggered()), this, SLOT(slotGotoSelection()));
    addAction(gotoAction);

    QAction* followAction = new QAction(_("&Follow"), this);
    connect(followAction, SIGNAL(triggered()), this, SLOT(slotFollowSelection()));
    addAction(followAction);

    if (sel.star() == NULL && sel.deepsky() == NULL)
    {
        QAction* syncOrbitAction = new QAction(_("S&ynch Orbit"), this);
        connect(syncOrbitAction, SIGNAL(triggered()), this, SLOT(slotSyncOrbitSelection()));
        addAction(syncOrbitAction);
    }

    QAction* infoAction = new QAction("Info", this);
    connect(infoAction, SIGNAL(triggered()), this, SLOT(slotInfo()));
    addAction(infoAction);

	if (sel.body() != NULL)
	{
		QAction* setVisibilityAction = new QAction("Visible", this);
		setVisibilityAction->setCheckable(true);
		setVisibilityAction->setChecked(sel.body()->isVisible());
		connect(setVisibilityAction, SIGNAL(toggled(bool)), this, SLOT(slotToggleVisibility(bool)));
		addAction(setVisibilityAction);
	}

    // Marker submenu
    QMenu* markMenu = createMarkMenu();
    addMenu(markMenu);

    // Reference vector submenu
    if (selection.body() != NULL)
    {
        QMenu* refVecMenu = createReferenceVectorMenu();
        addMenu(refVecMenu);
    }

    if (selection.body() != NULL)
    {
        // Alternate surface submenu
        QMenu* surfacesMenu = createAlternateSurfacesMenu();
        if (surfacesMenu != NULL)
            addMenu(surfacesMenu);

        // Child object menus
        PlanetarySystem* sys = selection.body()->getSatellites();
        if (sys != NULL)
            addObjectMenus(sys);
    }
    else if (selection.star() != NULL)
    {
        // Child object menus for star
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(selection.star()->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            if (solarSys)
            {
                PlanetarySystem* sys = solarSys->getPlanets();
                if (sys != NULL)
                    addObjectMenus(sys);
            }
        }
    }
}


SelectionPopup::~SelectionPopup()
{
}


QMenu* SelectionPopup::createMarkMenu()
{
    const MarkerRepresentation::Symbol MARKER_SYMBOLS[] = {
        MarkerRepresentation::Diamond,
        MarkerRepresentation::Triangle,
        MarkerRepresentation::Square,
        MarkerRepresentation::FilledSquare,
        MarkerRepresentation::Plus,
        MarkerRepresentation::X,
        MarkerRepresentation::LeftArrow,
        MarkerRepresentation::RightArrow,
        MarkerRepresentation::UpArrow,
        MarkerRepresentation::DownArrow,
        MarkerRepresentation::Circle,
        MarkerRepresentation::Disk,
    };

    const char* MARKER_NAMES[] = {
        gettext_noop("Diamond"),
        gettext_noop("Triangle"),
        gettext_noop("Square"),
        gettext_noop("Filled Square"),
        gettext_noop("Plus"),
        gettext_noop("X"),
        gettext_noop("Left Arrow"),
        gettext_noop("Right Arrow"),
        gettext_noop("Up Arrow"),
        gettext_noop("Down Arrow"),
        gettext_noop("Circle"),
        gettext_noop("Disk"),
    };
        
    QMenu* markMenu = new QMenu(_("&Mark"));
    for (int i = 0; i < (int) (sizeof(MARKER_NAMES) / sizeof(MARKER_NAMES[0])); i++)
    {
        QAction* act = new QAction(_(MARKER_NAMES[i]), markMenu);
        act->setData((int) MARKER_SYMBOLS[i]);
        connect(act, SIGNAL(triggered()), this, SLOT(slotMark()));
        markMenu->addAction(act);
    }

    return markMenu;
}


QMenu* SelectionPopup::createReferenceVectorMenu()
{
    assert(selection.body() != NULL);

    Body* body = selection.body();
    QMenu* refVecMenu = new QMenu(_("&Reference Marks"));

    QAction* bodyAxesAction = new QAction(_("Show &Body Axes"), refVecMenu);
    bodyAxesAction->setCheckable(true);
    bodyAxesAction->setChecked(appCore->referenceMarkEnabled("body axes", selection));
    connect(bodyAxesAction, SIGNAL(triggered()), this, SLOT(slotToggleBodyAxes()));
    refVecMenu->addAction(bodyAxesAction);

    QAction* frameAxesAction = new QAction(_("Show &Frame Axes"), refVecMenu);
    frameAxesAction->setCheckable(true);
    frameAxesAction->setChecked(appCore->referenceMarkEnabled("frame axes", selection));
    connect(frameAxesAction, SIGNAL(triggered()), this, SLOT(slotToggleFrameAxes()));
    refVecMenu->addAction(frameAxesAction);

    QAction* sunDirectionAction = new QAction(_("Show &Sun Direction"), refVecMenu);
    sunDirectionAction->setCheckable(true);
    sunDirectionAction->setChecked(appCore->referenceMarkEnabled("sun direction", selection));
    connect(sunDirectionAction, SIGNAL(triggered()), this, SLOT(slotToggleSunDirection()));
    refVecMenu->addAction(sunDirectionAction);

    QAction* velocityVectorAction = new QAction(_("Show &Velocity Vector"), refVecMenu);
    velocityVectorAction->setCheckable(true);
    velocityVectorAction->setChecked(appCore->referenceMarkEnabled("velocity vector", selection));
    connect(velocityVectorAction, SIGNAL(triggered()), this, SLOT(slotToggleVelocityVector()));
    refVecMenu->addAction(velocityVectorAction);

    QAction* spinVectorAction = new QAction(_("Show S&pin Vector"), refVecMenu);
    spinVectorAction->setCheckable(true);
    spinVectorAction->setChecked(appCore->referenceMarkEnabled("spin vector", selection));
    connect(spinVectorAction, SIGNAL(triggered()), this, SLOT(slotToggleSpinVector()));
    refVecMenu->addAction(spinVectorAction);

    Selection center = body->getOrbitFrame(appCore->getSimulation()->getTime())->getCenter();
    if (center.body() != NULL)
    {
        // Only show the frame center menu item if the selection orbits another
        // a non-stellar object. If it orbits a star, this is generally identical
        // to the sun direction entry.
        QAction* frameCenterAction = new QAction(QString(_("Show &Direction to %1")).arg(QString::fromUtf8(center.body()->getName(true).c_str())), refVecMenu);
        frameCenterAction->setCheckable(true);
        frameCenterAction->setChecked(appCore->referenceMarkEnabled("frame center direction", selection));
        connect(frameCenterAction, SIGNAL(triggered()), this, SLOT(slotToggleFrameCenterDirection()));
        refVecMenu->addAction(frameCenterAction);
    }

    QAction* gridAction = new QAction(_("Show Planetographic &Grid"), refVecMenu);
    gridAction->setCheckable(true);
    gridAction->setChecked(appCore->referenceMarkEnabled("planetographic grid", selection));
    connect(gridAction, SIGNAL(triggered()), this, SLOT(slotTogglePlanetographicGrid()));
    refVecMenu->addAction(gridAction);

    QAction* terminatorAction = new QAction(_("Show &Terminator"), refVecMenu);
    terminatorAction->setCheckable(true);
    terminatorAction->setChecked(appCore->referenceMarkEnabled("terminator", selection));
    connect(terminatorAction, SIGNAL(triggered()), this, SLOT(slotToggleTerminator()));
    refVecMenu->addAction(terminatorAction);

    return refVecMenu;
}


QMenu* SelectionPopup::createAlternateSurfacesMenu()
{
    QMenu* surfacesMenu = NULL;
    std::vector<std::string>* altSurfaces = selection.body()->getAlternateSurfaceNames();
    if (altSurfaces != NULL)
    {
        if (!altSurfaces->empty())
        {
            surfacesMenu = new QMenu(_("&Alternate Surfaces"), this);
            QAction* normalAct = new QAction(_("Normal"), surfacesMenu);
            normalAct->setData(QString(""));
            surfacesMenu->addAction(normalAct);
            connect(normalAct, SIGNAL(triggered()), this, SLOT(slotSelectAlternateSurface()));

            for (std::vector<std::string>::const_iterator i = altSurfaces->begin();
                 i < altSurfaces->end(); i++)
            {
                QString surfaceName((*i).c_str());
                QAction* act = new QAction(surfaceName, surfacesMenu);
                act->setData(surfaceName);
                surfacesMenu->addAction(act);
                connect(act, SIGNAL(triggered()), this, SLOT(slotSelectAlternateSurface()));
            }
            
            addMenu(surfacesMenu);
        }
        
        delete altSurfaces;
    }

    return surfacesMenu;
}


QMenu* SelectionPopup::createObjectMenu(PlanetarySystem* sys,
                                        unsigned int classification)
{
    QMenu* menu = NULL;

    if (sys != NULL)
    {
        int nObjects = sys->getSystemSize();
        for (int i = 0; i < nObjects; i++)
        {
            Body* body = sys->getBody(i);
            if ((body->getClassification() & classification) != 0)
            {
                if (menu == NULL)
                {
                    QString title;
                    switch (classification)
                    {
                    case Body::Planet:
                        title = _("Planets");
                        break;
                    case Body::Moon:
                        title = _("Moons");
                        break;
                    case Body::Asteroid:
                        title = _("Asteroids");
                        break;
                    case Body::Comet:
                        title = _("Comets");
                        break;
                    case Body::Spacecraft:
                        title = _("Spacecraft");
                        break;
                    default:
                        title = _("Other objects");
                        break;
                    }
                    menu = new QMenu(title, this);
                }
                
                QAction* act = new QAction(QString::fromUtf8(body->getName(true).c_str()), menu);
                act->setData(i);
                connect(act, SIGNAL(triggered()), this, SLOT(slotSelectChildObject()));
                menu->addAction(act);
            }
        }
    }

    return menu;
}


void SelectionPopup::addObjectMenus(PlanetarySystem* sys)
{
    QMenu* planetsMenu = createObjectMenu(sys, Body::Planet);
    if (planetsMenu != NULL)
        addMenu(planetsMenu);

    QMenu* moonsMenu = createObjectMenu(sys, Body::Moon);
    if (moonsMenu != NULL)
        addMenu(moonsMenu);

    QMenu* asteroidsMenu = createObjectMenu(sys, Body::Asteroid);
    if (asteroidsMenu != NULL)
        addMenu(asteroidsMenu);

    QMenu* cometsMenu = createObjectMenu(sys, Body::Comet);
    if (cometsMenu != NULL)
        addMenu(cometsMenu);

    QMenu* spacecraftMenu = createObjectMenu(sys, Body::Spacecraft);
    if (spacecraftMenu != NULL)
        addMenu(spacecraftMenu);
}


void SelectionPopup::popupAtGoto(const QPoint& pt)
{
    exec(pt, gotoAction);
}


void SelectionPopup::popupAtCenter(const QPoint& pt)
{
    exec(pt, centerAction);
}


/******** Slots *********/

void SelectionPopup::slotSelect()
{
    appCore->getSimulation()->setSelection(selection);
}


void SelectionPopup::slotCenterSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("c");
}


void SelectionPopup::slotGotoSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("g");
}


void SelectionPopup::slotFollowSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("f");
}


void SelectionPopup::slotSyncOrbitSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("y");
}


void SelectionPopup::slotSelectAlternateSurface()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        const char* surfName = action->data().toString().toUtf8().constData();
        Simulation* sim = appCore->getSimulation();
        sim->getActiveObserver()->setDisplayedSurface(surfName);
    }
}


void SelectionPopup::slotSelectChildObject()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        bool convertOK = false;
        int childIndex = action->data().toInt(&convertOK);
        if (convertOK)
        {
            Simulation* sim = appCore->getSimulation();
            PlanetarySystem* sys = NULL;
            if (selection.body() != NULL)
            {
                sys = selection.body()->getSatellites();
            }
            else if (selection.star())
            {
                SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
                SolarSystemCatalog::iterator iter = solarSystemCatalog->find(selection.star()->getCatalogNumber());
                if (iter != solarSystemCatalog->end())
                {
                    SolarSystem* solarSys = iter->second;
                    if (solarSys)
                        sys = solarSys->getPlanets();
                }
            }   

            if (sys != NULL)
            {
                if (childIndex >= 0 && childIndex < sys->getSystemSize())
                    sim->setSelection(Selection(sys->getBody(childIndex)));
            }
        }
    }
}


void SelectionPopup::slotMark()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        bool convertOK = false;
        int symbol = action->data().toInt(&convertOK);
        if (convertOK)
        {
            Simulation* sim = appCore->getSimulation();
            sim->getUniverse()->markObject(selection,
                                           MarkerRepresentation(MarkerRepresentation::Symbol(symbol), 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)),
                                           1);

            // Automatically enable markers
            appCore->getRenderer()->setRenderFlags(appCore->getRenderer()->getRenderFlags() | Renderer::ShowMarkers);
        }
    }
}


void SelectionPopup::slotToggleBodyAxes()
{
    appCore->toggleReferenceMark("body axes", selection);
}


void SelectionPopup::slotToggleFrameAxes()
{
    appCore->toggleReferenceMark("frame axes", selection);
}


void SelectionPopup::slotToggleSunDirection()
{
    appCore->toggleReferenceMark("sun direction", selection);
}

void SelectionPopup::slotToggleVelocityVector()
{
    appCore->toggleReferenceMark("velocity vector", selection);
}

void SelectionPopup::slotToggleSpinVector()
{
    appCore->toggleReferenceMark("spin vector", selection);
}


void SelectionPopup::slotToggleFrameCenterDirection()
{
    appCore->toggleReferenceMark("frame center direction", selection);
}


void SelectionPopup::slotTogglePlanetographicGrid()
{
    appCore->toggleReferenceMark("planetographic grid", selection);
}


void SelectionPopup::slotToggleTerminator()
{
    appCore->toggleReferenceMark("terminator", selection);
}


void SelectionPopup::slotGotoStartDate()
{
    assert(selection.body() != NULL);
    double startDate = 0.0;
    double endDate = 0.0;
    selection.body()->getLifespan(startDate, endDate);
    appCore->getSimulation()->setTime(startDate);
}


void SelectionPopup::slotGotoEndDate()
{
    assert(selection.body() != NULL);
    double startDate = 0.0;
    double endDate = 0.0;
    selection.body()->getLifespan(startDate, endDate);
    appCore->getSimulation()->setTime(endDate);
}


void SelectionPopup::slotInfo()
{
    emit selectionInfoRequested(selection);
}


void SelectionPopup::slotToggleVisibility(bool visible)
{
	assert(selection.body() != NULL);
	selection.body()->setVisible(visible);	
}
