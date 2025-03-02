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


#include "qtselectionpopup.h"
#include "qtdateutil.h"

#include <array>
#include <cassert>
#include <cmath>
#include <string>

#include <Eigen/Core>

#include <fmt/format.h>

#include <QAction>
#include <QFont>
#include <QString>

#include <celengine/body.h>
#include <celengine/marker.h>
#include <celengine/render.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/solarsys.h>
#include <celestia/celestiacore.h>
#include <celestia/helper.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>

namespace celestia::qt
{

namespace
{

QAction*
boldTextItem(const QString& s)
{
    QAction* act = new QAction(s, nullptr);
    QFont boldFont = act->font();
    boldFont.setBold(true);
    act->setFont(boldFont);

    return act;
}

QAction*
italicTextItem(const QString& s)
{
    QAction* act = new QAction(s, nullptr);
    QFont italicFont = act->font();
    italicFont.setItalic(true);
    act->setFont(italicFont);

    return act;
}

} // end unnamed namespace

SelectionPopup::SelectionPopup(const Selection& sel,
                               CelestiaCore* _appCore,
                               QWidget* parent) :
    QMenu(parent),
    selection(sel),
    appCore(_appCore),
    centerAction(nullptr),
    gotoAction(nullptr)
{
    Simulation* sim = appCore->getSimulation();
    Eigen::Vector3d v = sel.getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());

    if (sel.body() != nullptr)
    {
        addAction(boldTextItem(QString::fromStdString(sel.body()->getName(true))));

        // Start and end dates
        double startTime = 0.0;
        double endTime = 0.0;
        sel.body()->getLifespan(startTime, endTime);

        if (startTime > -1.0e9 || endTime < 1.0e9)
        {
            addSeparator();

            if (startTime > -1.0e9)
            {
                QString startDateStr = QString(_("Start: %1")).arg(TDBToQString(startTime));
                QAction* startDateAct = new QAction(startDateStr, this);
                connect(startDateAct, SIGNAL(triggered()),
                        this, SLOT(slotGotoStartDate()));
                addAction(startDateAct);
            }

            if (endTime < 1.0e9)
            {
                QString endDateStr = QString(_("End: %1")).arg(TDBToQString(endTime));
                QAction* endDateAct = new QAction(endDateStr, this);
                connect(endDateAct, SIGNAL(triggered()),
                        this, SLOT(slotGotoEndDate()));
                addAction(endDateAct);
            }
        }

    }
    else if (sel.star() != nullptr)
    {
        std::string name = ReplaceGreekLetterAbbr(sim->getUniverse()->getStarCatalog()->getStarName(*sel.star(), true));
        addAction(boldTextItem(QString::fromStdString(name)));

        // Add some text items giving additional information about
        // the star.
        double distance = astro::kilometersToLightYears(v.norm());
        std::string buff;

        setlocale(LC_NUMERIC, "");

        if (std::abs(distance) >= astro::AUtoLightYears(1000.0f))
            buff = fmt::format(_("{:.3f} ly"), distance);
        else if (std::abs(distance) >= astro::kilometersToLightYears(10000000.0))
            buff = fmt::format(_("{:.3f} au"), astro::lightYearsToAU(distance));
        else if (std::abs(distance) > astro::kilometersToLightYears(1.0f))
            buff = fmt::format(_("{:.3f} km"), astro::lightYearsToKilometers(distance));
        else
            buff = fmt::format(_("{:.3f} m"), astro::lightYearsToKilometers(distance) * 1000.0f);

        addAction(italicTextItem(_("Distance: ") + QString::fromStdString(buff)));

        buff = fmt::format("{:.2f} ({:.2f})",
                           sel.star()->getAbsoluteMagnitude(),
                           sel.star()->getApparentMagnitude((float) distance));
        addAction(italicTextItem(_("Abs (app) mag: ") + QString::fromStdString(buff)));

        addAction(italicTextItem(_("Class: ") + QString::fromStdString(sel.star()->getSpectralType())));

        setlocale(LC_NUMERIC, "C");
    }
    else if (sel.deepsky() != nullptr)
    {
        addAction(boldTextItem(QString::fromStdString(sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky(), true))));
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

    if (sel.star() == nullptr && sel.deepsky() == nullptr)
    {
        QAction* syncOrbitAction = new QAction(_("S&ync Orbit"), this);
        connect(syncOrbitAction, SIGNAL(triggered()), this, SLOT(slotSyncOrbitSelection()));
        addAction(syncOrbitAction);
    }

    QAction* infoAction = new QAction(_("Info"), this);
    connect(infoAction, SIGNAL(triggered()), this, SLOT(slotInfo()));
    addAction(infoAction);

    if (sel.body() != nullptr)
    {
        QAction* setVisibilityAction = new QAction(_("Visible"), this);
        setVisibilityAction->setCheckable(true);
        setVisibilityAction->setChecked(sel.body()->isVisible());
        connect(setVisibilityAction, SIGNAL(toggled(bool)), this, SLOT(slotToggleVisibility(bool)));
        addAction(setVisibilityAction);
    }

    // Marker submenu
    QMenu* markMenu = createMarkMenu();
    addMenu(markMenu);

    if (appCore->getSimulation()->getUniverse()->isMarked(selection, 1))
    {
        QAction *action = new QAction(_("&Unmark"), this);
        connect(action, &QAction::triggered, this, &SelectionPopup::slotUnmark);
        addAction(action);
    }

    if (selection.body() != nullptr)
    {
        // Reference vector submenu
        QMenu* refVecMenu = createReferenceVectorMenu();
        addMenu(refVecMenu);

        // Alternate surface submenu
        QMenu* surfacesMenu = createAlternateSurfacesMenu();
        if (surfacesMenu != nullptr)
            addMenu(surfacesMenu);

        if (Helper::hasPrimary(selection.body()))
        {
            QAction *action = new QAction(_("Select &Primary Body"), this);
            connect(action, SIGNAL(triggered()), this, SLOT(slotSelectPrimary()));
            addAction(action);
        }

        // Child object menus
        const PlanetarySystem* sys = selection.body()->getSatellites();
        if (sys != nullptr)
            addObjectMenus(sys);
    }
    else if (selection.star() != nullptr)
    {
        // Child object menus for star
        const SolarSystem* solarSys = sim->getUniverse()->getSolarSystem(selection.star());
        if (solarSys != nullptr)
        {
            const PlanetarySystem* sys = solarSys->getPlanets();
            if (sys != nullptr)
                addObjectMenus(sys);
        }
    }
}

QMenu*
SelectionPopup::createMarkMenu()
{
    using namespace celestia;

    constexpr std::array<MarkerRepresentation::Symbol, 12> MARKER_SYMBOLS{
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

    const std::array<const char*, 12> MARKER_NAMES{
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

QMenu*
SelectionPopup::createReferenceVectorMenu()
{
    assert(selection.body() != nullptr);

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
    if (center.body() != nullptr)
    {
        // Only show the frame center menu item if the selection orbits another
        // a non-stellar object. If it orbits a star, this is generally identical
        // to the sun direction entry.
        QAction* frameCenterAction = new QAction(QString(_("Show &Direction to %1")).arg(QString::fromStdString(center.body()->getName(true))), refVecMenu);
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

QMenu*
SelectionPopup::createAlternateSurfacesMenu()
{
    auto altSurfaces = GetBodyFeaturesManager()->getAlternateSurfaceNames(selection.body());
    if (!altSurfaces.has_value() || altSurfaces->empty())
        return nullptr;

    QMenu* surfacesMenu = new QMenu(_("&Alternate Surfaces"), this);
    QAction* normalAct = new QAction(_("Normal"), surfacesMenu);
    normalAct->setData(QString(""));
    surfacesMenu->addAction(normalAct);
    connect(normalAct, SIGNAL(triggered()), this, SLOT(slotSelectAlternateSurface()));

    for (const auto& surface : *altSurfaces)
    {
        QString surfaceName(surface.c_str());
        QAction* act = new QAction(surfaceName, surfacesMenu);
        act->setData(surfaceName);
        surfacesMenu->addAction(act);
        connect(act, SIGNAL(triggered()), this, SLOT(slotSelectAlternateSurface()));
    }

    addMenu(surfacesMenu);

    return surfacesMenu;
}

QMenu*
SelectionPopup::createObjectMenu(const PlanetarySystem* sys, BodyClassification classification)
{
    QMenu* menu = nullptr;

    if (sys != nullptr)
    {
        int nObjects = sys->getSystemSize();
        for (int i = 0; i < nObjects; i++)
        {
            Body* body = sys->getBody(i);
            if (util::is_set(body->getClassification(), classification))
            {
                if (menu == nullptr)
                {
                    QString title;
                    switch (classification)
                    {
                    case BodyClassification::Planet:
                        title = _("Planets");
                        break;
                    case BodyClassification::DwarfPlanet:
                        title = _("Dwarf planets");
                        break;
                    case BodyClassification::Moon:
                        title = _("Moons");
                        break;
                    case BodyClassification::MinorMoon:
                        title = _("Minor moons");
                        break;
                    case BodyClassification::Asteroid:
                        title = _("Asteroids");
                        break;
                    case BodyClassification::Comet:
                        title = _("Comets");
                        break;
                    case BodyClassification::Spacecraft:
                        title = C_("plural", "Spacecraft");
                        break;
                    default:
                        title = _("Other objects");
                        break;
                    }
                    menu = new QMenu(title, this);
                }

                QAction* act = new QAction(QString::fromStdString(body->getName(true)), menu);
                act->setData(i);
                connect(act, SIGNAL(triggered()), this, SLOT(slotSelectChildObject()));
                menu->addAction(act);
            }
        }
    }

    return menu;
}

void
SelectionPopup::addObjectMenus(const PlanetarySystem* sys)
{
    setStyleSheet("QMenu { menu-scrollable: 1; }"); //popupmenu with scrollbar
    QMenu* planetsMenu = createObjectMenu(sys, BodyClassification::Planet);
    if (planetsMenu != nullptr)
        addMenu(planetsMenu);

    QMenu* dwarfPlanetsMenu = createObjectMenu(sys, BodyClassification::DwarfPlanet);
    if (dwarfPlanetsMenu != nullptr)
        addMenu(dwarfPlanetsMenu);

    QMenu* moonsMenu = createObjectMenu(sys, BodyClassification::Moon);
    if (moonsMenu != nullptr)
        addMenu(moonsMenu);

    QMenu* minorMoonsMenu = createObjectMenu(sys, BodyClassification::MinorMoon);
    if (minorMoonsMenu != nullptr)
        addMenu(minorMoonsMenu);

    QMenu* asteroidsMenu = createObjectMenu(sys, BodyClassification::Asteroid);
    if (asteroidsMenu != nullptr)
        addMenu(asteroidsMenu);

    QMenu* cometsMenu = createObjectMenu(sys, BodyClassification::Comet);
    if (cometsMenu != nullptr)
        addMenu(cometsMenu);

    QMenu* spacecraftMenu = createObjectMenu(sys, BodyClassification::Spacecraft);
    if (spacecraftMenu != nullptr)
        addMenu(spacecraftMenu);
}

void
SelectionPopup::popupAtGoto(const QPoint& pt)
{
    exec(pt, gotoAction);
}

void
SelectionPopup::popupAtCenter(const QPoint& pt)
{
    exec(pt, centerAction);
}

/******** Slots *********/

void
SelectionPopup::slotSelect()
{
    appCore->getSimulation()->setSelection(selection);
}

void
SelectionPopup::slotCenterSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("c");
}

void
SelectionPopup::slotGotoSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("g");
}

void
SelectionPopup::slotFollowSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("f");
}

void
SelectionPopup::slotSyncOrbitSelection()
{
    appCore->getSimulation()->setSelection(selection);
    appCore->charEntered("y");
}

void
SelectionPopup::slotSelectAlternateSurface()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        const char* surfName = action->data().toString().toUtf8().constData();
        Simulation* sim = appCore->getSimulation();
        sim->getActiveObserver()->setDisplayedSurface(surfName);
    }
}

void
SelectionPopup::slotSelectPrimary()
{
    Body *selectedBody = selection.body();
    if (selectedBody != nullptr)
    {
        Simulation* sim = appCore->getSimulation();
        sim->setSelection(Helper::getPrimary(selectedBody));
    }
}

void
SelectionPopup::slotSelectChildObject()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action == nullptr)
        return;

    bool convertOK = false;
    int childIndex = action->data().toInt(&convertOK);
    if (!convertOK)
        return;

    Simulation* sim = appCore->getSimulation();
    const PlanetarySystem* sys = nullptr;
    if (selection.body() != nullptr)
    {
        sys = selection.body()->getSatellites();
    }
    else if (selection.star() != nullptr)
    {
        const SolarSystem* solarSys = sim->getUniverse()->getSolarSystem(selection.star());
        if (solarSys != nullptr)
            sys = solarSys->getPlanets();
    }

    if (sys != nullptr && childIndex >= 0 && childIndex < sys->getSystemSize())
        sim->setSelection(Selection(sys->getBody(childIndex)));
}

void
SelectionPopup::slotMark()
{
    using namespace celestia;

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
            appCore->getRenderer()->setRenderFlags(appCore->getRenderer()->getRenderFlags() | ::RenderFlags::ShowMarkers);
        }
    }
}

void
SelectionPopup::slotUnmark()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        Simulation* sim = appCore->getSimulation();
        sim->getUniverse()->unmarkObject(selection, 1);
    }
}

void
SelectionPopup::slotToggleBodyAxes()
{
    appCore->toggleReferenceMark("body axes", selection);
}

void
SelectionPopup::slotToggleFrameAxes()
{
    appCore->toggleReferenceMark("frame axes", selection);
}

void
SelectionPopup::slotToggleSunDirection()
{
    appCore->toggleReferenceMark("sun direction", selection);
}

void
SelectionPopup::slotToggleVelocityVector()
{
    appCore->toggleReferenceMark("velocity vector", selection);
}

void
SelectionPopup::slotToggleSpinVector()
{
    appCore->toggleReferenceMark("spin vector", selection);
}

void
SelectionPopup::slotToggleFrameCenterDirection()
{
    appCore->toggleReferenceMark("frame center direction", selection);
}

void
SelectionPopup::slotTogglePlanetographicGrid()
{
    appCore->toggleReferenceMark("planetographic grid", selection);
}

void
SelectionPopup::slotToggleTerminator()
{
    appCore->toggleReferenceMark("terminator", selection);
}

void
SelectionPopup::slotGotoStartDate()
{
    assert(selection.body() != nullptr);
    double startDate = 0.0;
    double endDate = 0.0;
    selection.body()->getLifespan(startDate, endDate);
    appCore->getSimulation()->setTime(startDate);
}

void
SelectionPopup::slotGotoEndDate()
{
    assert(selection.body() != nullptr);
    double startDate = 0.0;
    double endDate = 0.0;
    selection.body()->getLifespan(startDate, endDate);
    appCore->getSimulation()->setTime(endDate);
}

void
SelectionPopup::slotInfo()
{
    emit selectionInfoRequested(selection);
}

void
SelectionPopup::slotToggleVisibility(bool visible)
{
    assert(selection.body() != nullptr);
    selection.body()->setVisible(visible);
}

} // end namespace celestia::qt
