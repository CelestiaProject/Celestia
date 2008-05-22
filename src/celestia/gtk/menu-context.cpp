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
#include <gtk/gtk.h>

#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celutil/utf8.h>

#include "menu-context.h"
#include "actions.h"
#include "common.h"


/* Definitions: Callbacks */
static void wrapAction(GtkAction* action);
static void menuMark();
static void menuUnMark();
static void handleContextPlanet(gpointer data);
static void handleContextSurface(gpointer data);

/* Definitions: Helpers */
static GtkMenuItem* AppendMenu(GtkWidget* parent, GtkSignalFunc callback, const gchar* name, gpointer extra);
static GtkMenu* CreatePlanetarySystemMenu(string parentName, const PlanetarySystem* psys);
static GtkMenu* CreateAlternateSurfaceMenu(const vector<string>& surfaces);


/* There is no way to pass the AppData struct to the menu at this time. This
 * keeps the global variable local to this file, at least conceptually. */
static AppData* app;


/* Initializer. Sets the appData, since there is no way to pass it. */
void initContext(AppData* a)
{
	app = a;
}


/* ENTRY: Context menu (event handled by appCore)
 *        Normally, float x and y, but unused, so removed. */
void menuContext(float, float, Selection sel)
{
	GtkWidget* popup = gtk_menu_new();
	string name;

	switch (sel.getType())
	{
		case Selection::Type_Body:
		{
			name = sel.body()->getName();
			AppendMenu(popup, NULL, name.c_str(), gtk_action_group_get_action(app->agMain, "CenterSelection"));
			AppendMenu(popup, NULL, NULL, 0);
			AppendMenu(popup, NULL, "_Goto", gtk_action_group_get_action(app->agMain, "GotoSelection"));
			AppendMenu(popup, NULL, "_Follow", gtk_action_group_get_action(app->agMain, "FollowSelection"));
			AppendMenu(popup, NULL, "S_ync Orbit", gtk_action_group_get_action(app->agMain, "SyncSelection"));
			/* Add info eventually:
			 * AppendMenu(popup, NULL, "_Info", 0); */

			const PlanetarySystem* satellites = sel.body()->getSatellites();
			if (satellites != NULL && satellites->getSystemSize() != 0)
			{
				GtkMenu* satMenu = CreatePlanetarySystemMenu(name, satellites);
				gtk_menu_item_set_submenu(AppendMenu(popup, NULL, "_Satellites", 0), GTK_WIDGET(satMenu));
			}

			vector<string>* altSurfaces = sel.body()->getAlternateSurfaceNames();
			if (altSurfaces != NULL)
			{
				if (altSurfaces->size() > 0)
				{
					GtkMenu* surfMenu = CreateAlternateSurfaceMenu(*altSurfaces);
					gtk_menu_item_set_submenu(AppendMenu(popup, NULL, "_Alternate Surfaces", 0), GTK_WIDGET(surfMenu));
					delete altSurfaces;
				}
			}
		}
		break;

		case Selection::Type_Star:
		{
			Simulation* sim = app->simulation;
			name = ReplaceGreekLetterAbbr(sim->getUniverse()->getStarCatalog()->getStarName(*(sel.star())));
			AppendMenu(popup, NULL, name.c_str(), gtk_action_group_get_action(app->agMain, "CenterSelection"));
			AppendMenu(popup, NULL, NULL, 0);
			AppendMenu(popup, NULL, "_Goto", gtk_action_group_get_action(app->agMain, "GotoSelection"));
			/* Add info eventually:
			 * AppendMenu(popup, NULL, "_Info", 0); */

			SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
			SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star()->getCatalogNumber());
			if (iter != solarSystemCatalog->end())
			{
				SolarSystem* solarSys = iter->second;
				GtkMenu* planetsMenu = CreatePlanetarySystemMenu(name, solarSys->getPlanets());
				if (name == "Sol")
					gtk_menu_item_set_submenu(AppendMenu(popup, NULL, "Orbiting Bodies", 0), GTK_WIDGET(planetsMenu));
				else
					gtk_menu_item_set_submenu(AppendMenu(popup, NULL, "Planets", 0), GTK_WIDGET(planetsMenu));
			}
		}
		break;

		case Selection::Type_DeepSky:
		{
			AppendMenu(popup, NULL, app->simulation->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky()).c_str(), gtk_action_group_get_action(app->agMain, "CenterSelection"));
			AppendMenu(popup, NULL, NULL, 0);
			AppendMenu(popup, NULL, "_Goto", gtk_action_group_get_action(app->agMain, "GotoSelection"));
			AppendMenu(popup, NULL, "_Follow", gtk_action_group_get_action(app->agMain, "FollowSelection"));
			/* Add info eventually:
			 * AppendMenu(popup, NULL, "_Info", 0); */
		}
		break;

		case Selection::Type_Location:
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
	gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


/* CALLBACK: Wrap a GtkAction. The whole context menu should be replaced at
 *           some point */
static void wrapAction(GtkAction* action)
{
	gtk_action_activate(action);
}


/* CALLBACK: Mark the selected object. Might some day be a GtkAction. */
static void menuMark()
{
	Simulation* sim = app->simulation;
	if (sim->getUniverse() != NULL)
	{
		MarkerRepresentation markerRep(MarkerRepresentation::Diamond, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f));
		sim->getUniverse()->markObject(sim->getSelection(), markerRep, 1);
	}
}


/* CALLBACK: Unmark the selected object. Might some day be a GtkAction. */
static void menuUnMark()
{
	Simulation* sim = app->simulation;
	if (sim->getUniverse() != NULL)
		sim->getUniverse()->unmarkObject(sim->getSelection(), 1);
}


/* CALLBACK: Handle a planetary selection from the context menu. */
static void handleContextPlanet(gpointer data)
{
	int value = GPOINTER_TO_INT(data);
	
	/* Handle the satellite/child object submenu */
	Selection sel = app->simulation->getSelection();
	switch (sel.getType())
	{
		case Selection::Type_Star:
			app->simulation->selectPlanet(value);
			break;

		case Selection::Type_Body:
			{
				PlanetarySystem* satellites = (PlanetarySystem*) sel.body()->getSatellites();
				app->simulation->setSelection(Selection(satellites->getBody(value)));
				break;
			}

		case Selection::Type_DeepSky:
			/* Current deep sky object/galaxy implementation does
			 * not have children to select. */
			break;

		case Selection::Type_Location:
			break;

		default:
			break;
	}
}


/* CALLBACK: Handle an alternate surface from the context menu. */
static void handleContextSurface(gpointer data)
{
	int value = GPOINTER_TO_INT(data);
	
	/* Handle the alternate surface submenu */
	Selection sel = app->simulation->getSelection();
	if (sel.body() != NULL)
	{
		guint index = value - 1;
		vector<string>* surfNames = sel.body()->getAlternateSurfaceNames();
		if (surfNames != NULL)
		{
			string surfName;
			if (index < surfNames->size())
				surfName = surfNames->at(index);
			app->simulation->getActiveObserver()->setDisplayedSurface(surfName);
			delete surfNames;
		}
	}
}


/* HELPER: Append a menu item and return pointer. Used for context menu. */
static GtkMenuItem* AppendMenu(GtkWidget* parent, GtkSignalFunc callback, const gchar* name, gpointer extra)
{
	GtkWidget* menuitem;
	gpointer data;

	/* Check for separator */
	if (name == NULL)
		menuitem = gtk_separator_menu_item_new();
	else
		menuitem = gtk_menu_item_new_with_mnemonic(name);

	/* If no callback was provided, pass GtkAction, else convert to pointer */
	if (callback == NULL && extra != 0)
	{
		callback = G_CALLBACK(wrapAction);
		data = extra;
	}
	else
		data = GINT_TO_POINTER(extra);
	
	/* Add handler */
	if (callback != NULL)
		g_signal_connect_swapped (G_OBJECT(menuitem), "activate",
		                          G_CALLBACK(callback),
		                          data);

	gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
	return GTK_MENU_ITEM(menuitem);
}


/* Typedefs and structs for sorting objects by name in context menu. */
typedef pair<int,string> IntStrPair;
typedef vector<IntStrPair> IntStrPairVec;

struct IntStrPairComparePredicate
{
    IntStrPairComparePredicate() : dummy(0) {}

	bool operator()(const IntStrPair pair1, const IntStrPair pair2) const
	{
		return (pair1.second.compare(pair2.second) < 0);
	}

	int dummy;
};

/* HELPER: Create planetary submenu for context menu, return menu pointer. */
static GtkMenu* CreatePlanetarySystemMenu(string parentName, const PlanetarySystem* psys)
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
	vector<IntStrPair> asteroids;
	vector<IntStrPair> comets;
	vector<IntStrPair> invisibles;
	vector<IntStrPair> moons;
	vector<IntStrPair> planets;
	vector<IntStrPair> spacecraft;

	/* We will use these objects to iterate over all the above vectors */
	vector<IntStrPairVec> objects;
	vector<string> menuNames;

	/* Place each body in the correct vector based on classification */
	GtkWidget* menu = gtk_menu_new();
	for (int i = 0; i < psys->getSystemSize(); i++)
	{
		Body* body = psys->getBody(i);
		switch(body->getClassification())
		{
			case Body::Asteroid:
				asteroids.push_back(make_pair(i, body->getName()));
				break;
			case Body::Comet:
				comets.push_back(make_pair(i, body->getName()));
				break;
			case Body::Invisible:
				invisibles.push_back(make_pair(i, body->getName()));
				break;
			case Body::Moon:
				moons.push_back(make_pair(i, body->getName()));
				break;
			case Body::Planet:
				planets.push_back(make_pair(i, body->getName()));
				break;
			case Body::Spacecraft:
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
	objects.push_back(planets);
	menuNames.push_back("Planets");
	objects.push_back(spacecraft);
	menuNames.push_back("Spacecraft");

	/* Now sort each vector and generate submenus */
	IntStrPairComparePredicate pred;
	vector<IntStrPairVec>::iterator obj;
	vector<IntStrPair>::iterator it;
	vector<string>::iterator menuName;
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
				AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextPlanet), it->second.c_str(), GINT_TO_POINTER(it->first));
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
						AppendMenu(subMenu, GTK_SIGNAL_FUNC(handleContextPlanet), it->second.c_str(), GINT_TO_POINTER(it->first));

					gtk_menu_item_set_submenu(AppendMenu(menu, NULL, menuName->c_str(), 0), GTK_WIDGET(subMenu));
				}
				else
				{
					/* Just add items to the popup */
					for(it=obj->begin(); it != obj->end(); it++)
						AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextPlanet), it->second.c_str(), GINT_TO_POINTER(it->first));
				}
			}
		}
		menuName++;
	}

	return GTK_MENU(menu);
}


/* HELPER: Create surface submenu for context menu, return menu pointer. */
static GtkMenu* CreateAlternateSurfaceMenu(const vector<string>& surfaces)
{
	GtkWidget* menu = gtk_menu_new();

	AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextSurface), "Normal", 0);
	for (guint i = 0; i < surfaces.size(); i++)
	{
		AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextSurface), surfaces[i].c_str(), GINT_TO_POINTER(i+1));
	}

	return GTK_MENU(menu);
}
