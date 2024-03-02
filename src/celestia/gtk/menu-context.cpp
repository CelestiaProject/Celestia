/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: menu-context.cpp,v 1.4 2008-01-03 00:20:33 vincent_gian Exp $
 */

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <gtk/gtk.h>

#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celestia/helper.h>
#include <celutil/greek.h>

#include "menu-context.h"
#include "actions.h"
#include "common.h"

namespace celestia::gtk
{

namespace
{

/* There is no way to pass the AppData struct to the menu at this time. This
 * keeps the global variable local to this file, at least conceptually. */
AppData* app;

/* CALLBACK: Wrap a GtkAction. The whole context menu should be replaced at
 *           some point */
void
wrapAction(GtkAction* action)
{
    gtk_action_activate(action);
}

/* CALLBACK: Mark the selected object. Might some day be a GtkAction. */
void
menuMark()
{
    Simulation* sim = app->simulation;
    if (sim->getUniverse() != nullptr)
    {
        using namespace celestia;
        MarkerRepresentation markerRep(MarkerRepresentation::Diamond, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f));
        sim->getUniverse()->markObject(sim->getSelection(), markerRep, 1);
    }
}

/* CALLBACK: Unmark the selected object. Might some day be a GtkAction. */
void
menuUnMark()
{
    Simulation* sim = app->simulation;
    if (sim->getUniverse() != nullptr)
        sim->getUniverse()->unmarkObject(sim->getSelection(), 1);
}

/* CALLBACK: Handle a planetary selection from the context menu. */
void
handleContextPlanet(gpointer data)
{
    int value = GPOINTER_TO_INT(data);

    /* Handle the satellite/child object submenu */
    Selection sel = app->simulation->getSelection();
    switch (sel.getType())
    {
        case SelectionType::Star:
            app->simulation->selectPlanet(value);
            break;

        case SelectionType::Body:
            {
                PlanetarySystem* satellites = (PlanetarySystem*) sel.body()->getSatellites();
                app->simulation->setSelection(Selection(satellites->getBody(value)));
                break;
            }

        case SelectionType::DeepSky:
            /* Current deep sky object/galaxy implementation does
             * not have children to select. */
            break;

        case SelectionType::Location:
            break;

        default:
            break;
    }
}

/* CALLBACK: Handle an object's primary selection */
void
handleContextPrimary()
{
    Selection sel = app->simulation->getSelection();
    if (sel.body() != nullptr)
    {
        app->simulation->setSelection(Helper::getPrimary(sel.body()));
    }
}

/* CALLBACK: Handle an alternate surface from the context menu. */
void
handleContextSurface(gpointer data)
{
    int value = GPOINTER_TO_INT(data);

    /* Handle the alternate surface submenu */
    Selection sel = app->simulation->getSelection();
    const Body* body = sel.body();
    if (body == nullptr)
        return;

    guint index = value - 1;
    auto surfNames = GetBodyFeaturesManager()->getAlternateSurfaceNames(body);
    if (!surfNames.has_value())
        return;

    if (index < surfNames->size())
    {
        auto it = surfNames->begin();
        std::advance(it, index);
        app->simulation->getActiveObserver()->setDisplayedSurface(*it);
    }
    else
    {
        app->simulation->getActiveObserver()->setDisplayedSurface({});
    }
}

/* HELPER: Append a menu item and return pointer. Used for context menu. */
GtkMenuItem*
AppendMenu(GtkWidget* parent, GCallback callback, const gchar* name, gpointer extra)
{
    GtkWidget* menuitem;
    gpointer data;

    /* Check for separator */
    if (name == nullptr)
        menuitem = gtk_separator_menu_item_new();
    else
        menuitem = gtk_menu_item_new_with_mnemonic(name);

    /* If no callback was provided, pass GtkAction, else convert to pointer */
    if (callback == nullptr && extra != 0)
    {
        callback = G_CALLBACK(wrapAction);
        data = extra;
    }
    else
        data = GINT_TO_POINTER(extra);

    /* Add handler */
    if (callback != nullptr)
        g_signal_connect_swapped (G_OBJECT(menuitem), "activate",
                                  G_CALLBACK(callback),
                                  data);

    gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
    return GTK_MENU_ITEM(menuitem);
}

/* Typedefs and structs for sorting objects by name in context menu. */
using IntStrPair = std::pair<int, std::string>;
using IntStrPairVec = std::vector<IntStrPair>;

struct IntStrPairComparePredicate
{
    IntStrPairComparePredicate() : dummy(0) {}

    bool operator()(const IntStrPair& pair1, const IntStrPair& pair2) const
    {
        return (pair1.second.compare(pair2.second) < 0);
    }

    int dummy;
};

/* HELPER: Create planetary submenu for context menu, return menu pointer. */
GtkMenu*
CreatePlanetarySystemMenu(const std::string& parentName, const PlanetarySystem* psys)
{
    /*
     * Modified from winmain.cpp
     *
     * Use some vectors to categorize and sort the bodies within this
     * PlanetarySystem. In order to generate sorted menus, we must carry the
     * name and menu index as a single unit in the sort. One obvious way is to
     * create a vector<pair<int,string>> and then use a comparison predicate
     * to sort the vector based on the string in each pair.
     */

    /* Declare vector<pair<int,string>> objects for each classification of body */
    std::vector<IntStrPair> asteroids;
    std::vector<IntStrPair> comets;
    std::vector<IntStrPair> invisibles;
    std::vector<IntStrPair> moons;
    std::vector<IntStrPair> minorMoons;
    std::vector<IntStrPair> planets;
    std::vector<IntStrPair> dwarfPlanets;
    std::vector<IntStrPair> spacecraft;

    /* We will use these objects to iterate over all the above vectors */
    std::vector<IntStrPairVec> objects;
    std::vector<std::string> menuNames;

    /* Place each body in the correct vector based on classification */
    GtkWidget* menu = gtk_menu_new();
    for (int i = 0; i < psys->getSystemSize(); i++)
    {
        const Body* body = psys->getBody(i);
        switch(body->getClassification())
        {
            case BodyClassification::Asteroid:
                asteroids.push_back(make_pair(i, body->getName()));
                break;
            case BodyClassification::Comet:
                comets.push_back(make_pair(i, body->getName()));
                break;
            case BodyClassification::Invisible:
                invisibles.push_back(make_pair(i, body->getName()));
                break;
            case BodyClassification::Moon:
                moons.push_back(make_pair(i, body->getName()));
                break;
            case BodyClassification::MinorMoon:
                minorMoons.push_back(make_pair(i, body->getName()));
                break;
            case BodyClassification::Planet:
                planets.push_back(make_pair(i, body->getName()));
                break;
            case BodyClassification::DwarfPlanet:
                dwarfPlanets.push_back(make_pair(i, body->getName()));
                break;
            case BodyClassification::Spacecraft:
                spacecraft.push_back(make_pair(i, body->getName()));
                break;
        }
    }

    /* Add each vector of PlanetarySystem bodies to a vector to iterate over */
    objects.push_back(asteroids);
    menuNames.push_back("Asteroids");
    objects.push_back(comets);
    menuNames.push_back("Comets");
    objects.push_back(invisibles);
    menuNames.push_back("Invisibles");
    objects.push_back(moons);
    menuNames.push_back("Moons");
    objects.push_back(minorMoons);
    menuNames.push_back("Minor moons");
    objects.push_back(planets);
    menuNames.push_back("Planets");
    objects.push_back(dwarfPlanets);
    menuNames.push_back("Dwarf planets");
    objects.push_back(spacecraft);
    menuNames.push_back("Spacecraft");

    /* Now sort each vector and generate submenus */
    IntStrPairComparePredicate pred;
    std::vector<IntStrPairVec>::iterator obj;
    std::vector<IntStrPair>::iterator it;
    std::vector<std::string>::iterator menuName;
    GtkWidget* subMenu;
    int numSubMenus;

    /* Count how many submenus we need to create */
    numSubMenus = 0;
    for (obj=objects.begin(); obj != objects.end(); obj++)
    {
        if (obj->size() > 0)
            numSubMenus++;
    }

    menuName = menuNames.begin();
    for (obj=objects.begin(); obj != objects.end(); obj++)
    {
        /* Only generate a submenu if the vector is not empty */
        if (obj->size() > 0)
        {
            /* Don't create a submenu for a single item */
            if (obj->size() == 1)
            {
                it=obj->begin();
                AppendMenu(menu, G_CALLBACK(handleContextPlanet), it->second.c_str(), GINT_TO_POINTER(it->first));
            }
            else
            {
                /* Skip sorting if we are dealing with the planets in our own
                 * Solar System. */
                if (parentName != "Sol" || *menuName != "Planets")
                    sort(obj->begin(), obj->end(), pred);

                if (numSubMenus > 1)
                {
                    /* Add items to submenu */
                    subMenu = gtk_menu_new();

                    for(it=obj->begin(); it != obj->end(); it++)
                        AppendMenu(subMenu, G_CALLBACK(handleContextPlanet), it->second.c_str(), GINT_TO_POINTER(it->first));

                    gtk_menu_item_set_submenu(AppendMenu(menu, nullptr, menuName->c_str(), 0), GTK_WIDGET(subMenu));
                }
                else
                {
                    /* Just add items to the popup */
                    for(it=obj->begin(); it != obj->end(); it++)
                        AppendMenu(menu, G_CALLBACK(handleContextPlanet), it->second.c_str(), GINT_TO_POINTER(it->first));
                }
            }
        }
        menuName++;
    }

    return GTK_MENU(menu);
}

/* HELPER: Create surface submenu for context menu, return menu pointer. */
template<typename T>
GtkMenu*
CreateAlternateSurfaceMenu(const T& surfaces)
{
    GtkWidget* menu = gtk_menu_new();

    AppendMenu(menu, G_CALLBACK(handleContextSurface), "Normal", 0);
    guint i = 0;
    for (const auto& surface : surfaces)
    {
        ++i;
        AppendMenu(menu, G_CALLBACK(handleContextSurface), surface.c_str(), GINT_TO_POINTER(i));
    }

    return GTK_MENU(menu);
}

} // end unnamed namespace

/* Initializer. Sets the appData */
GTKContextMenuHandler::GTKContextMenuHandler(AppData* _app) :
    CelestiaCore::ContextMenuHandler()
{
    // FIXME: a workaround to have it referenced by any menu callback
    app = _app;
}

/* ENTRY: Context menu (event handled by appCore)
 *        Normally, float x and y, but unused, so removed. */
void GTKContextMenuHandler::requestContextMenu(float, float, Selection sel)
{
    GtkWidget* popup = gtk_menu_new();
    std::string name;

    switch (sel.getType())
    {
        case SelectionType::Body:
        {
            name = sel.body()->getName();
            AppendMenu(popup, nullptr, name.c_str(), gtk_action_group_get_action(app->agMain, "CenterSelection"));
            AppendMenu(popup, nullptr, nullptr, 0);
            AppendMenu(popup, nullptr, "_Goto", gtk_action_group_get_action(app->agMain, "GotoSelection"));
            AppendMenu(popup, nullptr, "_Follow", gtk_action_group_get_action(app->agMain, "FollowSelection"));
            AppendMenu(popup, nullptr, "S_ync Orbit", gtk_action_group_get_action(app->agMain, "SyncSelection"));
            /* Add info eventually:
             * AppendMenu(popup, nullptr, "_Info", 0); */
            if (Helper::hasPrimary(sel.body()))
            {
                AppendMenu(popup, G_CALLBACK(handleContextPrimary), "Select _Primary Body", nullptr);
            }

            if (const PlanetarySystem* satellites = sel.body()->getSatellites();
                satellites != nullptr && satellites->getSystemSize() != 0)
            {
                GtkMenu* satMenu = CreatePlanetarySystemMenu(name, satellites);
                gtk_menu_item_set_submenu(AppendMenu(popup, nullptr, "_Satellites", 0), GTK_WIDGET(satMenu));
            }

            if (auto altSurfaces = GetBodyFeaturesManager()->getAlternateSurfaceNames(sel.body());
                altSurfaces.has_value() && !altSurfaces->empty())
            {
                GtkMenu* surfMenu = CreateAlternateSurfaceMenu(*altSurfaces);
                gtk_menu_item_set_submenu(AppendMenu(popup, nullptr, "_Alternate Surfaces", 0), GTK_WIDGET(surfMenu));
            }
        }
        break;

        case SelectionType::Star:
        {
            Simulation* sim = app->simulation;
            name = ReplaceGreekLetterAbbr(sim->getUniverse()->getStarCatalog()->getStarName(*(sel.star())));
            AppendMenu(popup, nullptr, name.c_str(), gtk_action_group_get_action(app->agMain, "CenterSelection"));
            AppendMenu(popup, nullptr, nullptr, 0);
            AppendMenu(popup, nullptr, "_Goto", gtk_action_group_get_action(app->agMain, "GotoSelection"));
            /* Add info eventually:
             * AppendMenu(popup, nullptr, "_Info", 0); */

            const SolarSystem* solarSys = sim->getUniverse()->getSolarSystem(sel.star());
            if (solarSys != nullptr)
            {
                GtkMenu* planetsMenu = CreatePlanetarySystemMenu(name, solarSys->getPlanets());
                if (name == "Sol")
                    gtk_menu_item_set_submenu(AppendMenu(popup, nullptr, "Orbiting Bodies", 0), GTK_WIDGET(planetsMenu));
                else
                    gtk_menu_item_set_submenu(AppendMenu(popup, nullptr, "Planets", 0), GTK_WIDGET(planetsMenu));
            }
        }
        break;

        case SelectionType::DeepSky:
        {
            AppendMenu(popup, nullptr, app->simulation->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky()).c_str(), gtk_action_group_get_action(app->agMain, "CenterSelection"));
            AppendMenu(popup, nullptr, nullptr, 0);
            AppendMenu(popup, nullptr, "_Goto", gtk_action_group_get_action(app->agMain, "GotoSelection"));
            AppendMenu(popup, nullptr, "_Follow", gtk_action_group_get_action(app->agMain, "FollowSelection"));
            /* Add info eventually:
             * AppendMenu(popup, nullptr, "_Info", 0); */
        }
        break;

        case SelectionType::Location:
            break;

        default:
            break;
    }

    if (app->simulation->getUniverse()->isMarked(sel, 1))
        AppendMenu(popup, menuUnMark, "_Unmark", 0);
    else
        AppendMenu(popup, menuMark, "_Mark", 0);

    app->simulation->setSelection(sel);

    gtk_widget_show_all(popup);
    gtk_menu_popup(GTK_MENU(popup), nullptr, nullptr, nullptr, nullptr, 0, gtk_get_current_event_time());
}

} // end namespace celestia::gtk
