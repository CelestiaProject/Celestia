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
#include "qtselectionpopup.h"
#include "qtappwin.h"

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
    Vec3d v = sel.getPosition(sim->getTime()) - sim->getObserver().getPosition();

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
        std::string name = sim->getUniverse()->getStarCatalog()->getStarName(*sel.star());
        addAction(boldTextItem(QString::fromUtf8(ReplaceGreekLetterAbbr(name).c_str())));
        
        // Add some text items giving additional information about
        // the star.
        double distance = v.length() * 1e-6;
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

        addAction(italicTextItem(tr("Distance: ") + QString::fromUtf8(buff)));

        sprintf(buff, "%.2f (%.2f)",
                sel.star()->getAbsoluteMagnitude(),
                astro::absToAppMag(sel.star()->getAbsoluteMagnitude(),
                                   (float) distance));
        addAction(italicTextItem(tr("Abs (app) mag: ") + QString::fromUtf8(buff)));

        sprintf(buff, "%s", sel.star()->getSpectralType());
        addAction(italicTextItem(tr("Class: ") + QString::fromUtf8(buff)));

        setlocale(LC_NUMERIC, "C");
    }
    else if (sel.deepsky() != NULL)
    {
        addAction(boldTextItem(QString::fromUtf8(sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky()).c_str())));
    }

    addSeparator();

    QAction* selectAction = new QAction(tr("&Select"), this);
    addAction(selectAction);

    centerAction = new QAction(tr("&Center"), this);
    connect(centerAction, SIGNAL(triggered()), this, SLOT(slotCenterSelection()));
    addAction(centerAction);

    gotoAction = new QAction(tr("&Goto"), this);
    connect(gotoAction, SIGNAL(triggered()), this, SLOT(slotGotoSelection()));
    addAction(gotoAction);

    QAction* followAction = new QAction(tr("&Follow"), this);
    connect(followAction, SIGNAL(triggered()), this, SLOT(slotFollowSelection()));
    addAction(followAction);

    if (sel.star() == NULL && sel.deepsky() == NULL)
    {
        QAction* syncOrbitAction = new QAction(tr("S&ynch Orbit"), this);
        connect(syncOrbitAction, SIGNAL(triggered()), this, SLOT(slotSyncOrbitSelection()));
        addAction(syncOrbitAction);
    }

    QAction* infoAction = new QAction("Info", this);
    addAction(infoAction);

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
    const Marker::Symbol MARKER_SYMBOLS[] = {
        Marker::Diamond,
        Marker::Triangle,
        Marker::Square,
        Marker::FilledSquare,
        Marker::Plus,
        Marker::X,
        Marker::LeftArrow,
        Marker::RightArrow,
        Marker::UpArrow,
        Marker::DownArrow,
        Marker::Circle,
        Marker::Disk,
    };

    const char* MARKER_NAMES[] = {
        "Diamond",
        "Triangle",
        "Square",
        "Filled Square",
        "Plus",
        "X",
        "Left Arrow",
        "Right Arrow",
        "Up Arrow",
        "Down Arrow",
        "Circle",
        "Disk",
    };
        
    QMenu* markMenu = new QMenu("&Mark");
    for (int i = 0; i < (int) (sizeof(MARKER_NAMES) / sizeof(MARKER_NAMES[0])); i++)
    {
        QAction* act = new QAction(tr(MARKER_NAMES[i]), markMenu);
        act->setData((int) MARKER_SYMBOLS[i]);
        connect(act, SIGNAL(triggered()), this, SLOT(slotMark()));
        markMenu->addAction(act);
    }

    return markMenu;
}


QMenu* SelectionPopup::createReferenceVectorMenu()
{
    assert(selection.body() != NULL);

    const int REFVECS[] = {
        Body::BodyAxes,
        Body::FrameAxes,
        Body::SunDirection,
        Body::VelocityVector,
    };

    const char* REFVEC_NAMES[] = {
        "Show Body Axes",
        "Show Frame Axes",
        "Show Sun Direction",
        "Show Velocity Vector",
    };
        
    QMenu* refVecMenu = new QMenu("&Reference Vectors");
    for (int i = 0; i < (int) (sizeof(REFVECS) / sizeof(REFVECS[0])); i++)
    {
        QAction* act = new QAction(tr(REFVEC_NAMES[i]), refVecMenu);
        act->setData((int) REFVECS[i]);
        act->setCheckable(true);
        act->setChecked(selection.body()->referenceMarkVisible(REFVECS[i]));
        connect(act, SIGNAL(triggered()), this, SLOT(slotToggleReferenceVector()));
        //connect(act, SIGNAL(changed()), this, SLOT(toggleReferenceVector()));
        refVecMenu->addAction(act);
    }

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
            surfacesMenu = new QMenu(tr("&Alternate Surfaces"), this);
            QAction* normalAct = new QAction(tr("Normal"), surfacesMenu);
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
                        title = tr("Planets");
                        break;
                    case Body::Moon:
                        title = tr("Moons");
                        break;
                    case Body::Asteroid:
                        title = tr("Asteroids");
                        break;
                    case Body::Comet:
                        title = tr("Comets");
                        break;
                    case Body::Spacecraft:
                        title = tr("Spacecracft");
                        break;
                    default:
                        title = tr("Other objects");
                        break;
                    }
                    menu = new QMenu(title, this);
                }
                
                QAction* act = new QAction(QString(body->getName().c_str()), menu);
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
                                           10.0f,
                                           Color(0.0f, 1.0f, 0.0f, 0.9f),
                                           (Marker::Symbol) symbol,
                                           1,
                                           "");
        }
    }
}


void SelectionPopup::slotToggleReferenceVector()
{
    assert(selection.body() != NULL);

    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        bool convertOK = false;
        int refVec = action->data().toInt(&convertOK);
        if (convertOK)
        {
            Body* body = selection.body();
            body->setVisibleReferenceMarks(body->getVisibleReferenceMarks() ^ refVec);
        }
    }
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
