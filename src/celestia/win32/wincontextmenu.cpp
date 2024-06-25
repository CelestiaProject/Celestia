// wincontextmenu.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Windows context menu.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "wincontextmenu.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <celengine/body.h>
#include <celengine/dsodb.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/stardb.h>
#include <celengine/universe.h>
#include <celutil/gettext.h>

#include "res/resource.h"
#include "tstring.h"
#include "winmainwindow.h"

namespace celestia::win32
{

namespace
{

using IntStrPair = std::pair<int, tstring>;

struct IntStrPairComparer
{
    bool operator()(const IntStrPair& lhs, const IntStrPair& rhs) const;
};

bool
IntStrPairComparer::operator()(const IntStrPair& lhs, const IntStrPair& rhs) const
{
    if (lhs.second.empty())
        return rhs.second.empty() ? false : true;
    if (rhs.second.empty())
        return false;

#ifdef _UNICODE
    const auto length0 = static_cast<int>(lhs.second.size());
    const auto length1 = static_cast<int>(rhs.second.size());
    int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_LINGUISTIC_CASING,
                                 lhs.second.data(), length0, rhs.second.data(), length1,
                                 nullptr, nullptr, 0);
#else
    fmt::basic_memory_buffer<wchar_t, 256> wname0;
    int wlength0 = AppendCurrentCPToWide(lhs.second, wname0);
    if (wlength0 <= 0)
        return lhs < rhs;

    fmt::basic_memory_buffer<wchar_t, 256> wname1;
    int wlength1 = AppendCurrentCPToWide(rhs.second, wname1);
    if (wlength1 < 0)
        return lhs < rhs;

    int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_LINGUISTIC_CASING,
                                 wname0.data(), wlength0,
                                 wname1.data(), wlength1,
                                 nullptr, nullptr, 0);
#endif

    if (result == 0)
        return lhs < rhs;

    return result == CSTR_LESS_THAN;
}

HMENU
CreatePlanetarySystemMenu(std::string_view parentName, const PlanetarySystem* psys)
{
    // Use some vectors to categorize and sort the bodies within this PlanetarySystem.
    // In order to generate sorted menus, we must carry the name and menu index as a
    // single unit in the sort. One obvous way is to create a vector<pair<int,string>>
    // and then use a comparison predicate to sort.the vector based on the string in
    // each pair.

    // Declare vector<pair<int,string>> objects for each classification of body
    std::vector<IntStrPair> asteroids;
    std::vector<IntStrPair> comets;
    std::vector<IntStrPair> invisibles;
    std::vector<IntStrPair> moons;
    std::vector<IntStrPair> minorMoons;
    std::vector<IntStrPair> planets;
    std::vector<IntStrPair> dwarfPlanets;
    std::vector<IntStrPair> spacecraft;

    // We will use these objects to iterate over all the above vectors
    std::vector<std::vector<IntStrPair>> objects;
    std::vector<tstring> menuNames;

    // Place each body in the correct vector based on classification
    HMENU menu = CreatePopupMenu();
    for (int i = 0; i < psys->getSystemSize(); ++i)
    {
        Body* body = psys->getBody(i);
        if (!body->getName().empty())
        {
            switch(body->getClassification())
            {
            case BodyClassification::Asteroid:
                asteroids.push_back(std::make_pair(i, UTF8ToTString(body->getName(true))));
                break;
            case BodyClassification::Comet:
                comets.push_back(std::make_pair(i, UTF8ToTString(body->getName(true))));
                break;
            case BodyClassification::Invisible:
                invisibles.push_back(std::make_pair(i, UTF8ToTString(body->getName(true))));
                break;
            case BodyClassification::Moon:
                moons.push_back(std::make_pair(i, UTF8ToTString(body->getName(true))));
                break;
            case BodyClassification::MinorMoon:
                minorMoons.push_back(std::make_pair(i, UTF8ToTString(body->getName())));
                break;
            case BodyClassification::Planet:
                planets.push_back(std::make_pair(i, UTF8ToTString(body->getName(true))));
                break;
            case BodyClassification::DwarfPlanet:
                dwarfPlanets.push_back(std::make_pair(i, UTF8ToTString(body->getName())));
                break;
            case BodyClassification::Spacecraft:
                spacecraft.push_back(std::make_pair(i, UTF8ToTString(body->getName(true))));
                break;
            }
        }
    }

    // Add each vector of PlanetarySystem bodies to a vector to iterate over
    objects.push_back(asteroids);
    menuNames.push_back(UTF8ToTString(_("Asteroids")));
    objects.push_back(comets);
    menuNames.push_back(UTF8ToTString(_("Comets")));
    objects.push_back(invisibles);
    menuNames.push_back(UTF8ToTString(_("Invisibles")));
    objects.push_back(moons);
    menuNames.push_back(UTF8ToTString(_("Moons")));
    objects.push_back(minorMoons);
    menuNames.push_back(UTF8ToTString(_("Minor moons")));
    objects.push_back(planets);
    menuNames.push_back(UTF8ToTString(_("Planets")));
    objects.push_back(dwarfPlanets);
    menuNames.push_back(UTF8ToTString(_("Dwarf planets")));
    objects.push_back(spacecraft);

    // TRANSLATORS: translate this as plural
    menuNames.push_back(UTF8ToTString(C_("plural", "Spacecraft")));

    // Now sort each vector and generate submenus

    // Count how many submenus we need to create
    std::ptrdiff_t numSubMenus = std::count_if(objects.begin(), objects.end(),
                                               [](const auto& obj) { return !obj.empty(); });

    auto obj = objects.begin();
    auto menuName = menuNames.begin();
    while (obj != objects.end())
    {
        // Only generate a submenu if the vector is not empty
        if (obj->size() == 1)
        {
            // Don't create a submenu for a single item
            AppendMenu(menu, MF_STRING, MENU_CHOOSE_PLANET + obj->front().first, obj->front().second.c_str());
        }

        if (obj->size() <= 1)
        {
            ++obj;
            ++menuName;
            continue;
        }

        std::sort(obj->begin(), obj->end(), IntStrPairComparer{});
        if (numSubMenus > 1)
        {
            // Add items to submenu
            HMENU hSubMenu = CreatePopupMenu();
            for (const auto& subObj : *obj)
            {
                AppendMenu(hSubMenu, MF_STRING, MENU_CHOOSE_PLANET + subObj.first, subObj.second.c_str());
            }

            AppendMenu(menu, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(hSubMenu), menuName->c_str());
        }
        else
        {
            // Just add items to the popup
            for (const auto& subObj : *obj)
            {
                AppendMenu(menu, MF_STRING, MENU_CHOOSE_PLANET + subObj.first, subObj.second.c_str());
            }
        }

        ++obj;
        ++menuName;
    }

    return menu;
}

template<typename T>
HMENU
CreateAlternateSurfaceMenu(const T& surfaces)
{
    HMENU menu = CreatePopupMenu();

    // TRANSLATORS: normal texture in an alternative surface menu
    AppendMenu(menu, MF_STRING, MENU_CHOOSE_SURFACE, UTF8ToTString(_("Normal")).c_str());
    unsigned int i = 0;
    for (const auto& surface : surfaces)
    {
        ++i;
        AppendMenu(menu, MF_STRING, MENU_CHOOSE_SURFACE + i, UTF8ToTString(surface).c_str());
    }

    return menu;
}

}

WinContextMenuHandler::WinContextMenuHandler(const CelestiaCore* _appCore,
                                             HWND _hWnd,
                                             MainWindow* _mainWindow) :
    appCore(_appCore),
    hwnd(_hWnd),
    mainWindow(_mainWindow)
{
}

void
WinContextMenuHandler::requestContextMenu(float x, float y, Selection sel)
{
    HMENU hMenu;
    std::string name;

    hMenu = CreatePopupMenu();
    switch (sel.getType())
    {
    case SelectionType::Body:
        {
            name = sel.body()->getName(true);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, UTF8ToTString(name).c_str());
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, UTF8ToTString(_("&Goto")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, UTF8ToTString(_("&Follow")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_SYNCORBIT, UTF8ToTString(_("S&ync Orbit")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_INFO, UTF8ToTString(_("&Info")).c_str());
            HMENU refVectorMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(refVectorMenu), UTF8ToTString(_("&Reference Marks")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_BODY_AXES, UTF8ToTString(_("Show Body Axes")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_FRAME_AXES, UTF8ToTString(_("Show Frame Axes")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_SUN_DIRECTION, UTF8ToTString(_("Show Sun Direction")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_VELOCITY_VECTOR, UTF8ToTString(_("Show Velocity Vector")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_PLANETOGRAPHIC_GRID, UTF8ToTString(_("Show Planetographic Grid")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_TERMINATOR, UTF8ToTString(_("Show Terminator")).c_str());

            const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();
            CheckMenuItem(refVectorMenu, ID_RENDER_BODY_AXES, bodyFeaturesManager->findReferenceMark(sel.body(), "body axes") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_FRAME_AXES, bodyFeaturesManager->findReferenceMark(sel.body(), "frame axes") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_SUN_DIRECTION, bodyFeaturesManager->findReferenceMark(sel.body(), "sun direction") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_VELOCITY_VECTOR, bodyFeaturesManager->findReferenceMark(sel.body(), "velocity vector") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_PLANETOGRAPHIC_GRID, bodyFeaturesManager->findReferenceMark(sel.body(), "planetographic grid") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_TERMINATOR, bodyFeaturesManager->findReferenceMark(sel.body(), "terminator") ? MF_CHECKED : MF_UNCHECKED);

            AppendMenu(hMenu, MF_STRING, ID_SELECT_PRIMARY_BODY, UTF8ToTString(_("Select &Primary Body")).c_str());

            if (const PlanetarySystem* satellites = sel.body()->getSatellites();
                satellites != nullptr && satellites->getSystemSize() != 0)
            {
                HMENU satMenu = CreatePlanetarySystemMenu(name, satellites);
                AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT_PTR) satMenu,
                           UTF8ToTString(_("&Satellites")).c_str());
            }

            if (auto altSurfaces = bodyFeaturesManager->getAlternateSurfaceNames(sel.body());
                altSurfaces.has_value() && !altSurfaces->empty())
            {
                HMENU surfMenu = CreateAlternateSurfaceMenu(*altSurfaces);
                AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT_PTR) surfMenu,
                           UTF8ToTString(_("&Alternate Surfaces")).c_str());
            }
        }
        break;

    case SelectionType::Star:
        {
            Simulation* sim = appCore->getSimulation();
            name = sim->getUniverse()->getStarCatalog()->getStarName(*(sel.star()));
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, UTF8ToTString(name).c_str());
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, UTF8ToTString(_("&Goto")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_INFO, UTF8ToTString(_("&Info")).c_str());

            const SolarSystem* solarSys = sim->getUniverse()->getSolarSystem(sel.star());
            if (solarSys != nullptr)
            {
                HMENU planetsMenu = CreatePlanetarySystemMenu(name, solarSys->getPlanets());
                if (name == "Sol")
                    AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT_PTR) planetsMenu, UTF8ToTString(_("Orbiting Bodies")).c_str());
                else
                    AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT_PTR) planetsMenu, UTF8ToTString(_("Planets")).c_str());
            }
        }
        break;

    case SelectionType::DeepSky:
        {
            Simulation* sim = appCore->getSimulation();
            name = sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, UTF8ToTString(name).c_str());
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, UTF8ToTString(_("&Goto")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, UTF8ToTString(_("&Follow")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_INFO, UTF8ToTString(_("&Info")).c_str());
        }
        break;

    case SelectionType::Location:
        break;

    default:
        break;
    }

    if (appCore->getSimulation()->getUniverse()->isMarked(sel, 1))
        AppendMenu(hMenu, MF_STRING, ID_TOOLS_UNMARK, UTF8ToTString(_("&Unmark")).c_str());
    else
        AppendMenu(hMenu, MF_STRING, ID_TOOLS_MARK, UTF8ToTString(_("&Mark")).c_str());

    POINT point;
    point.x = (int) x;
    point.y = (int) y;

    if (!mainWindow->fullScreen())
        ClientToScreen(hwnd, (LPPOINT) &point);

    appCore->getSimulation()->setSelection(sel);
    TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hwnd, NULL);

    // TODO: Do we need to explicitly destroy submenus or does DestroyMenu
    // work recursively?
    // According to the MSDN documentation, DestroyMenu() IS recursive. Clint 11/01.
    DestroyMenu(hMenu);

    mainWindow->ignoreNextMove();
}

}
