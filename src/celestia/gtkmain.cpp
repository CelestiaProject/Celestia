// gtkmain.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// GTK front end for Celestia.
// GTK2 port by Pat Suwalski <pat@suwalski.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <celengine/gl.h>
#include <celengine/glext.h>
#include <celengine/celestia.h>
#include <celengine/starbrowser.h>

#ifndef DEBUG
#  define G_DISABLE_ASSERT
#endif

#ifdef GNOME
#include <gnome.h>
#include <libgnomeui/libgnomeui.h>
#include <gconf/gconf-client.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include "celmath/vecmath.h"
#include "celmath/quaternion.h"
#include "celmath/mathlib.h"
#include "celengine/astro.h"
#include "celengine/cmdparser.h"
#include "celutil/util.h"
#include "celutil/filetype.h"
#include "celutil/debug.h"
#include "imagecapture.h"
#include "celestiacore.h"
#include "celengine/simulation.h"
#include "eclipsefinder.h"

#define CELSPACING 8

using namespace std;

char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;
static Renderer* appRenderer = NULL;
static Simulation* appSim = NULL;

// Variables used throughout
static const int MinListStars = 10;
static const int MaxListStars = 500;
static int numListStars = 100;

// Mouse motion tracking
static int lastX = 0;
static int lastY = 0;

// GConf for Gnome
#ifdef GNOME
GConfClient *client;
#endif
	
typedef struct _checkFunc CheckFunc;
typedef int (*Callback)(int, char *);

struct _checkFunc
{
  GtkCheckMenuItem *widget;  /* These two will be filled in by init */
  GtkCheckButton *optWidget;
  const char *path;
  Callback func;
  int active;
  int funcdata;
  int action;
  GtkSignalFunc sigFunc;
};

static GtkWidget* mainWindow = NULL;
static GtkWidget* mainMenu = NULL;
static GtkWidget* mainBox = NULL;
static GtkWidget* oglArea = NULL;
int oglAttributeList[] = 
{
    GDK_GL_RGBA,
    GDK_GL_RED_SIZE, 1,
    GDK_GL_GREEN_SIZE, 1,
    GDK_GL_BLUE_SIZE, 1,
    GDK_GL_DEPTH_SIZE, 1,
    GDK_GL_DOUBLEBUFFER,
    GDK_GL_NONE,
};

struct AppPreferences
{
	int winWidth;
	int winHeight;
	int winX;
	int winY;
	int renderFlags;
	int labelMode;
	int orbitMask;
	float visualMagnitude;
	float ambientLight;
	int showLocalTime;
	int hudDetail;
	int fullScreen;
	string altSurfaceName;
	Renderer::StarStyle starStyle;
};
AppPreferences* prefs;

static GtkItemFactory* menuItemFactory = NULL;

static int verbose;

// enums for distinguising between check items
enum
{
    Menu_ShowGalaxies        = 2001,
    Menu_ShowOrbits          = 2002,
    Menu_ShowConstellations  = 2003,
    Menu_ShowAtmospheres     = 2004,
    Menu_PlanetLabels        = 2005,
    Menu_ShowClouds          = 2006,
    Menu_ShowCelestialSphere = 2007,
    Menu_ShowNightSideMaps   = 2008,
    Menu_MoonLabels          = 2009,
    Menu_AsteroidLabels      = 2010,
    Menu_StarLabels          = 2011,
    Menu_GalaxyLabels        = 2012,
    Menu_ConstellationLabels = 2013,
//  Menu_VertexShaders       = 2014,
    Menu_ShowLocTime         = 2015,
    Menu_ShowEclipseShadows  = 2016,
//  Menu_ShowStarsAsPoints   = 2017,
    Menu_CraftLabels         = 2018,
    Menu_ShowBoundaries      = 2019,
    Menu_AntiAlias           = 2020,
    Menu_AutoMag             = 2021,
    Menu_ShowCometTails      = 2022,
    Menu_ShowPlanets         = 2023,
    Menu_ShowRingShadows     = 2024,
    Menu_ShowStars           = 2025,
	Menu_ShowFrames          = 2026,
	Menu_SyncTime            = 2027,
	Menu_StarFuzzy           = 2028,
	Menu_StarPoints          = 2029,
	Menu_StarDiscs           = 2030,
	Menu_AmbientNone         = 2031,
	Menu_AmbientLow          = 2032,
	Menu_AmbientMed          = 2033,
	Menu_CometLabels         = 2034,
	Menu_LocationLabels      = 2035,
	Menu_AsteroidOrbits      = 2036,
	Menu_CometOrbits         = 2037,
	Menu_MoonOrbits          = 2038,
	Menu_PlanetOrbits        = 2039,
	Menu_SpacecraftOrbits    = 2040,
	Menu_FullScreen          = 2041,
};

static void menuSelectSol()
{
    appCore->charEntered('H');
}

static void menuCenter()
{
    appCore->charEntered('c');
}

static void menuGoto()
{
    appCore->charEntered('G');
}

static void menuSync()
{
    appCore->charEntered('Y');
}

static void menuTrack()
{
    appCore->charEntered('T');
}

static void menuFollow()
{
    appCore->charEntered('F');
}

static void menuFaster()
{
    appCore->charEntered('L');
}

static void menuSlower()
{
    appCore->charEntered('K');
}

static void menuPause()
{
    appCore->charEntered(' ');
}

static void menuRealTime()
{
    appCore->charEntered('\\');
}

static void menuReverse()
{
    appCore->charEntered('J');
}

static void menuViewSplitH()
{
	appCore->splitView(View::HorizontalSplit);
}

static void menuViewSplitV()
{
	appCore->splitView(View::VerticalSplit);
}

static void menuViewDelete()
{
	appCore->deleteView();
}

static void menuViewSingle()
{
	appCore->singleView();
}

static void menuViewShowFrames()
{
	appCore->setFramesVisible(!appCore->getFramesVisible());
}

static void menuViewSyncTime()
{
	appSim->setSyncTime(!appSim->getSyncTime());
	if (appSim->getSyncTime())
		appSim->synchronizeTime();
}

/*
static gint menuVertexShaders(GtkWidget*, gpointer)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowNightSideMaps, on);
    appCore->charEntered('\026'); //Ctrl+V
    return TRUE;
}
*/

static gint menuShowLocTime(GtkWidget*, gpointer)
{
    if (appCore->getTimeZoneBias() == 0)
    {
        appCore->setTimeZoneBias(-timezone + 3600 * daylight);
        appCore->setTimeZoneName(tzname[daylight]);
		prefs->showLocalTime = TRUE;
    }
    else
    {
        appCore->setTimeZoneBias(0);
        appCore->setTimeZoneName("UTC");
		prefs->showLocalTime = FALSE;
    }

	#ifdef GNOME
	gconf_client_set_bool(client, "/apps/celestia/showLocalTime", prefs->showLocalTime, NULL);
	#endif
	
    return TRUE;
}


// Function to return "active" flag of any toggle widget we use
static bool getActiveState(GtkWidget* w)
{
	if (GTK_IS_TOGGLE_BUTTON(w))
		return GTK_TOGGLE_BUTTON(w)->active;
	else if (GTK_IS_CHECK_MENU_ITEM(w))
		return GTK_CHECK_MENU_ITEM(w)->active;

	return FALSE;
}

#ifdef GNOME
void setFlag(int type, const char* name, int value) {
	// type: 0=render, 1=orbit, 2=label
	gchar key[60];
	switch (type) {
		case 0: sprintf(key, "%s%s", "/apps/celestia/render/", name); break;
		case 1: sprintf(key, "%s%s", "/apps/celestia/orbits/", name); break;
		case 2: sprintf(key, "%s%s", "/apps/celestia/labels/", name); break;
	}
	
	gconf_client_set_bool(client, key, value, NULL);
}
#endif

// Render Checkbox control superfunction!
// Renderer::Show* passed as (gpointer)
static gint menuRenderer(GtkWidget* w, gpointer flag)
{
	bool state = getActiveState(w);

    appRenderer->setRenderFlags((appRenderer->getRenderFlags() & ~(int)flag) |
                             (state ? (int)flag : 0));

	prefs->renderFlags = appRenderer->getRenderFlags();

	#ifdef GNOME
	// Update GConf
	switch ((int)flag) {
		case Renderer::ShowStars: setFlag(0, "stars", state); break;
		case Renderer::ShowPlanets: setFlag(0, "planets", state); break;
		case Renderer::ShowGalaxies: setFlag(0, "galaxies", state); break;
		case Renderer::ShowDiagrams: setFlag(0, "diagrams", state); break;
		case Renderer::ShowCloudMaps: setFlag(0, "cloudMaps", state); break;
		case Renderer::ShowOrbits: setFlag(0, "orbits", state); break;
		case Renderer::ShowCelestialSphere: setFlag(0, "celestialSphere", state); break;
		case Renderer::ShowNightMaps: setFlag(0, "nightMaps", state); break;
		case Renderer::ShowAtmospheres: setFlag(0, "atmospheres", state); break;
		case Renderer::ShowSmoothLines: setFlag(0, "smoothLines", state); break;
		case Renderer::ShowEclipseShadows: setFlag(0, "eclipseShadows", state); break;
		case Renderer::ShowStarsAsPoints: setFlag(0, "starsAsPoints", state); break;
		case Renderer::ShowRingShadows: setFlag(0, "ringShadows", state); break;
		case Renderer::ShowBoundaries: setFlag(0, "boundaries", state); break;
		case Renderer::ShowAutoMag: setFlag(0, "autoMag", state); break;
		case Renderer::ShowCometTails: setFlag(0, "cometTails", state); break;
		case Renderer::ShowMarkers: setFlag(0, "markers", state); break;
	}
	#endif

	return TRUE;
}

// Label Checkbox control superfunction!
// Renderer::*Labels passed as (gpointer)
static gint menuLabeler(GtkWidget* w, gpointer flag)
{
	bool state = getActiveState(w);

    appRenderer->setLabelMode((appRenderer->getLabelMode() & ~(int)flag) |
                           (state ? (int)flag : 0));

	#ifdef GNOME
	// Update GConf
	switch ((int)flag) {
		case Renderer::StarLabels: setFlag(2, "star", state); break;
		case Renderer::PlanetLabels: setFlag(2, "planet", state); break;
		case Renderer::MoonLabels: setFlag(2, "moon", state); break;
		case Renderer::ConstellationLabels: setFlag(2, "constellation", state); break;
		case Renderer::GalaxyLabels: setFlag(2, "galaxy", state); break;
		case Renderer::AsteroidLabels: setFlag(2, "asteroid", state); break;
		case Renderer::SpacecraftLabels: setFlag(2, "spacecraft", state); break;
		case Renderer::LocationLabels: setFlag(2, "location", state); break;
		case Renderer::CometLabels: setFlag(2, "comet", state); break;
	}
	#endif

	return TRUE;
}

// Orbit Checkbox control superfunction!
// Renderer::*Orbits passed as (gpointer)
static gint menuOrbiter(GtkWidget* w, gpointer flag)
{
	bool state = getActiveState(w);

    appRenderer->setOrbitMask((appRenderer->getOrbitMask() & ~(int)flag) |
                           (state ? (int)flag : 0));

	#ifdef GNOME
	// Update GConf
	switch ((int)flag) {
		case Body::Planet: setFlag(1, "planet", state); break;
		case Body::Moon: setFlag(1, "moon", state); break;
		case Body::Asteroid: setFlag(1, "asteroid", state); break;
		case Body::Spacecraft: setFlag(1, "spacecraft", state); break;
		case Body::Comet: setFlag(1, "comet", state); break;
	}
	#endif

	return TRUE;
}

// Star Style radio group control superfunction!
static gint menuStarStyle(GtkWidget*, gpointer flag)
{
	// Set up the desired style
	switch ((int)flag)
	{
		case Renderer::FuzzyPointStars:
			prefs->starStyle = Renderer::FuzzyPointStars;
			break;
		case Renderer::PointStars:
			prefs->starStyle = Renderer::PointStars;
			break;
		case Renderer::ScaledDiscStars:
			prefs->starStyle = Renderer::ScaledDiscStars;
	}

    appRenderer->setStarStyle(prefs->starStyle);

	#ifdef GNOME
	gconf_client_set_int(client, "/apps/celestia/starStyle", prefs->starStyle, NULL);
	#endif
	
	return TRUE;
}

static void menuMoreStars()
{
    appCore->charEntered(']');

	prefs->visualMagnitude = appSim->getFaintestVisible();

	#ifdef GNOME
	gconf_client_set_float(client, "/apps/celestia/visualMagnitude", prefs->visualMagnitude, NULL);
	#endif
}

static void menuLessStars()
{
    appCore->charEntered('[');

	prefs->visualMagnitude = appSim->getFaintestVisible();

	#ifdef GNOME
	gconf_client_set_float(client, "/apps/celestia/visualMagnitude", prefs->visualMagnitude, NULL);
	#endif
}

static void menuShowInfo()
{
    appCore->charEntered('V');

	prefs->hudDetail = appCore->getHudDetail();
	
	#ifdef GNOME
	gconf_client_set_int(client, "/apps/celestia/hudDetail", prefs->hudDetail, NULL);
	#endif
}

static void menuRunDemo()
{
    appCore->charEntered('D');
}

// CALLBACK: Mark the selected object.
static void menuMark()
{
	Simulation* sim = appCore->getSimulation();
	if (sim->getUniverse() != NULL)
	{
		sim->getUniverse()->markObject(sim->getSelection(),
						10.0f,
						Color(0.0f, 1.0f, 0.0f, 0.9f),
						Marker::Diamond,
						1);
	}
}

// CALLBACK: Unmark the selected object.
static void menuUnMark()
{
	Simulation* sim = appCore->getSimulation();
	if (sim->getUniverse() != NULL)
		sim->getUniverse()->unmarkObject(sim->getSelection(), 1);
}

// CALLBACK: Toggle fullscreen mode
static void menuFullScreen()
{
	if (prefs->fullScreen == FALSE) {
		gtk_window_fullscreen(GTK_WINDOW(mainWindow));
		prefs->fullScreen = TRUE;
	}
	else {
		gtk_window_unfullscreen(GTK_WINDOW(mainWindow));
		prefs->fullScreen = FALSE;
	}

	#ifdef GNOME
	gconf_client_set_bool(client, "/apps/celestia/fullScreen", prefs->fullScreen, NULL);
	#endif
}

static void menuAbout()
{
    const gchar* authors[] = {
        "Chris Laurel <claurel@shatters.net>",
        "Deon Ramsey <dramsey@sourceforge.net>",
        "Clint Weisbrod <cweisbrod@adelphia.net>",
		"Fridger Schrempp <fridger.schrempp@desy.de>",
		"Pat Suwalski <pat@suwalski.net>",
        NULL
    };

	GtkWidget* about = NULL;

	#ifdef GNOME
    about = gnome_about_new("Celestia",
	                        VERSION,
	                        "(c) 2001-2004 Chris Laurel",
	                        "3D Space Simulation",
	                        authors,
	                        NULL,
	                        NULL,
	                        NULL);
	#else
	// Join the author array into a single string.
	int i = 0;
	string auth = "Celestia, (c) 2001-2004 Chris Laurel\n\n";
	while (authors[i] != NULL) {
		if (i != 0) auth += ",\n";
		auth += authors[i];
		i++;
	}
	
	about = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
	                               GTK_DIALOG_DESTROY_WITH_PARENT,
	                               GTK_MESSAGE_INFO,
	                               GTK_BUTTONS_CLOSE,
	                               (gchar *)auth.c_str());
	#endif
	if (about == NULL)
	    return;

	#ifdef GNOME
	g_signal_connect(G_OBJECT(about), "destroy", G_CALLBACK(gtk_widget_destroyed), &about);
	gtk_window_present(GTK_WINDOW(about));
	#else
	gtk_dialog_run (GTK_DIALOG(about));
	gtk_widget_destroy (about);
	#endif
}

static int glResX = 0;
static const int resolutions[] = { 640, 800, 1024, 1152, 1280 };

// CALLBACK: Set Resolution Value.
void viewerSizeSelect(GtkWidget* w, gpointer)
{
	GtkMenu* menu = GTK_MENU(w);
	GtkWidget* item = gtk_menu_get_active(GTK_MENU(menu));

	GList* list = gtk_container_children(GTK_CONTAINER(menu));
    int itemIndex = g_list_index(list, item);

	if (itemIndex == 0)
	{
		// No resolution change
		glResX = 0;
	}
	else
	{
		// Get resolution according to resolutions[]
		itemIndex = itemIndex - 2;
		glResX = resolutions[itemIndex];
	}		
}

void menuViewerSize()
{
	int currentX, currentY, winX, winY;
	glResX = 0;

    GtkWidget* dialog = gtk_dialog_new_with_buttons("Set Viewer Size...",
	                                                GTK_WINDOW(mainWindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_STOCK_CANCEL,
													GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OK,
	                                                GTK_RESPONSE_OK,
                                                    NULL);
    if (dialog == NULL)
        return;

    GtkWidget* vbox = gtk_vbox_new(FALSE, CELSPACING); 
	gtk_container_set_border_width(GTK_CONTAINER(vbox), CELSPACING);

    GtkWidget* label = gtk_label_new("Dimensions for Main Window:");
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

    GtkWidget* menubox = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(vbox), menubox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox, TRUE, TRUE, 0);

    GtkWidget* menu = gtk_menu_new();

	char res[15];

	if (prefs->fullScreen) {
		sprintf(res, "Fullscreen");
		gtk_widget_set_sensitive(menubox, FALSE);
	}
	else {
		sprintf(res, "Current: %d x %d", oglArea->allocation.width, oglArea->allocation.height);
	}

	GtkWidget *item = gtk_menu_item_new_with_label(res);
    gtk_menu_append(GTK_MENU(menu), item);
	gtk_menu_append(GTK_MENU(menu), GTK_WIDGET(gtk_separator_menu_item_new()));

    for (int i = 0; i < int(sizeof(resolutions) / sizeof(i)); i++)
    {
		sprintf(res, "%d x %d", resolutions[i], int(0.75 * resolutions[i]));
	
        item = gtk_menu_item_new_with_label(res);
        gtk_menu_append(GTK_MENU(menu), item);
        gtk_widget_show(item);
    }

    g_signal_connect(GTK_OBJECT(menu),
                     "selection-done",
                     G_CALLBACK(viewerSizeSelect),
                     NULL);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(menubox), menu);

	gtk_widget_show_all(vbox);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gint whichButton = gtk_dialog_run(GTK_DIALOG(dialog));

	if (whichButton == GTK_RESPONSE_OK) {
		// Set resolution accordingly
		if (glResX > 0) {
			currentX = oglArea->allocation.width;
			currentY = oglArea->allocation.height;
			gtk_window_get_size(GTK_WINDOW(mainWindow), &winX, &winY);

			gtk_window_resize(GTK_WINDOW(mainWindow), glResX + winX - currentX, int(0.75 * glResX) + winY - currentY);
		}
	}
	
	gtk_widget_destroy(GTK_WIDGET(dialog));
}


void handleOpenScript(GtkWidget*, gpointer fileSelector)
{
	const gchar *filename;
	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileSelector));

	// Modified From Win32 HandleOpenScript
	if (filename)
	{
		// If you got here, a path and file has been specified.
		// filename contains full path to specified file
		ContentType type = DetermineFileType(filename);

		if (type == Content_CelestiaScript)
		{
			appCore->runScript(filename);
		}
		else if (type == Content_CelestiaLegacyScript)
		{
			ifstream scriptfile(filename);
			if (!scriptfile.good())
			{
        		GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
		                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
						        						   GTK_MESSAGE_ERROR,
								        				   GTK_BUTTONS_OK,
		                                                   "Error opening script file.");
				gtk_dialog_run(GTK_DIALOG(errBox));
				gtk_widget_destroy(errBox);
			}
			else
			{
				CommandParser parser(scriptfile);
				CommandSequence* script = parser.parse();
				if (script == NULL)
				{
					const vector<string>* errors = parser.getErrors();
					const char* errorMsg = "";
					if (errors->size() > 0)
						errorMsg = (*errors)[0].c_str();
        			GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
		    	                                               GTK_DIALOG_DESTROY_WITH_PARENT,
							        						   GTK_MESSAGE_ERROR,
									        				   GTK_BUTTONS_OK,
		   		                                               errorMsg);
					gtk_dialog_run(GTK_DIALOG(errBox));
					gtk_widget_destroy(errBox);
				}
				else
				{
					appCore->cancelScript(); // cancel any running script
					appCore->runScript(script);
				}
			}
		}
		else
		{
			GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_ERROR,
							GTK_BUTTONS_OK,
							"Bad File Type. Use *.(cel|celx|clx).");
			gtk_dialog_run(GTK_DIALOG(errBox));
			gtk_widget_destroy(errBox);
		}
	}
}


void menuOpenScript()
{
	GtkWidget* fileSelector = gtk_file_selection_new("Open Script.");
	
    g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->ok_button),
                       "clicked",
                       G_CALLBACK(handleOpenScript),
                       (gpointer)fileSelector);
    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->ok_button),
                              "clicked",
                              G_CALLBACK(gtk_widget_destroy),
                              (gpointer)fileSelector);
    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->cancel_button),
                              "clicked",
                              G_CALLBACK(gtk_widget_destroy),
                              (gpointer)fileSelector);
    gtk_file_selection_show_fileop_buttons(GTK_FILE_SELECTION(fileSelector));
    gtk_widget_show(fileSelector);
}


static string* captureFilename = NULL;

void storeCaptureFilename(GtkWidget*, gpointer fileSelector)
{
    const gchar *tmp;
    tmp=gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileSelector));
    if(!tmp || !(*tmp))  // Don't change the captureFilename and exit if empty
        return;
    *captureFilename = tmp; // remember it for next time

    // Get the dimensions of the current viewport
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    bool success = false;
    ContentType type = DetermineFileType(*captureFilename);
    if (type == Content_Unknown)
    {
        GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
		                                           GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_OK,
		                                           "Unable to determine image file type from name, please use a name ending in '.jpg' or '.png'.");
		gtk_dialog_run(GTK_DIALOG(errBox));
		gtk_widget_destroy(errBox);
        return;
    }
    else if (type == Content_JPEG)
    {
        success = CaptureGLBufferToJPEG(*captureFilename,
                                        viewport[0], viewport[1],
                                        viewport[2], viewport[3]);
    }
    else if (type == Content_PNG)
    {
        success = CaptureGLBufferToPNG(string(*captureFilename),
                                       viewport[0], viewport[1],
                                       viewport[2], viewport[3]);
    }
    else
    {
        GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
		                                           GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_OK,
		                                           "Sorry, currently screen capturing to JPEG or PNG files only is supported.");
		gtk_dialog_run(GTK_DIALOG(errBox));
		gtk_widget_destroy(errBox);
        return;
    }

    if (!success)
    {
        GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(mainWindow),
		                                           GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_OK,
		                                           "Error writing captured image.");
		gtk_dialog_run(GTK_DIALOG(errBox));
		gtk_widget_destroy(errBox);
    }
}



static void menuCaptureImage()
{
	GtkWidget* fileSelector = gtk_file_selection_new("Select capture file.");

    g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->ok_button),
                       "clicked",
                       G_CALLBACK(storeCaptureFilename),
                       (gpointer)fileSelector);
    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->ok_button),
                              "clicked",
                              G_CALLBACK(gtk_widget_destroy),
                              (gpointer)fileSelector);
    g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->cancel_button),
                              "clicked",
                              G_CALLBACK(gtk_widget_destroy),
                              (gpointer)fileSelector);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileSelector), captureFilename->c_str());
    gtk_file_selection_show_fileop_buttons(GTK_FILE_SELECTION(fileSelector));
    gtk_widget_show(fileSelector);
}


static const char * const infoLabels[]=
{
    "None",
    "Terse",
    "Verbose",
    NULL
};

static GtkToggleButton *ambientGads[4]; // Store the Gadgets here for later mods
static GtkToggleButton *infoGads[3];

void makeRadioItems(const char* const *labels, GtkWidget *box, GtkSignalFunc sigFunc, GtkToggleButton **gads, gpointer data)
{
    GSList *group = NULL;
    for(int i=0; labels[i]; i++)
    {
        GtkWidget *button = gtk_radio_button_new_with_label(group, labels[i]);
        if(gads)
            gads[i] = GTK_TOGGLE_BUTTON(button);
        if (button == NULL)
        {
            DPRINTF(0, "Unable to get GTK Elements.\n");
           // return;
        }
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (i == 0)?1:0);
        gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
        gtk_widget_show (button);
        g_signal_connect(GTK_OBJECT(button), "pressed", sigFunc, (gpointer)i);

		if (data != NULL)
			g_object_set_data(G_OBJECT(button), "data", data);
    }
}


/*
static gint changeLMag(GtkSpinButton *spinner, gpointer)
{
    float tmp = gtk_spin_button_get_value_as_float(spinner);

    if(tmp > 0.001f && tmp < 15.5f)
        appCore->setFaintest(tmp);
		appCore->getRenderer()->setDistanceLimit(tmp*1000);
    return TRUE;
}
*/

static const int DistanceSliderRange = 10000;
static const float MaxDistanceLimit = 1.0e6f;
GtkWidget* maglabel = NULL;

static float makeDistanceLimit(float value)
{
	float logDistanceLimit = value / DistanceSliderRange;
	return (float) pow(MaxDistanceLimit, logDistanceLimit);
}	

static gint changeDistanceLimit(GtkRange *slider, gpointer)
{
	float limit = makeDistanceLimit(gtk_range_get_value(slider));
	appCore->getRenderer()->setDistanceLimit(limit);

	char labeltext[10] = "100000 ly";
	sprintf(labeltext, "%d ly", (int)limit);
	gtk_label_set_text(GTK_LABEL(maglabel), labeltext);

	return TRUE;
}

static const char * const ambientLabels[]=
{
    "None",
    "Low",
    "Medium",
    NULL
};

static float amLevels[] =
{
    0.0,
    0.1,
    0.25
};

static int ambientChanged(GtkWidget*, int lev)
{
    if (lev>=0 && lev<3)
    {
        appCore->getRenderer()->setAmbientLightLevel(amLevels[lev]);

		prefs->ambientLight = amLevels[lev];

		#ifdef GNOME
		gconf_client_set_float(client, "/apps/celestia/ambientLight", prefs->ambientLight, NULL);
		#endif
		
        return TRUE;
    }
    return FALSE;
}


static int infoChanged(GtkButton*, int info)
{
	prefs->hudDetail = info;

	appCore->setHudDetail(prefs->hudDetail);
	
	#ifdef GNOME
	gconf_client_set_int(client, "/apps/celestia/hudDetail", prefs->hudDetail, NULL);
	#endif
	
    return TRUE;
}


extern void resyncAll();

GtkWidget *showFrame=NULL, *labelFrame=NULL, *orbitFrame=NULL, *showBox=NULL, *labelBox=NULL, *orbitBox=NULL;
GtkWidget *optionDialog=NULL, *slider=NULL;

static void menuOptions()
{
	if (optionDialog != NULL)
	{
		gtk_dialog_run(GTK_DIALOG(optionDialog));
		gtk_widget_hide(optionDialog);
		return;
	}
    optionDialog = gtk_dialog_new_with_buttons("View Options",
                                               GTK_WINDOW(mainWindow),
	                                           GTK_DIALOG_DESTROY_WITH_PARENT,
	                                           GTK_STOCK_OK,
	                                           GTK_RESPONSE_OK,
	                                           NULL);
    GtkWidget *hbox = gtk_hbox_new(FALSE, CELSPACING);
    GtkWidget *midBox = gtk_vbox_new(FALSE, CELSPACING);
    GtkWidget *miscBox = gtk_vbox_new(FALSE, CELSPACING);
    GtkWidget *limitFrame = gtk_frame_new("Filter Stars");
    GtkWidget *ambientFrame = gtk_frame_new("Ambient Light");
    GtkWidget *ambientBox = gtk_vbox_new(FALSE, 0);
    GtkWidget *infoFrame = gtk_frame_new("Info Text");
    GtkWidget *infoBox = gtk_vbox_new(FALSE, 0);
    GtkWidget *limitBox = gtk_vbox_new(FALSE, 0);
    if ((optionDialog == NULL) || (hbox == NULL) || (midBox==NULL) ||
        (limitFrame == NULL) || (limitBox == NULL) || (ambientFrame == NULL) ||
        (infoFrame == NULL) || (miscBox == NULL) || (ambientBox == NULL) ||
        (infoBox == NULL))
    {
        optionDialog=NULL;
        DPRINTF(0,"Unable to get some Elements of the Options Dialog!\n");
        return;
    }
    gtk_container_border_width(GTK_CONTAINER(limitBox), CELSPACING);
    gtk_container_border_width(GTK_CONTAINER(ambientBox), CELSPACING);
    gtk_container_border_width(GTK_CONTAINER(infoBox), CELSPACING);
    gtk_container_border_width(GTK_CONTAINER(limitFrame), 0);
    gtk_container_border_width(GTK_CONTAINER(ambientFrame), 0);
    gtk_container_border_width(GTK_CONTAINER(infoFrame), 0);
    gtk_container_add(GTK_CONTAINER(limitFrame),GTK_WIDGET(limitBox));
    gtk_container_add(GTK_CONTAINER(ambientFrame),GTK_WIDGET(ambientBox));
    gtk_container_add(GTK_CONTAINER(infoFrame),GTK_WIDGET(infoBox));

    gtk_box_pack_start(GTK_BOX(hbox), showFrame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(midBox), labelFrame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(midBox), limitFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(miscBox), orbitFrame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(miscBox), ambientFrame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(miscBox), infoFrame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), midBox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), miscBox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG (optionDialog)->vbox), hbox, TRUE,
                       TRUE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);

	float logDistanceLimit = log(appCore->getRenderer()->getDistanceLimit()) / log((float)MaxDistanceLimit);
    GtkAdjustment *adj= (GtkAdjustment *)
							gtk_adjustment_new((gfloat)(logDistanceLimit * DistanceSliderRange),
                                               0.0, DistanceSliderRange, 1.0, 2.0, 0.0);

	maglabel = gtk_label_new(NULL);
	slider = gtk_hscale_new(adj);
	gtk_scale_set_draw_value(GTK_SCALE(slider), 0);
	gtk_box_pack_start(GTK_BOX(limitBox), slider, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(limitBox), maglabel, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(slider), "value-changed", G_CALLBACK(changeDistanceLimit), NULL);
	changeDistanceLimit(GTK_RANGE(GTK_HSCALE(slider)), NULL);

    makeRadioItems(ambientLabels,ambientBox,GTK_SIGNAL_FUNC(ambientChanged),ambientGads, NULL);
    makeRadioItems(infoLabels,infoBox,GTK_SIGNAL_FUNC(infoChanged),infoGads, NULL);

	gtk_widget_show_all(hbox);

    resyncAll();

	gtk_dialog_set_default_response(GTK_DIALOG(optionDialog), GTK_RESPONSE_OK);
	gtk_dialog_run(GTK_DIALOG(optionDialog));
	gtk_widget_hide(optionDialog);
}


static void menuSelectObject()
{
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Select Object",
	                                                GTK_WINDOW(mainWindow),
	                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_OK,
													GTK_RESPONSE_OK,
                                                    GTK_STOCK_CANCEL,
													GTK_RESPONSE_CANCEL,
                                                    NULL);

    if (dialog == NULL)
        return;

	GtkWidget* box = gtk_hbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(box), CELSPACING);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), box, TRUE, TRUE, 0);

    GtkWidget* label = gtk_label_new("Object name");
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    GtkWidget* entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(box));

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    gint whichButton = gtk_dialog_run(GTK_DIALOG(dialog));
    if (whichButton == GTK_RESPONSE_OK)
    {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (name != NULL)
        {
            Selection sel = appCore->getSimulation()->findObject(name);
            if (!sel.empty())
                appCore->getSimulation()->setSelection(sel);
        }
    }

	gtk_widget_destroy(GTK_WIDGET(dialog));
}


static const char * const unitLabels[] =
{
	"km",
	"radii",
	"au",
	NULL
};


typedef struct _gotoObjectData gotoObjectData;

struct _gotoObjectData {
    GtkWidget* dialog;
    GtkWidget* nameEntry;
    GtkWidget* latEntry;
    GtkWidget* longEntry;
    GtkWidget* distEntry;

	int units;
};


static bool GetEntryFloat(GtkWidget* w, float& f)
{
    GtkEntry* entry = GTK_ENTRY(w);
    bool tmp;
    if (entry == NULL)
        return false;

    gchar* text = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    f = 0.0;
    if (text == NULL)
        return false;

    tmp=sscanf(text, " %f", &f) == 1;
    g_free(text);
    return tmp;
}


//static gint GotoObject(GtkWidget*, gpointer data)
static void GotoObject(gotoObjectData* gotoObjectDlg)
{
    const gchar* objectName = gtk_entry_get_text(GTK_ENTRY(gotoObjectDlg->nameEntry));
    if (objectName != NULL)
    {
        Selection sel = appSim->findObjectFromPath(objectName);
        if (!sel.empty())
        {
            appSim->setSelection(sel);
            appSim->follow();

            float distance = (float) (sel.radius() * 5.0f);
            if (GetEntryFloat(gotoObjectDlg->distEntry, distance))
            {
				// Adjust for km (0), radii (1), au (2)
				if (gotoObjectDlg->units == 2)
					distance = astro::AUtoKilometers(distance);
				else if (gotoObjectDlg->units == 1)
					distance = distance * (float) sel.radius();

                distance += (float) sel.radius();
            }
            distance = astro::kilometersToLightYears(distance);

            float longitude, latitude;
            if (GetEntryFloat(gotoObjectDlg->latEntry, latitude) &&
                GetEntryFloat(gotoObjectDlg->longEntry, longitude))
            {
                appSim->gotoSelectionLongLat(5.0,
                                          distance,
                                          degToRad(longitude),
                                          degToRad(latitude),
                                          Vec3f(0, 1, 0));
            }
            else
            {
                appSim->gotoSelection(5.0,
                                   distance,
                                   Vec3f(0, 1, 0),
                                   astro::ObserverLocal);
            }
        }
    }
}


// CALLBACK: for km|radii|au in Goto Object dialog
int changeGotoUnits(GtkButton* w, int selection)
{
	gotoObjectData* data = (gotoObjectData *)g_object_get_data(G_OBJECT(w), "data");

	data->units = selection;
	
	return TRUE;
}


// CALLBACK: Navigation->Goto Object
void menuGotoObject()
{
	gotoObjectData *data = g_new0(gotoObjectData, 1);

    data->dialog = gtk_dialog_new_with_buttons("Goto Object",
	                                           GTK_WINDOW(mainWindow),
											   GTK_DIALOG_DESTROY_WITH_PARENT,
											   "Go To",
											   GTK_RESPONSE_OK,
											   GTK_STOCK_CANCEL,
											   GTK_RESPONSE_CANCEL,
											   NULL);
    data->nameEntry = gtk_entry_new();
    data->latEntry = gtk_entry_new();
    data->longEntry = gtk_entry_new();
    data->distEntry = gtk_entry_new();
    
    if (data->dialog == NULL ||
        data->nameEntry == NULL ||
        data->latEntry == NULL ||
        data->longEntry == NULL ||
        data->distEntry == NULL)
    {
        // Potential memory leak here . . .
        return;
    }

	// Set up the values (from windows cpp file)
	double distance, longitude, latitude;
	appSim->getSelectionLongLat(distance, longitude, latitude);

	//Display information in format appropriate for object
	if (appSim->getSelection().body() != NULL)
	{
		char temp[20];
		distance = distance - (double) appSim->getSelection().body()->getRadius();
		sprintf(temp, "%.1f", (float)distance);
		gtk_entry_set_text(GTK_ENTRY(data->distEntry), temp);
		sprintf(temp, "%.5f", (float)longitude);
		gtk_entry_set_text(GTK_ENTRY(data->longEntry), temp);
		sprintf(temp, "%.5f", (float)latitude);
		gtk_entry_set_text(GTK_ENTRY(data->latEntry), temp);
		gtk_entry_set_text(GTK_ENTRY(data->nameEntry), (char*) appSim->getSelection().body()->getName().c_str());
	}

	GtkWidget* vbox = gtk_vbox_new(TRUE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), CELSPACING);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(data->dialog)->vbox), vbox, TRUE, TRUE, 0);

	GtkWidget* align = NULL;
    GtkWidget* hbox = NULL;
    GtkWidget* label = NULL;

    // Object name label and entry
	align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Object name:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->nameEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

    // Latitude and longitude
	align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Latitude:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->latEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

	align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Longitude:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->longEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

    // Distance
	align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Distance:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->distEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

	// Distance Options
	data->units = 0;
    hbox = gtk_hbox_new(FALSE, CELSPACING);
	makeRadioItems(unitLabels, hbox, GTK_SIGNAL_FUNC(changeGotoUnits), NULL, data);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	
	gtk_widget_show_all(vbox);

	if (gtk_dialog_run(GTK_DIALOG(data->dialog)) == GTK_RESPONSE_OK)
		GotoObject(data);
	
	gtk_widget_destroy(data->dialog);
}


static Destination* selectedDest = NULL;
static gint TourGuideSelect(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowGalaxies, on);
    // appCore->charEntered('U');
    GtkMenu* menu = GTK_MENU(w);
    if (menu == NULL)
        return FALSE;

    GtkWidget* item = gtk_menu_get_active(GTK_MENU(menu));
    if (item == NULL)
        return FALSE;

    GList* list = gtk_container_children(GTK_CONTAINER(menu));
    int itemIndex = g_list_index(list, item);

    const DestinationList* destinations = appCore->getDestinations();
    if (destinations != NULL &&
        itemIndex >= 0 && itemIndex < (int) destinations->size())
    {
        selectedDest = (*destinations)[itemIndex];
    }

    GtkLabel* descLabel = GTK_LABEL(data);
    if (descLabel != NULL && selectedDest != NULL)
    {
        gtk_label_set_text(descLabel, selectedDest->description.c_str());
    }

    return TRUE;
}

static gint TourGuideGoto(GtkWidget*, gpointer)
{
    if (selectedDest != NULL && appSim != NULL)
    {
        Selection sel = appSim->findObjectFromPath(selectedDest->target);
        if (!sel.empty())
        {
            appSim->follow();
            appSim->setSelection(sel);
            if (selectedDest->distance <= 0)
            {
                // Use the default distance
                appSim->gotoSelection(5.0,
                                   Vec3f(0, 1, 0),
                                   astro::ObserverLocal);
            }
            else
            {
                appSim->gotoSelection(5.0,
                                   selectedDest->distance,
                                   Vec3f(0, 1, 0),
                                   astro::ObserverLocal);
            }
        }
    }

    return TRUE;
}


static void menuTourGuide()
{
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Tour Guide...",
	                                                GTK_WINDOW(mainWindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_OK,
	                                                GTK_RESPONSE_OK,
                                                    NULL);
    if (dialog == NULL)
        return;

    GtkWidget* hbox = gtk_hbox_new(FALSE, CELSPACING); 
	gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);

    GtkWidget* label = gtk_label_new("Select your destination:");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

    GtkWidget* menubox = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(hbox), menubox, TRUE, TRUE, 0);

    GtkWidget* gotoButton = gtk_button_new_with_label("Go To");
    gtk_box_pack_start(GTK_BOX(hbox), gotoButton, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);

    gtk_widget_show(hbox);

    GtkWidget* descLabel = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(descLabel), TRUE);
    gtk_label_set_justify(GTK_LABEL(descLabel), GTK_JUSTIFY_FILL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), descLabel, TRUE, TRUE, 0);

    GtkWidget* menu = gtk_menu_new();
    const DestinationList* destinations = appCore->getDestinations();
    if (destinations != NULL)
    {
        for (DestinationList::const_iterator iter = destinations->begin();
             iter != destinations->end(); iter++)
        {
            Destination* dest = *iter;
            if (dest != NULL)
            {
                GtkWidget* item = gtk_menu_item_new_with_label(dest->name.c_str());
                gtk_menu_append(GTK_MENU(menu), item);
                gtk_widget_show(item);
            }
        }
    }

    g_signal_connect(GTK_OBJECT(menu),
                     "selection-done",
                     G_CALLBACK(TourGuideSelect),
                     GTK_OBJECT(descLabel));
    g_signal_connect(GTK_OBJECT(gotoButton),
                     "pressed",
                     G_CALLBACK(TourGuideGoto),
                     NULL);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(menubox), menu);

    gtk_widget_set_usize(dialog, 440, 300);
    gtk_widget_show(label);
    gtk_widget_show(menubox);
    gtk_widget_show(descLabel);
    gtk_widget_show(gotoButton);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
}


char *readFromFile(const char *fname)
{
    ifstream textFile(fname, ios::in);
    string s("");
    if (!textFile.is_open())
    {
	s="Unable to open file '";
	s+= fname ;
	s+= "', probably due to improper installation !\n";
    }
    char c;
    while(textFile.get(c))
    {
	if (c=='\t')
	    s+="        "; // 8 spaces
	else if (c=='\014') // Ctrl+L (form feed)
	    s+="\n\n\n\n";
	else
	    s+=c;
    }
    return g_strdup(s.c_str());
}


static void textInfoDialog(GtkWidget** dialog, const char *txt, const char *title)
{
    /* I would use a gnome_message_box dialog for this, except they don't seem
       to notice that the texts are so big that they create huge windows, and
       would work better with a scrolled window. Deon */
	*dialog = gtk_dialog_new_with_buttons(title,
	                                      GTK_WINDOW(mainWindow),
										  GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      GTK_STOCK_OK,
										  GTK_RESPONSE_OK,
	                                      NULL);

	if (*dialog == NULL)
	{
	    DPRINTF(0, "Unable to open '%s' dialog!\n",title);
		return;
	}

	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG (*dialog)->vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scrolled_window);

	GtkWidget *text = gtk_label_new (txt);
	gtk_widget_modify_font(text, pango_font_description_from_string("mono"));
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (text));
	gtk_widget_show (text);

	gtk_window_set_default_size(GTK_WINDOW(*dialog), 500, 400); //Absolute Size, urghhh

	gtk_dialog_run(GTK_DIALOG(*dialog));
	gtk_widget_destroy(*dialog);
}


static GtkWidget* controldialog=NULL;

static void menuControls()
{
    char *txt=readFromFile("controls.txt");
    textInfoDialog(&controldialog,txt,"Mouse and Keyboard Controls");
    g_free(txt);
}


static GtkWidget* licdialog=NULL;

static void menuLicense()
{
    char *txt=readFromFile("COPYING");
    textInfoDialog(&licdialog,txt,"Celestia License");
    g_free(txt);
}

static GtkWidget* ogldialog=NULL;

static void menuOpenGL()
{
    // Code grabbed from winmain.gtk
    char* vendor = (char*) glGetString(GL_VENDOR);
    char* render = (char*) glGetString(GL_RENDERER);
    char* version = (char*) glGetString(GL_VERSION);
    char* ext = (char*) glGetString(GL_EXTENSIONS);
    string s;
    s = "Vendor : ";
    if (vendor != NULL)
	s += vendor;
    s += "\n";

    s += "Renderer : ";
    if (render != NULL)
	s += render;
    s += "\n";

    s += "Version : ";
    if (version != NULL)
	s += version;
    s += "\n";

    char buf[100];
    GLint simTextures = 1;
    if (ExtensionSupported("GL_ARB_multitexture"))
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &simTextures);
    sprintf(buf, "Max simultaneous textures: %d\n",
	    simTextures);
    s += buf;

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    sprintf(buf, "Max texture size: %d\n\n",
	    maxTextureSize);
    s += buf;

    s += "Supported Extensions:\n    ";
    if (ext != NULL)
    {
	string extString(ext);
	unsigned int pos = extString.find(' ', 0);
	while (pos != string::npos)
	{
	    extString.replace(pos, 1, "\n    ");
	    pos = extString.find(' ', pos+5);
	}
	s += extString;
    }

    textInfoDialog(&ogldialog,s.c_str(),"Open GL Info");
}



static const char * const sstitles[]=
{
    "Name",
    "Type"
};

static const char * const cstitles[]=
{
    "Name",
    "Distance(LY)",
    "App. Mag",
    "Abs. Mag",
    "Type"
};

static const char * const starBrowserLabels[]=
{
    "Nearest",
	"Brightest (App.)",
    "Brightest (Abs.)",
    "With Planets",
    NULL
};

static const char *tmp[5];

// Solar Browser and Star Browser Widgets/Lists
static GtkWidget *solarTree = NULL;
static GtkTreeStore *solarTreeStore = NULL;
static GtkWidget *starList = NULL;
static GtkListStore *starListStore = NULL;

void addPlanetarySystemToTree(const PlanetarySystem* sys, GtkTreeIter *parent)
{
    for (int i = 0; i < sys->getSystemSize(); i++)
    {
	Body* world = sys->getBody(i);
	tmp[0] = g_strdup(world->getName().c_str());
        switch(world->getClassification())
        {
        case Body::Planet:
            tmp[1]="Planet";
            break;
        case Body::Moon:
            tmp[1]="Moon";
            break;
        case Body::Asteroid:
            tmp[1]="Asteroid";
            break;
        case Body::Comet:
            tmp[1]="Comet";
            break;
        case Body::Spacecraft:
            tmp[1]="Spacecraft";
            break;
        case Body::Unknown:
        default:
            tmp[1]="-";
            break;
        }

	const PlanetarySystem* satellites = world->getSatellites();

	// Add child
	// tmp[0] == Name, tmp[1] == Type, world == pointer to object
	GtkTreeIter child;
	gtk_tree_store_append(solarTreeStore, &child, parent);
	gtk_tree_store_set(solarTreeStore, &child, 0, tmp[0], 1, tmp[1], 2, (gpointer)world, -1);

	// Recurse
	g_free((char*) tmp[0]);
	if (satellites != NULL)
            addPlanetarySystemToTree(satellites, &child);
    }
}


static const Star *nearestStar;

static gint centerBrowsed()
{
    appCore->charEntered('c');
    return TRUE;
}

static gint gotoBrowsed()
{
    appCore->charEntered('G');
    return TRUE;
}


// CALLBACK: When Star is selected in Star Browser
static gint listStarSelect(GtkTreeSelection* sel, gpointer)
{
	GValue value = { 0, 0 }; // Very strange, warning-free declaration!
	GtkTreeIter iter;
	GtkTreeModel* model;

	// IF prevents selection while list is being updated
	if (!gtk_tree_selection_get_selected(sel, &model, &iter))
		return FALSE;

	gtk_tree_model_get_value(model, &iter, 5, &value);
	Star *selStar = (Star *)g_value_get_pointer(&value);

    if (selStar)
    {
		appSim->setSelection(Selection(selStar));
		return TRUE;
    }
    return FALSE;
}

// CALLBACK: When Object is selected in Solar System Browser
static gint treeSolarSelect(GtkTreeSelection* sel, gpointer)
{
    Body *body;
	GValue value = { 0, 0 }; // Very strange, warning-free declaration!
	GtkTreeIter iter;
	GtkTreeModel* model;

	gtk_tree_selection_get_selected(sel, &model, &iter);
	gtk_tree_model_get_value(model, &iter, 2, &value);

    if ((body = (Body *)g_value_get_pointer(&value)))
    {
        if (body == (Body *) nearestStar)
			appSim->setSelection(Selection((Star *) nearestStar));
        else
			appSim->setSelection(Selection(body));
			return TRUE;
    }
    DPRINTF(0, "Unable to find body for this node.\n");
    return FALSE;
}


static gint buttonMake(GtkWidget *hbox, const char *txt, GtkSignalFunc func, gpointer data)
{
    GtkWidget* button = gtk_button_new_with_label(txt);
    if (button == NULL)
    {
	DPRINTF(0, "Unable to get GTK Elements.\n");
	return 1;
    }
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX (hbox),
                       button, TRUE, TRUE, 0);

    g_signal_connect(GTK_OBJECT(button),
                     "pressed",
                     func,
                     data);
    return 0;
}


static StarBrowser sbrowser;
unsigned int currentLength=0;

static void addStars()
{
    StarDatabase* stardb = appSim->getUniverse()->getStarCatalog();
    sbrowser.refresh();
    vector<const Star*> *stars = sbrowser.listStars(numListStars);
    currentLength=(*stars).size();
	appSim->setSelection(Selection((Star *)(*stars)[0]));
    UniversalCoord ucPos = appSim->getObserver().getPosition();

	GtkTreeIter iter;
	gtk_list_store_clear(starListStore);

    for (unsigned int i = 0; i < currentLength; i++)
    {
        char buf[20];
        const Star *star=(*stars)[i];
        tmp[0] = g_strdup((stardb->getStarName(*star)).c_str());

        Point3f pStar = star->getPosition();
        Vec3d v(pStar.x * 1e6 - (float)ucPos.x, 
                pStar.y * 1e6 - (float)ucPos.y, 
                pStar.z * 1e6 - (float)ucPos.z);
        double d = v.length() * 1e-6;
        
        sprintf(buf, " %.3f ", d);
        tmp[1] = g_strdup(buf);

        Vec3f r = star->getPosition() - ucPos;
        sprintf(buf, " %.2f ", astro::absToAppMag(star->getAbsoluteMagnitude(), d));
        tmp[2] = g_strdup(buf);

        sprintf(buf, " %.2f ", star->getAbsoluteMagnitude());
        tmp[3] = g_strdup(buf);

        star->getStellarClass().str(buf, sizeof buf);
        tmp[4] = g_strdup(buf);

		gtk_list_store_append(starListStore, &iter);
		gtk_list_store_set(starListStore, &iter,
		                   0, tmp[0],
						   1, tmp[1],
						   2, tmp[2],
						   3, tmp[3],
						   4, tmp[4],
						   5, (gpointer)star, -1);

        for (unsigned int j = 0; j < 5; j++)
            g_free((void*) tmp[j]);
	}

    delete stars;
}


static int radioClicked(GtkButton*, int pred)
{
    if (!sbrowser.setPredicate(pred))
		return FALSE;
    addStars();
    return TRUE;
}


static int refreshBrowser()
{
    addStars();
    return TRUE;
}


static void loadNearestStarSystem()
{
    const SolarSystem* solarSys = appSim->getNearestSolarSystem();
    StarDatabase *stardb = appSim->getUniverse()->getStarCatalog();
    g_assert(stardb);

	GtkTreeIter top;
	gtk_tree_store_clear(solarTreeStore);
	gtk_tree_store_append(solarTreeStore, &top, NULL);

    if (solarSys != NULL)
    {
		nearestStar = solarSys->getStar();

		tmp[0] = const_cast<char*>(stardb->getStarName(*nearestStar).c_str());

		char buf[30];
		sprintf(buf, "%s Star", (nearestStar->getStellarClass().str().c_str()));
		tmp[1] = buf;
	
		// Set up the top-level node
		gtk_tree_store_set(solarTreeStore, &top, 0, tmp[0], 1, tmp[1], 2, (gpointer)nearestStar, -1);

		const PlanetarySystem* planets = solarSys->getPlanets();
		if (planets != NULL)
		    addPlanetarySystemToTree(planets, &top);

		// Open up the top node
		GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(solarTreeStore), &top);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(solarTree), path, FALSE);
	}
	else
		gtk_tree_store_set(solarTreeStore, &top, 0, "No Planetary Bodies", -1);
}


// MENU: Navigation -> Solar System Browser...
static void menuSolarBrowser()
{
    GtkWidget *browser = gtk_dialog_new_with_buttons("Solar System Browser",
	                                                GTK_WINDOW(mainWindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
				                                    GTK_STOCK_OK,
													GTK_RESPONSE_OK,
				                                    NULL);
	gtk_window_set_modal(GTK_WINDOW(browser), FALSE);
	appSim->setSelection(Selection((Star *) NULL));
 
    // Solar System Browser
	GtkWidget *mainbox = gtk_vbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(browser)->vbox), mainbox, TRUE, TRUE, 0);
	
    GtkWidget *scrolled_win = gtk_scrolled_window_new (NULL, NULL);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(mainbox), scrolled_win, TRUE, TRUE, 0);

	// Set the tree store to have 2 visible cols, one hidden
	// The hidden one stores pointer to the row's object
	solarTreeStore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	solarTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(solarTreeStore));

	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(solarTree), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_win), solarTree);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	for (int i=0; i<2; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (sstitles[i], renderer, "text", i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(solarTree), column);
		gtk_tree_view_column_set_min_width(column, 200);
	}
																							 
	loadNearestStarSystem();

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(solarTree));
	g_signal_connect(selection, "changed", G_CALLBACK(treeSolarSelect), NULL);

    // Common Buttons
    GtkWidget *hbox = gtk_hbox_new(TRUE, CELSPACING);
    if (buttonMake(hbox, "Center", (GtkSignalFunc)centerBrowsed, NULL))
        return;
    if (buttonMake(hbox, "Go To", (GtkSignalFunc)gotoBrowsed, NULL))
        return;
    gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

    gtk_widget_set_usize(browser, 500, 400); // Absolute Size, urghhh
	gtk_widget_show_all(browser);

	gtk_dialog_run(GTK_DIALOG(browser));
	gtk_widget_destroy(browser);
	solarTree = NULL;
}

static gboolean listStarEntryChange(GtkEntry *entry, GdkEventFocus *event, gpointer scale)
{
	// If not called by the slider, but rather by user
	// Prevents infinite recursion
	if (event != NULL)
	{
		numListStars = atoi(gtk_entry_get_text(entry));
		
		// Sanity Check
		if (numListStars < MinListStars)
		{
			numListStars = MinListStars;
			// Call self to set text (NULL event = no recursion)
			listStarEntryChange(entry, NULL, NULL);
		}
		if (numListStars > MaxListStars)
		{
			numListStars = MaxListStars;
			// Call self to set text
			listStarEntryChange(entry, NULL, NULL);
		}

		gtk_range_set_value(GTK_RANGE(scale), (gdouble)numListStars);
		return FALSE;
	}

	// Update value of this box
	char stars[3];
	sprintf(stars, "%d", numListStars);
	gtk_entry_set_text(entry, stars);
	return TRUE;
}

static void listStarSliderChange(GtkRange *range, gpointer entry)
{
	// Update the value of the text entry box
	numListStars = (int)gtk_range_get_value(GTK_RANGE(range));
	listStarEntryChange(GTK_ENTRY(entry), NULL, NULL);

	// Refresh the browser listbox
	refreshBrowser();
}

// MENU: Navigation -> Star Browser...
static void menuStarBrowser()
{
    GtkWidget *browser = gtk_dialog_new_with_buttons("Star System Browser",
	                                                GTK_WINDOW(mainWindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
				                                    GTK_STOCK_OK,
													GTK_RESPONSE_OK,
				                                    NULL);
	gtk_window_set_modal(GTK_WINDOW(browser), FALSE);
	appSim->setSelection(Selection((Star *) NULL));
 
    // Star System Browser
	GtkWidget *mainbox = gtk_vbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(browser)->vbox), mainbox, TRUE, TRUE, 0);
	
    GtkWidget *scrolled_win = gtk_scrolled_window_new (NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_win),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(mainbox), scrolled_win, TRUE, TRUE, 0);

	// Create listbox list
	starListStore = gtk_list_store_new(6,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_POINTER);
	starList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(starListStore));

	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(starList), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_win), starList);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	// Add the columns
	for (int i=0; i<5; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (cstitles[i], renderer, "text", i, NULL);
		if (i > 0 && i < 4) {
			// Right align
			gtk_tree_view_column_set_alignment(column, 1.0);
			g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
		}
		gtk_tree_view_append_column(GTK_TREE_VIEW(starList), column);
	}

	// Initialize the star browser
	sbrowser.setSimulation(appSim);

	// Set up callback for when a star is selected
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(starList));
	g_signal_connect(selection, "changed", G_CALLBACK(listStarSelect), NULL);

	// From now on, it's the bottom-of-the-window controls
	GtkWidget *frame = gtk_frame_new("Star Search Criteria");
    gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);
	
    GtkWidget *hbox = gtk_hbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

	// List viewing preference settings
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *hbox2 = gtk_hbox_new(FALSE, CELSPACING);
	GtkWidget *label = gtk_label_new("Maximum Stars Displayed in List");
    gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, FALSE, 0);
	GtkWidget *entry = gtk_entry_new_with_max_length(3);
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_box_pack_start(GTK_BOX(hbox2), entry, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);
	GtkWidget *scale = gtk_hscale_new_with_range(MinListStars, MaxListStars, 1);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
	g_signal_connect(scale, "value-changed", G_CALLBACK(listStarSliderChange), (gpointer)entry);
	g_signal_connect(entry, "focus-out-event", G_CALLBACK(listStarEntryChange), (gpointer)scale);
    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, FALSE, 0);

	// Set initial Star Value
	gtk_range_set_value(GTK_RANGE(scale), numListStars);
	if (numListStars == MinListStars)
	{
		// Force update manually (scale won't trigger event)
		listStarEntryChange(GTK_ENTRY(entry), NULL, NULL);
		refreshBrowser();
	}

	// Radio Buttons
	vbox = gtk_vbox_new(TRUE, 0);
    makeRadioItems(starBrowserLabels, vbox, GTK_SIGNAL_FUNC(radioClicked), NULL, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    // Common Buttons
    hbox = gtk_hbox_new(TRUE, CELSPACING);
    if (buttonMake(hbox, "Center", (GtkSignalFunc)centerBrowsed, NULL))
        return;
    if (buttonMake(hbox, "Go To", (GtkSignalFunc)gotoBrowsed, NULL))
        return;
    if (buttonMake(hbox, "Refresh", (GtkSignalFunc)refreshBrowser, NULL))
        return;
    gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

	gtk_widget_show_all(mainbox);

    gtk_widget_set_usize(browser, 500, 400); // Absolute Size, urghhh

	gtk_dialog_run(GTK_DIALOG(browser));
	gtk_widget_destroy(browser);

	appSim->setSelection(Selection((Star *) NULL));
}

static gint intAdjChanged(GtkAdjustment* adj, int *val)
{
    if (val)
    {
	*val=(int)adj->value;
	return TRUE;
    }
    return FALSE;
}


const char * timeOptions[]=
{
    "UTC",
    NULL,
    NULL
};

const char * const monthOptions[]=
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
    NULL
};

static int tzone;
static int *monthLoc=NULL;

static gint zonechosen(GtkOptionMenu *menu)
{
	tzone = gtk_option_menu_get_history(menu) + 1;
    return TRUE;
}

static gint monthchosen(GtkOptionMenu *menu)
{
    if (monthLoc)
		*monthLoc = gtk_option_menu_get_history(menu) + 1;
	return TRUE;
}


static void chooseOption(GtkWidget *hbox, const char *str, char *choices[], int *val, gpointer, GtkSignalFunc chosen)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *label = gtk_label_new(str);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    GtkWidget *optmenu = gtk_option_menu_new ();
    GtkWidget *menu = gtk_menu_new ();
	for(unsigned int i=0; choices[i]; i++)
	{
		GtkWidget *menu_item = gtk_menu_item_new_with_label (choices[i]);
		gtk_menu_append (GTK_MENU (menu), menu_item);
		gtk_widget_show (menu_item);
	}
    gtk_option_menu_set_menu (GTK_OPTION_MENU (optmenu), menu);
    gtk_option_menu_set_history (GTK_OPTION_MENU (optmenu), (*val-1));
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), optmenu, FALSE, TRUE, 7);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 2);
    gtk_widget_show (label);
    gtk_widget_show (optmenu);
    gtk_widget_show (vbox);

	g_signal_connect(GTK_OBJECT(optmenu), "changed", G_CALLBACK(chosen), NULL);
}


static void intSpin(GtkWidget *hbox, const char *str, int min, int max, int *val, const char *sep)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *label = gtk_label_new(str);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    GtkAdjustment *adj = (GtkAdjustment *) gtk_adjustment_new ((float)*val, (float) min, (float) max,
							      1.0, 5.0, 0.0);
    GtkWidget *spinner = gtk_spin_button_new (adj, 1.0, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (spinner), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON (spinner),TRUE);
    gtk_entry_set_max_length(GTK_ENTRY (spinner), ((max<99)?2:4) );

    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
    if ((sep) && (*sep))
    {
	gtk_widget_show (label);
	GtkWidget *hbox2 = gtk_hbox_new(FALSE, 3);
	label = gtk_label_new(sep);
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox2), spinner, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 7);
	gtk_widget_show (label);
	gtk_widget_show (hbox2);
    }
    else
    {
	gtk_box_pack_start (GTK_BOX (vbox), spinner, TRUE, TRUE, 7);
    }
    gtk_widget_show (label);
    gtk_widget_show (spinner);
    gtk_widget_show (vbox);
    g_signal_connect(GTK_OBJECT(adj), "value-changed",
		       G_CALLBACK(intAdjChanged), val);
}


static void menuSetTime()
{
    int second;
    GtkWidget *stimedialog = gtk_dialog_new_with_buttons("Set Time",
	                                                     GTK_WINDOW(mainWindow),
														 GTK_DIALOG_DESTROY_WITH_PARENT,
														 GTK_STOCK_OK,
														 GTK_RESPONSE_OK,
														 "Set Current Time",
														 GTK_RESPONSE_ACCEPT,
														 GTK_STOCK_CANCEL,
														 GTK_RESPONSE_CANCEL,
														 NULL);
    tzone = 1;
    if (appCore->getTimeZoneBias())
		tzone = 2;
	
    if (stimedialog == NULL)
    {
		DPRINTF(0, "Unable to open 'Set Time' dialog!\n");
		return;
    }

    GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
    GtkWidget *frame = gtk_frame_new("Time");
    GtkWidget *align=gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    
    if ((hbox == NULL) || (frame == NULL) || (align == NULL))
    {
		DPRINTF(0, "Unable to get GTK Elements.\n");
		return;
    }
    astro::Date date(appSim->getTime() +
                     astro::secondsToJulianDate(appCore->getTimeZoneBias()));
    monthLoc = &date.month;
    gtk_widget_show(align);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(align),GTK_WIDGET(hbox));
    gtk_container_add(GTK_CONTAINER(frame),GTK_WIDGET(align));
    gtk_container_set_border_width (GTK_CONTAINER (frame), 7);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(stimedialog)->vbox), frame, FALSE, FALSE, 0);
    intSpin(hbox, "Hour", 0, 23, &date.hour, ":");
    intSpin(hbox, "Minute", 0, 59, &date.minute, ":");
    second=(int)date.seconds;
    intSpin(hbox, "Second", 0, 59, &second, "  ");
    chooseOption(hbox, "Timezone", (char **)timeOptions, &tzone, NULL, GTK_SIGNAL_FUNC(zonechosen));
    gtk_widget_show_all(hbox);
    hbox = gtk_hbox_new(FALSE, 6);
    frame = gtk_frame_new("Date");
    align=gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 7);
    
    if ((hbox == NULL) || (frame == NULL) || (align == NULL))
    {
		DPRINTF(0, "Unable to get GTK Elements.\n");
		return;
    }
    chooseOption(hbox,"Month", (char **)monthOptions, &date.month, NULL, GTK_SIGNAL_FUNC(monthchosen));
    intSpin(hbox, "Day", 1, 31, &date.day, ",");
    intSpin(hbox,"Year", -9999, 9999, &date.year, " "); /* (Hopefully,
			    noone will need to go beyond these :-) */
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(stimedialog)->vbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(align),GTK_WIDGET(hbox));
    gtk_container_add(GTK_CONTAINER(frame),GTK_WIDGET(align));
    gtk_widget_show(align);
    gtk_widget_show(frame);
    gtk_widget_show_all(hbox);

	gtk_dialog_set_default_response(GTK_DIALOG(stimedialog), GTK_RESPONSE_OK);
    gint button = gtk_dialog_run(GTK_DIALOG(stimedialog));
    
	monthLoc = NULL;
    if (button == GTK_RESPONSE_ACCEPT)  // Set current time and exit.
    {
		time_t curtime = time(NULL);
		appSim->setTime((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
		appSim->update(0.0);
    }
    else if (button == GTK_RESPONSE_OK)  // Set entered time and exit
    {
		appSim->setTime((double) date - ((tzone==1) ? 0 : astro::secondsToJulianDate(appCore->getTimeZoneBias())));
		appSim->update(0.0);
    }

	gtk_widget_destroy(stimedialog);
}

typedef struct _selDate selDate;

struct _selDate {
	int year;
	int month;
	int day;
};

//selDate *d1, *d2;

typedef struct _EclipseData EclipseData;

struct _EclipseData {
	// Start Time
	selDate* d1;

	// End Time
	selDate* d2;

	bool bSolar;
	char body[7];
	GtkTreeSelection* sel;

	GtkWidget *eclipseList;
	GtkListStore *eclipseListStore;

	GtkDialog* window;
};

static void setButtonDateString(GtkToggleButton *button, int year, int month, int day)
{
	char date[50];
	sprintf(date, "%d %s %d", day, monthOptions[month - 1], year);

	gtk_button_set_label(GTK_BUTTON(button), date);
}

static void calDateSelect(GtkCalendar *calendar, GtkToggleButton *button)
{
	// Set the selected date
	guint year, month, day; 
	gtk_calendar_get_date(calendar, &year, &month, &day);

//	g_object_set_data(G_OBJECT(button), "year", (gpointer)year);
//	g_object_set_data(G_OBJECT(button), "month", (gpointer)(month + 1));
//	g_object_set_data(G_OBJECT(button), "day", (gpointer)day);

	selDate* date = (selDate *)g_object_get_data(G_OBJECT(button), "eclipsedata");
	date->year = year;
	date->month = month + 1;
	date->day = day;

	// Update the button
	setButtonDateString(button, year, month + 1, day);

	// Close the calendar window
	gtk_toggle_button_set_active(button, !gtk_toggle_button_get_active(button));
}

static void showCalPopup(GtkToggleButton *button, EclipseData *ed) //GtkWindow *parentwindow)
{
	GtkWidget* calwindow = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "calendar"));

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
	{
		// Pushed in
		if (!calwindow)
		{
			calwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

			// Temporary: should be a transient, but then there are focus issues
			gtk_window_set_modal(GTK_WINDOW(calwindow), TRUE);
			gtk_window_set_type_hint(GTK_WINDOW(calwindow), GDK_WINDOW_TYPE_HINT_DOCK);
			gtk_window_set_decorated(GTK_WINDOW(calwindow), FALSE);
			gtk_window_set_resizable(GTK_WINDOW(calwindow), FALSE);
			gtk_window_stick(GTK_WINDOW(calwindow));

			GtkWidget* calendar = gtk_calendar_new();

//			int year = (int)g_object_get_data(G_OBJECT(button),  "year");
//			int month = (int)g_object_get_data(G_OBJECT(button),  "month");
//			int day = (int)g_object_get_data(G_OBJECT(button),  "day");
//			gtk_calendar_select_month(GTK_CALENDAR(calendar), month - 1, year);
//			gtk_calendar_select_day(GTK_CALENDAR(calendar), day);
		
			selDate* date = (selDate *)g_object_get_data(G_OBJECT(button), "eclipsedata");
		
			gtk_calendar_select_month(GTK_CALENDAR(calendar), date->month - 1, date->year);
			gtk_calendar_select_day(GTK_CALENDAR(calendar), date->day);
	
			gtk_container_add(GTK_CONTAINER(calwindow), calendar);
			gtk_widget_show(calendar);

			int x, y, i, j;
			gdk_window_get_origin(GDK_WINDOW(GTK_WIDGET(button)->window), &x, &y);
			gtk_widget_translate_coordinates(GTK_WIDGET(button), GTK_WIDGET(ed->window), 10, 10, &i, &j);

			gtk_window_move(GTK_WINDOW(calwindow), x + i, y + j);

			g_signal_connect(calendar, "day-selected-double-click", G_CALLBACK(calDateSelect), button);

			gtk_window_present(GTK_WINDOW(calwindow));

			g_object_set_data_full(
							G_OBJECT (button), "calendar",
							calwindow, (GDestroyNotify)gtk_widget_destroy);
		}
	}
	else
	{
		// Pushed out
		if (calwindow)
		{
			/* Destroys the calendar */
			g_object_set_data(G_OBJECT (button), "calendar", NULL);
			calwindow = NULL;
		}
	}
}

static const char * const eclipsetitles[] =
{
	"Planet",
	"Satellite",
	"Date",
	"Start",
	"End",
	NULL
};

static const char * const eclipsetypetitles[] =
{
	"solar",
	"moon",
	NULL
};

static const char * const eclipseplanettitles[] =
{
	"Earth",
	"Jupiter",
	"Saturn",
	"Uranus",
	"Neptune",
	"Pluto",
	NULL
};


// CALLBACK: "SetTime/Goto" in Eclipse Finder
static gint eclipseGoto(GtkButton*, EclipseData* ed)
{
	GValue value = { 0, 0 }; // Very strange, warning-free declaration!
	GtkTreeIter iter;
	GtkTreeModel* model;
	int time[6];

	// Nothing selected
	if (ed->sel == NULL)
		return FALSE;

	// IF prevents selection while list is being updated
	if (!gtk_tree_selection_get_selected(ed->sel, &model, &iter))
		return FALSE;

	// Tedious method of extracting the desired time
	// However, still better than parsing a string.
	for (int i=0; i<6; i++)
	{
		gtk_tree_model_get_value(model, &iter, i+5, &value);
		time[i] = g_value_get_int(&value);
		g_value_unset(&value);
	}	

	// Retrieve the selected body
	gtk_tree_model_get_value(model, &iter, 11, &value);
	Body* body  = (Body *)g_value_get_pointer(&value);
	g_value_unset(&value);
			
	Simulation* sim = appCore->getSimulation();

	// Set time based on retrieved values
	astro::Date d(time[0], time[1], time[2]);
	d.hour = time[3];
	d.minute = time[4];
	d.seconds = (double)time[5];
	appCore->getSimulation()->setTime((double)d);

	// The rest is directly from the Windows eclipse code
	Selection target(body);
	Selection ref(body->getSystem()->getStar());
	// Use the phase lock coordinate system to set a position
	// on the line between the sun and the body where the eclipse
	// is occurring.
	sim->setFrame(FrameOfReference(astro::PhaseLock, target, ref));
	sim->update(0.0);

	double distance = astro::kilometersToMicroLightYears(target.radius() * 4.0);
	RigidTransform to;
	to.rotation = Quatd::yrotation(PI);
	to.translation = Point3d(0, 0, -distance);
	sim->gotoLocation(to, 2.5);

	return TRUE;
}

// CALLBACK: Double-click on the Eclipse Finder Listbox
static gint eclipse2Click(GtkWidget*, GdkEventButton* event, EclipseData* ed)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		// Double-click, same as hitting the select and go button
		return eclipseGoto(NULL, ed);
	}

	return FALSE;
}

// CALLBACK: Compute button in Eclipse Finder
static gint eclipseCompute(GtkButton* button, EclipseData* ed)
{
	GtkTreeIter iter;

	// Set the cursor to a watch and force redraw
	gdk_window_set_cursor(GTK_WIDGET(button)->window, gdk_cursor_new(GDK_WATCH));
	gtk_main_iteration();

	// Clear the listbox
	gtk_list_store_clear(ed->eclipseListStore);

	// Create the dates in a more suitable format
	astro::Date from(ed->d1->year, ed->d1->month, ed->d1->day);
	astro::Date to(ed->d2->year, ed->d2->month, ed->d2->day);

	// Initialize the eclipse finder
	EclipseFinder ef(appCore, ed->body, ed->bSolar ? Eclipse::Solar : Eclipse::Moon, (double)from, (double)to);
	vector<Eclipse> eclipseListRaw = ef.getEclipses();

	for (std::vector<Eclipse>::iterator i = eclipseListRaw.begin();
		i != eclipseListRaw.end();
		i++) {

		// Handle 0 case
		if ((*i).planete == "None") {
			gtk_list_store_append(ed->eclipseListStore, &iter);
			gtk_list_store_set(ed->eclipseListStore, &iter, 0, (*i).planete.c_str(), -1);
			continue;
		}

		char d[12], strStart[10], strEnd[10];
		astro::Date start((*i).startTime);
		astro::Date end((*i).endTime);

		sprintf(d, "%d-%02d-%02d", (*i).date->year, (*i).date->month, (*i).date->day);
		sprintf(strStart, "%02d:%02d:%02d", start.hour, start.minute, (int)start.seconds);
		sprintf(strEnd, "%02d:%02d:%02d", end.hour, end.minute, (int)end.seconds);

		// Set time to middle time so that eclipse it right on earth
		astro::Date timeToSet = (start + end) / 2.0f;

		// Add item
		// Entries 5->11 are not displayed and store data
		gtk_list_store_append(ed->eclipseListStore, &iter);
		gtk_list_store_set(ed->eclipseListStore, &iter,
						   0, (*i).planete.c_str(),
						   1, (*i).sattelite.c_str(),
						   2, d,
						   3, strStart,
						   4, strEnd,
		                   5, timeToSet.year,
						   6, timeToSet.month,
						   7, timeToSet.day,
						   8, timeToSet.hour,
						   9, timeToSet.minute,
						   10, (int)timeToSet.seconds,
						   11, (*i).body,
						   -1);
	}

	// Set the cursor back
	gdk_window_set_cursor(GTK_WIDGET(button)->window, gdk_cursor_new(GDK_LEFT_PTR));
	return TRUE;
}

// CALLBACK: When Eclipse Body is selected
void eclipseBodySelect(GtkMenuShell* menu, EclipseData* ed)
{
	GtkWidget* item = gtk_menu_get_active(GTK_MENU(menu));

	GList* list = gtk_container_children(GTK_CONTAINER(menu));
	int itemIndex = g_list_index(list, item);

	// Set string according to body array
	sprintf(ed->body, "%s", eclipseplanettitles[itemIndex]);
}

// CALLBACK: When Eclipse Type (Solar:Moon) is selected
void eclipseTypeSelect(GtkMenuShell* menu, EclipseData* ed)
{
	GtkWidget* item = gtk_menu_get_active(GTK_MENU(menu));

	GList* list = gtk_container_children(GTK_CONTAINER(menu));
	int itemIndex = g_list_index(list, item);

	// Solar eclipse
	if (itemIndex == 0)
		ed->bSolar = 1;
	// Moon eclipse
	else
		ed->bSolar = 0;
}

// CALLBACK: When Eclipse is selected in Eclipse Finder
static gint listEclipseSelect(GtkTreeSelection* sel, EclipseData* ed)
{
	// Simply set the selection pointer to this data item
	ed->sel = sel;

	return TRUE;
}


// MENU: Navigation -> Eclipse Finder
static void menuEclipseFinder()
{
	EclipseData* ed = g_new0(EclipseData, 1);
	selDate* d1 = g_new0(selDate, 1);
	selDate* d2 = g_new0(selDate, 1);
	ed->d1 = d1;
	ed->d2 = d2;
	ed->eclipseList = NULL;
	ed->eclipseListStore = NULL;
	ed->bSolar = TRUE;
	sprintf(ed->body, "%s", eclipseplanettitles[0]);
	ed->sel = NULL;

    ed->window = GTK_DIALOG(gtk_dialog_new_with_buttons("Eclipse Finder",
	                                                GTK_WINDOW(mainWindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
				                                    GTK_STOCK_OK,
													GTK_RESPONSE_OK,
				                                    NULL));
	gtk_window_set_modal(GTK_WINDOW(ed->window), FALSE);
 
	GtkWidget *mainbox = gtk_vbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ed->window)->vbox), mainbox, TRUE, TRUE, 0);
	
    GtkWidget *scrolled_win = gtk_scrolled_window_new (NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_win),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(mainbox), scrolled_win, TRUE, TRUE, 0);

	// Create listbox list
	// Six invisible ints at the end to hold actual time
	// This will save string parsing like in KDE version
	// Last field holds pointer to selected Body
	ed->eclipseListStore = gtk_list_store_new(12,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
									   G_TYPE_INT,
									   G_TYPE_INT,
									   G_TYPE_INT,
									   G_TYPE_INT,
									   G_TYPE_INT,
									   G_TYPE_INT,
	                                   G_TYPE_POINTER);
	ed->eclipseList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ed->eclipseListStore));

	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(ed->eclipseList), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_win), ed->eclipseList);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	// Add the columns
	for (int i=0; i<5; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (eclipsetitles[i], renderer, "text", i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(ed->eclipseList), column);
	}

	// Set up callback for when an eclipse is selected
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ed->eclipseList));
	g_signal_connect(selection, "changed", G_CALLBACK(listEclipseSelect), ed);

	// From now on, it's the bottom-of-the-window controls
	GtkWidget *label;
    GtkWidget *hbox;

	// --------------------------------
	hbox = gtk_hbox_new(FALSE, CELSPACING);
	
	label = gtk_label_new("Find");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
    GtkWidget* menuTypeBox = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(hbox), menuTypeBox, FALSE, FALSE, 0);

	label = gtk_label_new("eclipse on");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
    GtkWidget* menuBodyBox = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(hbox), menuBodyBox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	// --------------------------------
	hbox = gtk_hbox_new(FALSE, CELSPACING);

	label = gtk_label_new("From");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	// Get current date
	astro::Date datenow(appCore->getSimulation()->getTime());

	// Set current time
	ed->d1->year = datenow.year - 1;
	ed->d1->month = datenow.month;
	ed->d1->day = datenow.day;

	// Set time a year from now
	ed->d2->year = ed->d1->year + 2;
	ed->d2->month = ed->d1->month;
	ed->d2->day = ed->d1->day;

	GtkWidget* date1Button = gtk_toggle_button_new();
	setButtonDateString(GTK_TOGGLE_BUTTON(date1Button), ed->d1->year, ed->d1->month, ed->d1->day);
	g_object_set_data(G_OBJECT(date1Button), "eclipsedata", ed->d1);
	gtk_box_pack_start(GTK_BOX(hbox), date1Button, FALSE, FALSE, 0);

	label = gtk_label_new("to");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	GtkWidget* date2Button = gtk_toggle_button_new();
	setButtonDateString(GTK_TOGGLE_BUTTON(date2Button), ed->d2->year, ed->d2->month, ed->d2->day);
	g_object_set_data(G_OBJECT(date2Button), "eclipsedata", ed->d2);
	gtk_box_pack_start(GTK_BOX(hbox), date2Button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	// --------------------------------

    // Common Buttons
    hbox = gtk_hbox_new(TRUE, CELSPACING);
    if (buttonMake(hbox, "Compute", (GtkSignalFunc)eclipseCompute, ed))
        return;
    if (buttonMake(hbox, "Set Date and Go to Planet", (GtkSignalFunc)eclipseGoto, ed))
        return;
    gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

	// Set up the drop-down boxes
	GtkWidget *item;

    GtkWidget* menuType = gtk_menu_new();
    for (int i = 0; eclipsetypetitles[i] != NULL; i++)
    {
        item = gtk_menu_item_new_with_label(eclipsetypetitles[i]);
        gtk_menu_append(GTK_MENU(menuType), item);
		gtk_widget_show(item);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(menuTypeBox), menuType);

    GtkWidget* menuBody = gtk_menu_new();
    for (int i = 0; eclipseplanettitles[i] != NULL; i++)
    {
        item = gtk_menu_item_new_with_label(eclipseplanettitles[i]);
        gtk_menu_append(GTK_MENU(menuBody), item);
		gtk_widget_show(item);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(menuBodyBox), menuBody);

	// Hook up all the signals
	g_signal_connect(GTK_OBJECT(menuType), "selection-done", G_CALLBACK(eclipseTypeSelect), ed);
    g_signal_connect(GTK_OBJECT(menuBody), "selection-done", G_CALLBACK(eclipseBodySelect), ed);

	// Double-click handler
	g_signal_connect(GTK_OBJECT(ed->eclipseList), "button-press-event", G_CALLBACK(eclipse2Click), ed);

	g_signal_connect(GTK_OBJECT(date1Button), "toggled", G_CALLBACK(showCalPopup), ed);
	g_signal_connect(GTK_OBJECT(date2Button), "toggled", G_CALLBACK(showCalPopup), ed);

    gtk_widget_set_usize(GTK_WIDGET(ed->window), 400, 400); // Absolute Size, urghhh
	gtk_widget_show_all(mainbox);

	gtk_dialog_run(GTK_DIALOG(ed->window));
	gtk_widget_destroy(GTK_WIDGET(ed->window));
}


static GtkItemFactoryEntry menuItems[] =
{
    { (gchar *)"/_File",                               NULL,                    NULL,             0,                (gchar *)"<Branch>",     NULL },
    { (gchar *)"/File/_Open Script...",                NULL,                    menuOpenScript  , 0,                (gchar *)"<StockItem>",  GTK_STOCK_OPEN },
    { (gchar *)"/File/_Capture Image...",              (gchar *)"F10",          menuCaptureImage, 0,                (gchar *)"<StockItem>",  GTK_STOCK_SAVE_AS },
    { (gchar *)"/File/-",                              NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/File/_Quit",                          (gchar *)"<control>Q",   gtk_main_quit,    0,                (gchar *)"<StockItem>",  GTK_STOCK_QUIT },
    { (gchar *)"/_Navigation",                         NULL,                    NULL,             0,                (gchar *)"<Branch>",     NULL },
    { (gchar *)"/Navigation/Select _Sol",              (gchar *)"H",            menuSelectSol,    0,                (gchar *)"<StockItem>",  GTK_STOCK_HOME },
    { (gchar *)"/Navigation/Tour G_uide...",           NULL,                    menuTourGuide,    0,                NULL,                    NULL },
    { (gchar *)"/Navigation/Select _Object...",        NULL,                    menuSelectObject, 0,                (gchar *)"<StockItem>",  GTK_STOCK_FIND },
    { (gchar *)"/Navigation/Goto Object...",           NULL,                    menuGotoObject,   0,                NULL,                    NULL },
    { (gchar *)"/Navigation/-",                        NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/Navigation/_Center Selection",        (gchar *)"c",            menuCenter,       0,                NULL,                    NULL },
    { (gchar *)"/Navigation/_Goto Selection",          (gchar *)"G",            menuGoto,         0,                (gchar *)"<StockItem>",  GTK_STOCK_JUMP_TO },
    { (gchar *)"/Navigation/_Follow Selection",        (gchar *)"F",            menuFollow,       0,                NULL,                    NULL },
    { (gchar *)"/Navigation/S_ync Orbit Selection",    (gchar *)"Y",            menuSync,         0,                NULL,                    NULL },
    { (gchar *)"/Navigation/_Track Selection",         (gchar *)"T",            menuTrack,        0,                NULL,                    NULL },
    { (gchar *)"/Navigation/-",                        NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/Navigation/Solar System _Browser...", NULL,                    menuSolarBrowser, 0,                NULL,                    NULL },
    { (gchar *)"/Navigation/Star B_rowser...",         NULL,                    menuStarBrowser,  0,                NULL,                    NULL },
    { (gchar *)"/Navigation/_Eclipse Finder",          NULL,                    menuEclipseFinder,0,                NULL,                    NULL },
    { (gchar *)"/_Time",                               NULL,                    NULL,             0,                (gchar *)"<Branch>",     NULL },
    { (gchar *)"/Time/10x _Faster",                    (gchar *)"L",            menuFaster,       0,                NULL,                    NULL },
    { (gchar *)"/Time/10x _Slower",                    (gchar *)"K",            menuSlower,       0,                NULL,                    NULL },
    { (gchar *)"/Time/Free_ze",                        (gchar *)"space",        menuPause,        0,                NULL,                    NULL },
    { (gchar *)"/Time/_Real Time",                     (gchar *)"backslash",    menuRealTime,     0,                NULL,                    NULL },
    { (gchar *)"/Time/Re_verse Time",                  (gchar *)"J",            menuReverse,      0,                NULL,                    NULL },
    { (gchar *)"/Time/Set _Time...",                   NULL,                    menuSetTime,      0,                NULL,                    NULL },
    { (gchar *)"/Time/-",                              NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/Time/Show _Local Time",               NULL,                    NULL,             Menu_ShowLocTime, (gchar *)"<ToggleItem>", NULL },
    { (gchar *)"/_Render",                             NULL,                    NULL,             0,                (gchar *)"<Branch>",     NULL },
    { (gchar *)"/Render/Set Viewer Size...",           NULL,                    menuViewerSize,   0,                (gchar *)"<StockItem>",  GTK_STOCK_ZOOM_FIT },
    { (gchar *)"/Render/Full Screen",                  (gchar *)"<alt>Return",  NULL,             Menu_FullScreen,  (gchar *)"<ToggleItem>", NULL },
    { (gchar *)"/Render/-",                            NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/Render/View _Options...",             NULL,                    menuOptions,      0,                (gchar *)"<StockItem>",  GTK_STOCK_PREFERENCES },
    { (gchar *)"/Render/Show _Info Text",              (gchar *)"V",            menuShowInfo,     0,                NULL,                    NULL },
    { (gchar *)"/Render/-",                            NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/Render/_More Stars Visible",          (gchar *)"bracketright", menuMoreStars,    0,                NULL,                    NULL },
    { (gchar *)"/Render/_Fewer Stars Visible",         (gchar *)"bracketleft",  menuLessStars,    0,                NULL,                    NULL },
    { (gchar *)"/Render/Auto Magnitude",               (gchar *)"<control>Y",   NULL,             Menu_AutoMag,     (gchar *)"<ToggleItem>", NULL },
    { (gchar *)"/Render/Star St_yle",                  NULL,                    NULL,             0,                (gchar *)"<Branch>",     NULL },
    { (gchar *)"/Render/Star St_yle/_Fuzzy Points",    NULL,                    NULL,             Menu_StarFuzzy,   (gchar *)"<RadioItem>",  NULL },
    { (gchar *)"/Render/Star St_yle/_Points",          NULL,                    NULL,             Menu_StarPoints,  (gchar *)"<RadioItem>",  NULL },
    { (gchar *)"/Render/Star St_yle/Scaled _Discs",    NULL,                    NULL,             Menu_StarDiscs,   (gchar *)"<RadioItem>",  NULL },
    { (gchar *)"/Render/-",                            NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
	{ (gchar *)"/Render/_Ambient Light",               NULL,                    NULL,             0,                (gchar *)"<Branch>",     NULL },
    { (gchar *)"/Render/_Ambient Light/_None",         NULL,                    NULL,             Menu_AmbientNone, (gchar *)"<RadioItem>",  NULL },
    { (gchar *)"/Render/_Ambient Light/_Low",          NULL,                    NULL,             Menu_AmbientLow,  (gchar *)"<RadioItem>",  NULL },
    { (gchar *)"/Render/_Ambient Light/_Medium",       NULL,                    NULL,             Menu_AmbientMed,  (gchar *)"<RadioItem>",  NULL },
    { (gchar *)"/Render/Antialiasing",                 (gchar *)"<control>X",   NULL,             Menu_AntiAlias,   (gchar *)"<ToggleItem>", NULL },
	{ (gchar *)"/_View",                               NULL,                    NULL,             0,                (gchar *)"<Branch>",     NULL },
    { (gchar *)"/View/Split _Horizontally",            (gchar *)"<control>R",   menuViewSplitH,   0,                NULL,                    NULL },
    { (gchar *)"/View/Split _Vertically",              (gchar *)"<control>U",   menuViewSplitV,   0,                NULL,                    NULL },
    { (gchar *)"/View/_Delete Active View",            (gchar *)"delete",       menuViewDelete,   0,                NULL,                    NULL },
    { (gchar *)"/View/_Single View",                   (gchar *)"<control>D",   menuViewSingle,   0,                NULL,                    NULL },
    { (gchar *)"/View/-",                              NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/View/Show _Frames",                   NULL,                    NULL,             Menu_ShowFrames,  (gchar *)"<ToggleItem>", NULL },
    { (gchar *)"/View/Synchronize _Time",              NULL,                    NULL,             Menu_SyncTime,    (gchar *)"<ToggleItem>", NULL },
    { (gchar *)"/_Help",                               NULL,                    NULL,             0,                (gchar *)"<LastBranch>", NULL },
    { (gchar *)"/Help/Run _Demo",                      (gchar *)"D",            menuRunDemo,      0,                (gchar *)"<StockItem>",  GTK_STOCK_EXECUTE },  
    { (gchar *)"/Help/-",                              NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
    { (gchar *)"/Help/_Controls",                      NULL,                    menuControls,     0,                (gchar *)"<StockItem>",  GTK_STOCK_HELP },
    { (gchar *)"/Help/OpenGL _Info",                   NULL,                    menuOpenGL,       0,                NULL,                    NULL },
    { (gchar *)"/Help/_License",                       NULL,                    menuLicense,      0,                NULL,                    NULL },
    { (gchar *)"/Help/-",                              NULL,                    NULL,             0,                (gchar *)"<Separator>",  NULL },
	#ifdef GNOME
    { (gchar *)"/Help/_About",                         NULL,                    menuAbout,        0,                (gchar *)"<StockItem>",  GNOME_STOCK_ABOUT },
	#else
    { (gchar *)"/Help/_About",                         NULL,                    menuAbout,        0,                NULL,                    NULL },
	#endif
};

/*
static GtkItemFactoryEntry optMenuItems[] =
{
//  { (gchar *)"/Render/Use _Vertex Shaders", (gchar *)"<control>V", NULL,    Menu_VertexShaders, (gchar *)"<ToggleItem>", NULL },
};
*/

int checkLocalTime(int, char*)
{
	return(appCore->getTimeZoneBias()!=0);
}

/*
int checkVertexShaders(int, char*)
{
	return(appRenderer->getVertexShaderEnabled());
}
*/

int checkShowGalaxies(int, char*)
{
	return((appRenderer->getRenderFlags() & Renderer::ShowGalaxies) == Renderer::ShowGalaxies);
}

int checkShowFrames(int, char*)
{
	return appCore->getFramesVisible();
}

int checkSyncTime(int, char*)
{
	return appSim->getSyncTime();
}


GtkCheckButton *makeCheckButton(char *label, GtkWidget *vbox, bool set, GtkSignalFunc sigFunc, gpointer data)
{
    g_assert(vbox);
    GtkCheckButton* button = GTK_CHECK_BUTTON(gtk_check_button_new_with_label(label));
    if (button)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), set);
        gtk_widget_show(GTK_WIDGET(button));
        gtk_box_pack_start(GTK_BOX (vbox),
                           GTK_WIDGET(button), FALSE, TRUE, 0);

        g_signal_connect(GTK_OBJECT(button),
                         "toggled",
                         G_CALLBACK(sigFunc),
                         data);
    }
    return button;
}


int checkRenderFlag(int flag, char*)
{
	return ((appRenderer->getRenderFlags() & flag) == flag);
}


int checkLabelFlag(int flag, char*)
{
	return ((appRenderer->getLabelMode() & flag) == flag);
}

int checkOrbitFlag(int flag, char*)
{
	return ((appRenderer->getOrbitMask() & flag) == flag);
}

int radioStarStyle(int flag, char*)
{
	return (appCore->getRenderer()->getStarStyle() == flag);
}

int checkFullScreen(int, char*)
{
	return prefs->fullScreen;
}

int radioAmbientLight(int flag, char*)
{
	return (appCore->getRenderer()->getAmbientLightLevel() == amLevels[flag]);
}


/*
	Reverse alphabetical order!

	Legend for [4] ----
	 0 = Off
	 1 = View Options Only
	 2 = Menu Only
	 3 = Both
*/
static CheckFunc checks[] =
{
    { NULL, NULL, "/Time/Show Local Time",         checkLocalTime,     3, 0, Menu_ShowLocTime, GTK_SIGNAL_FUNC(menuShowLocTime) },
	{ NULL, NULL, "/Render/Full Screen",           checkFullScreen,    2, 0, Menu_FullScreen, GTK_SIGNAL_FUNC(menuFullScreen) },
//  { NULL, NULL, "/Render/Use Vertex Shaders",    checkVertexShaders, 3, 0, Menu_VertexShaders, GTK_SIGNAL_FUNC(menuVertexShaders) },
    { NULL, NULL, "/Render/Antialiasing",          checkRenderFlag,    3, Renderer::ShowSmoothLines, Menu_AntiAlias, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/AutoMag for Stars",     checkRenderFlag,    3, Renderer::ShowAutoMag, Menu_AutoMag, GTK_SIGNAL_FUNC(menuRenderer) },
	{ NULL, NULL, "/Render/Spacecraft",            checkOrbitFlag,     1, Body::Spacecraft, Menu_SpacecraftOrbits, GTK_SIGNAL_FUNC(menuOrbiter) },
	{ NULL, NULL, "/Render/Planets",               checkOrbitFlag,     1, Body::Planet, Menu_PlanetOrbits, GTK_SIGNAL_FUNC(menuOrbiter) },
	{ NULL, NULL, "/Render/Moons",                 checkOrbitFlag,     1, Body::Moon, Menu_MoonOrbits, GTK_SIGNAL_FUNC(menuOrbiter) },
	{ NULL, NULL, "/Render/Comets",                checkOrbitFlag,     1, Body::Comet, Menu_CometOrbits, GTK_SIGNAL_FUNC(menuOrbiter) },
	{ NULL, NULL, "/Render/Asteroids",             checkOrbitFlag,     1, Body::Asteroid, Menu_AsteroidOrbits, GTK_SIGNAL_FUNC(menuOrbiter) },
    { NULL, NULL, "/Render/Stars",                 checkLabelFlag,     1, Renderer::StarLabels, Menu_StarLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Spacecraft",            checkLabelFlag,     1, Renderer::SpacecraftLabels, Menu_CraftLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Planets",               checkLabelFlag,     1, Renderer::PlanetLabels, Menu_PlanetLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Moons",                 checkLabelFlag,     1, Renderer::MoonLabels, Menu_MoonLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Locations",             checkLabelFlag,     1, Renderer::LocationLabels, Menu_LocationLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Galaxies",              checkLabelFlag,     1, Renderer::GalaxyLabels, Menu_GalaxyLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Constellations",        checkLabelFlag,     1, Renderer::ConstellationLabels, Menu_ConstellationLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Comets",                checkLabelFlag,     1, Renderer::CometLabels, Menu_CometLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Asteroids",             checkLabelFlag,     1, Renderer::AsteroidLabels, Menu_AsteroidLabels, GTK_SIGNAL_FUNC(menuLabeler) },
    { NULL, NULL, "/Render/Stars",                 checkRenderFlag,    1, Renderer::ShowStars, Menu_ShowStars, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Ring Shadows",          checkRenderFlag,    1, Renderer::ShowRingShadows, Menu_ShowRingShadows, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Planets",               checkRenderFlag,    1, Renderer::ShowPlanets, Menu_ShowPlanets, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Orbits",                checkRenderFlag,    1, Renderer::ShowOrbits, Menu_ShowOrbits, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Night Side Lights",     checkRenderFlag,    1, Renderer::ShowNightMaps, Menu_ShowNightSideMaps, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Galaxies",              checkRenderFlag,    1, Renderer::ShowGalaxies, Menu_ShowGalaxies, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Eclipse Shadows",       checkRenderFlag,    1, Renderer::ShowEclipseShadows, Menu_ShowEclipseShadows, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Constellations",        checkRenderFlag,    1, Renderer::ShowDiagrams, Menu_ShowConstellations, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Constellation Borders", checkRenderFlag,    1, Renderer::ShowBoundaries, Menu_ShowBoundaries, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Comet Tails",           checkRenderFlag,    1, Renderer::ShowCometTails, Menu_ShowCometTails, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Clouds",                checkRenderFlag,    1, Renderer::ShowCloudMaps, Menu_ShowClouds, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Celestial Grid",        checkRenderFlag,    1, Renderer::ShowCelestialSphere, Menu_ShowCelestialSphere, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/Render/Atmospheres",           checkRenderFlag,    1, Renderer::ShowAtmospheres, Menu_ShowAtmospheres, GTK_SIGNAL_FUNC(menuRenderer) },
    { NULL, NULL, "/View/Synchronize Time",        checkSyncTime,      2, 0, Menu_SyncTime, GTK_SIGNAL_FUNC(menuViewSyncTime) },
    { NULL, NULL, "/View/Show Frames",             checkShowFrames,    2, 0, Menu_ShowFrames, GTK_SIGNAL_FUNC(menuViewShowFrames) },
	{ NULL, NULL, "/Render/Star Style/Fuzzy Points", radioStarStyle,   2, Renderer::FuzzyPointStars, Menu_StarFuzzy, GTK_SIGNAL_FUNC(menuStarStyle) },
	{ NULL, NULL, "/Render/Star Style/Points",       radioStarStyle,   2, Renderer::PointStars, Menu_StarPoints, GTK_SIGNAL_FUNC(menuStarStyle) },
	{ NULL, NULL, "/Render/Star Style/Scaled Discs", radioStarStyle,   2, Renderer::ScaledDiscStars, Menu_StarDiscs, GTK_SIGNAL_FUNC(menuStarStyle) },
	{ NULL, NULL, "/Render/Ambient Light/None",    radioAmbientLight,  2, 0, Menu_AmbientNone, GTK_SIGNAL_FUNC(ambientChanged) },
	{ NULL, NULL, "/Render/Ambient Light/Low",     radioAmbientLight,  2, 1, Menu_AmbientLow, GTK_SIGNAL_FUNC(ambientChanged) },
	{ NULL, NULL, "/Render/Ambient Light/Medium",  radioAmbientLight,  2, 2, Menu_AmbientMed, GTK_SIGNAL_FUNC(ambientChanged) },
};


void setupCheckItem(GtkItemFactory* factory, int action, GtkSignalFunc func, CheckFunc *cfunc) {
	GtkWidget *frame = NULL;

    if (cfunc->active & 2) {
        GtkWidget* w = gtk_item_factory_get_widget_by_action(factory, action);
		
        if (w != NULL) {
			// Add signal handler and pass Renderer::whatever as gpointer data
            g_signal_connect(GTK_OBJECT(w), "toggled",
                             G_CALLBACK(func),
                             (gpointer)cfunc->funcdata);
        }
	}
	else {
		char *optName = rindex(cfunc->path,'/');
		if (optName) {
			if (cfunc->func == checkLabelFlag) frame = labelBox;
			else if (cfunc->func == checkRenderFlag) frame = showBox;
			else if (cfunc->func == checkOrbitFlag) frame = orbitBox;
			else frame = NULL;

			// Add a check button with appropriate callback
			cfunc->optWidget = makeCheckButton(++optName, frame, 0, cfunc->sigFunc, (gpointer)cfunc->funcdata);
		}
	}
}


// Function to create main menu bar
void createMainMenu(GtkWidget* window, GtkWidget** menubar)
{
    gint nItems = sizeof(menuItems) / sizeof(menuItems[0]);

    GtkAccelGroup* accelGroup = gtk_accel_group_new();
    menuItemFactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
                                           "<main>",
                                           accelGroup);
	gtk_window_add_accel_group(GTK_WINDOW (window), accelGroup);
    gtk_item_factory_create_items(menuItemFactory, nItems, menuItems, NULL);
    appRenderer=appCore->getRenderer();
    g_assert(appRenderer);
    appSim = appCore->getSimulation();
    g_assert(appSim);

	/*
    //if (appRenderer->vertexShaderSupported())
	{
	gtk_item_factory_create_item(menuItemFactory, &optMenuItems[0], NULL, 1);
	checks[1].active = 3;
	}
	*/

    if (menubar != NULL)
        *menubar = gtk_item_factory_get_widget(menuItemFactory, "<main>");

    for(int i=sizeof(checks) / sizeof(checks[0])-1; i>=0; --i)
    {
        if(checks[i].active)
            setupCheckItem(menuItemFactory, checks[i].action, checks[i].sigFunc, &checks[i]);
    }
}


// FUNCTION: Append a menu item and return pointer. Used for context menu.
GtkMenuItem* AppendMenu(GtkWidget* parent, GtkSignalFunc callback, const gchar* name, int value) {
	GtkWidget* menuitem;

	// Check for separator
	if (name == NULL) {
		menuitem = gtk_separator_menu_item_new();
	}
	else {
		menuitem = gtk_menu_item_new_with_mnemonic(name);
	}

	// Add handler
	if (callback != NULL) {
		g_signal_connect_swapped (G_OBJECT(menuitem), "activate",
						G_CALLBACK(callback),
						(gpointer)value);
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
	return GTK_MENU_ITEM(menuitem);
}


// CALLBACK: Handle a planetary selection from the context menu.
void handleContextPlanet(int value) {
	// Handle the satellite/child object submenu
	Selection sel = appCore->getSimulation()->getSelection();
	switch (sel.getType())
	{
		case Selection::Type_Star:
			appCore->getSimulation()->selectPlanet(value);
			break;

		case Selection::Type_Body:
			{
				PlanetarySystem* satellites = (PlanetarySystem*) sel.body()->getSatellites();
				appCore->getSimulation()->setSelection(Selection(satellites->getBody(value)));
				break;
			}

		case Selection::Type_DeepSky:
			// Current deep sky object/galaxy implementation does
			// not have children to select.
			break;

		case Selection::Type_Location:
			break;

		default:
			break;
	}
}


// CALLBACK: Handle an alternate surface from the context menu.
void handleContextSurface(int value) {
	// Handle the alternate surface submenu
	Selection sel = appCore->getSimulation()->getSelection();
	if (sel.body() != NULL)
	{
		guint index = value - 1;
		vector<string>* surfNames = sel.body()->getAlternateSurfaceNames();
		if (surfNames != NULL)
		{
			string surfName;
			if (index < surfNames->size())
				surfName = surfNames->at(index);
			appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(surfName);
			delete surfNames;
		}
	}
}

// Typedefs and structs for sorting objects by name in context menu.
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

// FUNCTION: Create planetary submenu for context menu, return menu pointer.
GtkMenu* CreatePlanetarySystemMenu(string parentName, const PlanetarySystem* psys)
{
	// Modified from winmain.cpp

    // Use some vectors to categorize and sort the bodies within this PlanetarySystem.
	// In order to generate sorted menus, we must carry the name and menu index as a
	// single unit in the sort. One obvious way is to create a vector<pair<int,string>>
	// and then use a comparison predicate to sort the vector based on the string in
	// each pair.

	// Declare vector<pair<int,string>> objects for each classification of body
	vector<IntStrPair> asteroids;
	vector<IntStrPair> comets;
	vector<IntStrPair> invisibles;
	vector<IntStrPair> moons;
	vector<IntStrPair> planets;
	vector<IntStrPair> spacecraft;

	// We will use these objects to iterate over all the above vectors
	vector<IntStrPairVec> objects;
	vector<string> menuNames;

	// Place each body in the correct vector based on classification
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

	// Add each vector of PlanetarySystem bodies to a vector to iterate over
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

	// Now sort each vector and generate submenus
	IntStrPairComparePredicate pred;
	vector<IntStrPairVec>::iterator obj;
	vector<IntStrPair>::iterator it;
	vector<string>::iterator menuName;
	GtkWidget* subMenu;
	int numSubMenus;

	// Count how many submenus we need to create
	numSubMenus = 0;
	for (obj=objects.begin(); obj != objects.end(); obj++)
	{
		if (obj->size() > 0)
			numSubMenus++;
	}

	menuName = menuNames.begin();
	for (obj=objects.begin(); obj != objects.end(); obj++)
	{
		// Only generate a submenu if the vector is not empty
		if (obj->size() > 0)
		{
			// Don't create a submenu for a single item
			if (obj->size() == 1)
			{
				it=obj->begin();
				AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextPlanet), it->second.c_str(), it->first);
			}
			else
			{
				// Skip sorting if we are dealing with the planets in our own Solar System.
				if (parentName != "Sol" || *menuName != "Planets")
					sort(obj->begin(), obj->end(), pred);

				if (numSubMenus > 1)
				{
					// Add items to submenu
					subMenu = gtk_menu_new();
					//hSubMenu = CreatePopupMenu();
					for(it=obj->begin(); it != obj->end(); it++)
						AppendMenu(subMenu, GTK_SIGNAL_FUNC(handleContextPlanet), it->second.c_str(), it->first);

					gtk_menu_item_set_submenu(AppendMenu(menu, NULL, menuName->c_str(), 0), GTK_WIDGET(subMenu));
				}
				else
				{
					// Just add items to the popup
					for(it=obj->begin(); it != obj->end(); it++)
						AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextPlanet), it->second.c_str(), it->first);
				}
			}
		}
		menuName++;
	}

	return GTK_MENU(menu);
}


// FUNCTION: Create surface submenu for context menu, return menu pointer.
GtkMenu* CreateAlternateSurfaceMenu(const vector<string>& surfaces)
{
	GtkWidget* menu = gtk_menu_new();

	AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextSurface), "Normal", 0);
	for (guint i = 0; i < surfaces.size(); i++)
	{
		AppendMenu(menu, GTK_SIGNAL_FUNC(handleContextSurface), surfaces[i].c_str(), i+1);
	}

	return GTK_MENU(menu);
}


// CALLBACK: Context menu (event handled by appCore)
// Normally, float x and y, but unused, so removed.
void contextMenu(float, float, Selection sel) {
	GtkWidget* popup = gtk_menu_new();
	string name;

	switch (sel.getType())
	{
		case Selection::Type_Body:
		{
			name = sel.body()->getName();
			AppendMenu(popup, menuCenter, name.c_str(), 0);
			AppendMenu(popup, NULL, NULL, 0);
			AppendMenu(popup, menuGoto, "_Goto", 0);
			AppendMenu(popup, menuFollow, "_Follow", 0);
			AppendMenu(popup, menuSync, "S_ync Orbit", 0);
			AppendMenu(popup, NULL, "_Info", 0);

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
			Simulation* sim = appCore->getSimulation();
			name = sim->getUniverse()->getStarCatalog()->getStarName(*(sel.star()));
			AppendMenu(popup, menuCenter, name.c_str(), 0);
			AppendMenu(popup, NULL, NULL, 0);
			AppendMenu(popup, menuGoto, "_Goto", 0);
			// *** Add info eventually
			// AppendMenu(popup, NULL, "_Info", 0);

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
			AppendMenu(popup, menuCenter, sel.deepsky()->getName().c_str(), 0);
			AppendMenu(popup, NULL, NULL, 0);
			AppendMenu(popup, menuGoto, "_Goto", 0);
			AppendMenu(popup, menuFollow, "_Follow", 0);
			// *** Add info eventually
			// AppendMenu(popup, NULL, "_Info", 0);
		}
		break;

		case Selection::Type_Location:
			break;

		default:
			break;
	}

	if (appCore->getSimulation()->getUniverse()->isMarked(sel, 1))
		AppendMenu(popup, menuUnMark, "_Unmark", 0);
	else
		AppendMenu(popup, menuMark, "_Mark", 0);

	appCore->getSimulation()->setSelection(sel);

	gtk_widget_show_all(popup);	
	gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}


// GL CALLBACKS ---------------------------------------------------------------

// Keeps track of whether GL area is initialized
bool bReady = false;

// CALLBACK: GL Function for event "configure_event"
gint reshapeFunc(GtkWidget* widget, GdkEventConfigure*, gpointer)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
		return FALSE;

	// Don't save window size if going into fullscreen
	if (prefs->fullScreen == false) {
		prefs->winWidth = widget->allocation.width;
		prefs->winHeight = widget->allocation.height;
	}
	
	appCore->resize(widget->allocation.width, widget->allocation.height);

	// GConf changes only saved upon exit, since caused a lot of CPU activity
	// while saving intermediate steps.

	gdk_gl_drawable_gl_end (gldrawable);
    return TRUE;
}

// CALLBACK: GL Function for event "realize"
gint initFunc(GtkWidget* widget, gpointer)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
		return FALSE;

	if (!appCore->initRenderer())
	{
		cerr << "Failed to initialize renderer.\n";
		return TRUE;
	}

	time_t curtime=time(NULL);
	appCore->start((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
	localtime(&curtime); /* Only doing this to set timezone as a side
							effect*/
	appCore->setTimeZoneBias(-timezone + 3600 * daylight);
	appCore->setTimeZoneName(tzname[daylight]);
	timeOptions[1]=tzname[daylight];

	gdk_gl_drawable_gl_end (gldrawable);

	// Set the cursor to a crosshair
	gdk_window_set_cursor(widget->window, gdk_cursor_new(GDK_CROSSHAIR));

	return TRUE;
}

// GL Common Draw function.
// If everything checks out, call appCore->draw()
gint Display(GtkWidget* widget)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
		return FALSE;

    if (bReady)
    {
        appCore->draw();
        gdk_gl_drawable_swap_buffers(GDK_GL_DRAWABLE(gldrawable));
    }

	gdk_gl_drawable_gl_end (gldrawable);
	return TRUE;
}

// CALLBACK: GL Function for main update (in GTK idle loop)
gint glarea_idle(void*)
{
    appCore->tick();
    return Display(oglArea);
}

// CALLBACK: GL Function for event "expose_event"
gint gldrawFunc(GtkWidget* widget, GdkEventExpose* event, gpointer)
{
    // Draw only the last expose
    if (event->count > 0)
        return TRUE;

	// Redraw -- draw checks are made in function
	return Display(widget);
}


gint glarea_motion_notify(GtkWidget*, GdkEventMotion* event)
{
    int x = (int) event->x;
    int y = (int) event->y;

    int buttons = 0;
    if ((event->state & GDK_BUTTON1_MASK) != 0)
        buttons |= CelestiaCore::LeftButton;
    if ((event->state & GDK_BUTTON2_MASK) != 0)
        buttons |= CelestiaCore::MiddleButton;
    if ((event->state & GDK_BUTTON3_MASK) != 0)
        buttons |= CelestiaCore::RightButton;
    if ((event->state & GDK_SHIFT_MASK) != 0)
        buttons |= CelestiaCore::ShiftKey;
    if ((event->state & GDK_CONTROL_MASK) != 0)
        buttons |= CelestiaCore::ControlKey;

    appCore->mouseMove(x - lastX, y - lastY, buttons);

    lastX = x;
    lastY = y;

    return TRUE;
}

gint glarea_mouse_scroll(GtkWidget*, GdkEventScroll* event, gpointer)
{
	if (event->direction == GDK_SCROLL_UP)
        appCore->mouseWheel(-1.0f, 0);
	else 
        appCore->mouseWheel(1.0f, 0);
	
	return TRUE;
}

gint glarea_button_press(GtkWidget*, GdkEventButton* event, gpointer) 
{
    lastX = (int) event->x;
    lastY = (int) event->y;

    if (event->button == 1)
        appCore->mouseButtonDown(event->x, event->y, CelestiaCore::LeftButton);
    else if (event->button == 2)
        appCore->mouseButtonDown(event->x, event->y, CelestiaCore::MiddleButton);
    else if (event->button == 3)
        appCore->mouseButtonDown(event->x, event->y, CelestiaCore::RightButton);

    return TRUE;
}

gint glarea_button_release(GtkWidget*, GdkEventButton* event, gpointer)
{
    lastX = (int) event->x;
    lastY = (int) event->y;

    if (event->button == 1)
        appCore->mouseButtonUp(event->x, event->y, CelestiaCore::LeftButton);
    else if (event->button == 2)
        appCore->mouseButtonUp(event->x, event->y, CelestiaCore::MiddleButton);
    else if (event->button == 3)
        appCore->mouseButtonUp(event->x, event->y, CelestiaCore::RightButton);

    return TRUE;
}


static bool handleSpecialKey(int key, bool down)
{
    int k = -1;
    switch (key)
    {
    case GDK_Up:
        k = CelestiaCore::Key_Up;
        break;
    case GDK_Down:
        k = CelestiaCore::Key_Down;
        break;
    case GDK_Left:
        k = CelestiaCore::Key_Left;
        break;
    case GDK_Right:
        k = CelestiaCore::Key_Right;
        break;
    case GDK_Home:
        k = CelestiaCore::Key_Home;
        break;
    case GDK_End:
        k = CelestiaCore::Key_End;
        break;
    case GDK_F1:
        k = CelestiaCore::Key_F1;
        break;
    case GDK_F2:
        k = CelestiaCore::Key_F2;
        break;
    case GDK_F3:
        k = CelestiaCore::Key_F3;
        break;
    case GDK_F4:
        k = CelestiaCore::Key_F4;
        break;
    case GDK_F5:
        k = CelestiaCore::Key_F5;
        break;
    case GDK_F6:
        k = CelestiaCore::Key_F6;
        break;
    case GDK_F7:
        k = CelestiaCore::Key_F7;
        break;
    case GDK_F10:
        if (down)
            menuCaptureImage();
        break;
    case GDK_KP_Insert:
    case GDK_KP_0:
        k = CelestiaCore::Key_NumPad0;
        break;
    case GDK_KP_End:
    case GDK_KP_1:
        k = CelestiaCore::Key_NumPad1;
        break;
    case  GDK_KP_Down:
    case GDK_KP_2:
        k = CelestiaCore::Key_NumPad2;
        break;
    case GDK_KP_Next:
    case GDK_KP_3:
        k = CelestiaCore::Key_NumPad3;
        break;
    case GDK_KP_Left:
    case GDK_KP_4:
        k = CelestiaCore::Key_NumPad4;
        break;
    case GDK_KP_Begin:
    case GDK_KP_5:
        k = CelestiaCore::Key_NumPad5;
        break;
    case GDK_KP_Right:
    case GDK_KP_6:
        k = CelestiaCore::Key_NumPad6;
        break;
    case GDK_KP_Home:
    case GDK_KP_7:
        k = CelestiaCore::Key_NumPad7;
        break;
    case GDK_KP_Up:
    case GDK_KP_8:
        k = CelestiaCore::Key_NumPad8;
        break;
    case GDK_KP_Prior:
    case GDK_KP_9:
        k = CelestiaCore::Key_NumPad9;
        break;
    case GDK_A:
    case GDK_a:
        k = 'A';
        break;
    case GDK_Z:
    case GDK_z:
        k = 'Z';
        break;
    }

    if (k >= 0)
    {
        if (down)
            appCore->keyDown(k);
        else
            appCore->keyUp(k);
        return (k < 'A' || k > 'Z');
    }
    else
    {
        return false;
    }
}


gint glarea_key_press(GtkWidget* widget, GdkEventKey* event, gpointer)
{
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_press_event");
    switch (event->keyval)
    {
    case GDK_Escape:
        appCore->charEntered('\033');
        break;

	// The next few things are sort of "hacks"
	// They catch any keypresses that have preferences associeated with them
	// but don't have menu entries. This way the prefs can be updated.
	case 's':
	case 'S':
		if(event->state & GDK_CONTROL_MASK) {
			appCore->charEntered(event->string);
			prefs->starStyle = appCore->getRenderer()->getStarStyle();

			#ifdef GNOME
			gconf_client_set_int(client, "/apps/celestia/starStyle", prefs->starStyle, NULL);
			#endif

			break;
		}

	case '{':
	case '}':
		appCore->charEntered(event->string);
		prefs->ambientLight = appRenderer->getAmbientLightLevel();

		#ifdef GNOME
		gconf_client_set_float(client, "/apps/celestia/ambientLight", prefs->ambientLight, NULL);
		#endif

		break;

    default:
        if (!handleSpecialKey(event->keyval, true))
        {
            if ((event->string != NULL) && (*(event->string)))
            {
                // See if our key accelerators will handle this event.
	        if((!appCore->getTextEnterMode()) && gtk_accel_groups_activate (G_OBJECT (mainWindow), event->keyval, GDK_SHIFT_MASK))
                    return TRUE;
            
                char* s = event->string;

                while (*s != '\0')
                {
                    char c = *s++;
                    appCore->charEntered(c);
                }
            }
        }
        if (event->state & GDK_MOD1_MASK)
	    return FALSE;
    }

    return TRUE;
}


gint glarea_key_release(GtkWidget* widget, GdkEventKey* event, gpointer)
{
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_release_event");
    return handleSpecialKey(event->keyval, false);
}


#ifdef GNOME
// GNOME initializing function options struct
struct poptOption options[] =
{
    {
		"verbose",
		'v',
		POPT_ARG_INT,
		&verbose,
		0,
		"Lots of additional Messages",
		NULL
	},

    {
		NULL,
		'\0',
		0,
		NULL,
		0,
		NULL,
		NULL
  }
};
#endif


void resyncMenus()
{
    // Check all the toggle items that they are set on/off correctly
    int i = sizeof(checks) / sizeof(checks[0]);
    for(CheckFunc *cfunc=&checks[--i];i>=0;cfunc=&checks[--i])
    {
	if (!cfunc->active)
	    continue;
        unsigned int res=((*cfunc->func)(cfunc->funcdata, (char *)cfunc->path))?1:0;
#if 0
        cout <<"res  "<<res<<'\t'<<i<<'\t'<<cfunc->path<<endl;
#endif
        if (cfunc->active & 2)
        {
            g_assert(cfunc->path);
            cfunc->widget=GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(menuItemFactory, cfunc->path));
            if (!cfunc->widget)
            {
                cfunc->active=0;
                DPRINTF(0, "Menu item %s status checking deactivated due to being unable to find it!", cfunc->path);
                continue;
            }
            g_assert(cfunc->func);
            if (cfunc->widget->active != res)
            {
                // Change state of widget *without* causing a message to be
                // sent (which would change the state again).
                gtk_widget_hide(GTK_WIDGET(cfunc->widget));
                cfunc->widget->active=res;
                gtk_widget_show(GTK_WIDGET(cfunc->widget));
            }
        } else {
        if ((((unsigned int)(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cfunc->optWidget))?1:0)) != res) && (optionDialog))
        {
            // Change state of OptionButton *without* causing a message to be
            // sent (which would change the state again).
            gtk_widget_hide(GTK_WIDGET(cfunc->optWidget));
            (GTK_TOGGLE_BUTTON(cfunc->optWidget))->active=res;
            gtk_widget_show(GTK_WIDGET(cfunc->optWidget));
        }
    }
    }
}


void resyncAmbient()
{
    if(optionDialog) // Only  modify if optionsDialog is setup
    {
        float ambient = appRenderer->getAmbientLightLevel();
        int index = 2;
        for(int i=2; (ambient<=amLevels[i]) && (i>=0); i--)
            index = i;
        if(!(ambientGads[index])->active)
            gtk_toggle_button_set_active(ambientGads[index],1);
    }
}


/*
void resyncFaintest()
{
    if(optionDialog) // Only  modify if optionsDialog is setup
    {
        float val=appSim->getFaintestVisible();
        if(val != gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(spinner)))
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), (gfloat)val);
    }
}
*/


void resyncVerbosity()
{
    if(optionDialog) // Only  modify if optionsDialog is setup
    {
        int index=appCore->getHudDetail();
        if(!(infoGads[index])->active)
            gtk_toggle_button_set_active(infoGads[index],1);
    }
}


void resyncAll()
{
    resyncMenus();
    resyncAmbient();
    resyncVerbosity();
	// There is no longer a way to set faintest.
    // resyncFaintest();
}


/* Our own watcher. Celestiacore will call notifyChange() to tell us
   we need to recheck the check menu items and option buttons. */

class GtkWatcher : public CelestiaWatcher
{
public:
    GtkWatcher(CelestiaCore*);
    virtual void notifyChange(CelestiaCore*, int);
};


GtkWatcher::GtkWatcher(CelestiaCore* _appCore) :
    CelestiaWatcher(*_appCore)
{
}

void GtkWatcher::notifyChange(CelestiaCore*, int property)
{
    if ((property & (CelestiaCore::RenderFlagsChanged|CelestiaCore::LabelFlagsChanged|CelestiaCore::TimeZoneChanged)))
        resyncMenus();
    else if (property & CelestiaCore::AmbientLightChanged)
        resyncAmbient();
	// There is no longer a way to change faintest level.
    // else if (property & CelestiaCore::FaintestChanged)
    //     resyncFaintest();
    else if (property & CelestiaCore::VerbosityLevelChanged)
        resyncVerbosity();
}


GtkWatcher *gtkWatcher=NULL;


#ifdef GNOME
// Reads in GConf->Main preferences into AppPreferences
void readGConfMain(AppPreferences* p) {
	// This area should, in theory, have error checking
	p->winWidth = gconf_client_get_int(client, "/apps/celestia/winWidth", NULL);
	p->winHeight = gconf_client_get_int(client, "/apps/celestia/winHeight", NULL);
	p->winX = gconf_client_get_int(client, "/apps/celestia/winX", NULL);
	p->winY = gconf_client_get_int(client, "/apps/celestia/winY", NULL);
	p->ambientLight = gconf_client_get_float(client, "/apps/celestia/ambientLight", NULL);
	p->visualMagnitude = gconf_client_get_float(client, "/apps/celestia/visualMagnitude", NULL);
	p->showLocalTime = gconf_client_get_bool(client, "/apps/celestia/showLocalTime", NULL);
	p->hudDetail = gconf_client_get_int(client, "/apps/celestia/hudDetail", NULL);
	p->fullScreen = gconf_client_get_bool(client, "/apps/celestia/fullScreen", NULL);
	p->starStyle = (Renderer::StarStyle)gconf_client_get_int(client, "/apps/celestia/starStyle", NULL);
	p->altSurfaceName = gconf_client_get_string(client, "/apps/celestia/altSurfaceName", NULL);
}

// Reads in GConf->Labels preferences into AppPreferences
void readGConfLabels(AppPreferences* p) {
	p->labelMode = Renderer::NoLabels;
	p->labelMode |= Renderer::StarLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/star", NULL);
	p->labelMode |= Renderer::PlanetLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/planet", NULL);
	p->labelMode |= Renderer::MoonLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/moon", NULL);
	p->labelMode |= Renderer::ConstellationLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/constellation", NULL);
	p->labelMode |= Renderer::GalaxyLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/galaxy", NULL);
	p->labelMode |= Renderer::AsteroidLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/asteroid", NULL);
	p->labelMode |= Renderer::SpacecraftLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/spacecraft", NULL);
	p->labelMode |= Renderer::LocationLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/location", NULL);
	p->labelMode |= Renderer::CometLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/comet", NULL);
}

// Reads in GConf->Render preferences into AppPreferences
void readGConfRender(AppPreferences* p) {
	p->renderFlags = Renderer::ShowNothing;
	p->renderFlags |= Renderer::ShowStars * gconf_client_get_bool(client, "/apps/celestia/render/stars",  NULL);
	p->renderFlags |= Renderer::ShowPlanets * gconf_client_get_bool(client, "/apps/celestia/render/planets",  NULL);
	p->renderFlags |= Renderer::ShowGalaxies * gconf_client_get_bool(client, "/apps/celestia/render/galaxies",  NULL);
	p->renderFlags |= Renderer::ShowDiagrams * gconf_client_get_bool(client, "/apps/celestia/render/diagrams",  NULL);
	p->renderFlags |= Renderer::ShowCloudMaps * gconf_client_get_bool(client, "/apps/celestia/render/cloudMaps",  NULL);
	p->renderFlags |= Renderer::ShowOrbits * gconf_client_get_bool(client, "/apps/celestia/render/orbits",  NULL);
	p->renderFlags |= Renderer::ShowCelestialSphere * gconf_client_get_bool(client, "/apps/celestia/render/celestialSphere",  NULL);
	p->renderFlags |= Renderer::ShowNightMaps * gconf_client_get_bool(client, "/apps/celestia/render/nightMaps",  NULL);
	p->renderFlags |= Renderer::ShowAtmospheres * gconf_client_get_bool(client, "/apps/celestia/render/atmospheres",  NULL);
	p->renderFlags |= Renderer::ShowSmoothLines * gconf_client_get_bool(client, "/apps/celestia/render/smoothLines",  NULL);
	p->renderFlags |= Renderer::ShowEclipseShadows * gconf_client_get_bool(client, "/apps/celestia/render/eclipseShadows",  NULL);
	p->renderFlags |= Renderer::ShowStarsAsPoints * gconf_client_get_bool(client, "/apps/celestia/render/starsAsPoints",  NULL);
	p->renderFlags |= Renderer::ShowRingShadows * gconf_client_get_bool(client, "/apps/celestia/render/ringShadows",  NULL);
	p->renderFlags |= Renderer::ShowBoundaries * gconf_client_get_bool(client, "/apps/celestia/render/boundaries",  NULL);
	p->renderFlags |= Renderer::ShowAutoMag * gconf_client_get_bool(client, "/apps/celestia/render/autoMag",  NULL);
	p->renderFlags |= Renderer::ShowCometTails * gconf_client_get_bool(client, "/apps/celestia/render/cometTails",  NULL);
	p->renderFlags |= Renderer::ShowMarkers * gconf_client_get_bool(client, "/apps/celestia/render/markers",  NULL);
}

// Reads in GConf->Orbits preferences into AppPreferences
void readGConfOrbits(AppPreferences* p) {
	p->orbitMask = 0;
	p->orbitMask |= Body::Planet * gconf_client_get_bool(client, "/apps/celestia/orbits/planet",  NULL);
	p->orbitMask |= Body::Moon * gconf_client_get_bool(client, "/apps/celestia/orbits/moon",  NULL);
	p->orbitMask |= Body::Asteroid * gconf_client_get_bool(client, "/apps/celestia/orbits/asteroid",  NULL);
	p->orbitMask |= Body::Comet * gconf_client_get_bool(client, "/apps/celestia/orbits/comet",  NULL);
	p->orbitMask |= Body::Spacecraft * gconf_client_get_bool(client, "/apps/celestia/orbits/spacecraft",  NULL);
	p->orbitMask |= Body::Invisible * gconf_client_get_bool(client, "/apps/celestia/orbits/invisible",  NULL);
	p->orbitMask |= Body::Unknown * gconf_client_get_bool(client, "/apps/celestia/orbits/unknown",  NULL);
}
#endif

// Loads saved preferences into AppPreference struct
// Uses GConf for Gnome, text file for GTK
void loadSavedPreferences(AppPreferences* p) {
	// Defaults, although GConf will have its own
	p->winWidth = 640;
	p->winHeight = 480;
	p->winX = -1;
	p->winY = -1;
	p->ambientLight = 0.1f;  // Low
	p->labelMode = 0;
	p->orbitMask = Body::Planet | Body::Moon;
	p->renderFlags = Renderer::ShowAtmospheres | Renderer::ShowStars |
	                 Renderer::ShowPlanets | Renderer::ShowSmoothLines |
	                 Renderer::ShowCometTails | Renderer::ShowRingShadows;
	p->visualMagnitude = 8.5f;   //Default specified in Simulation::Simulation()
	p->showLocalTime = 0;
	p->hudDetail = 1;
	p->fullScreen = 0;
	p->starStyle = Renderer::FuzzyPointStars;
	p->altSurfaceName = "";

	#ifdef GNOME
	readGConfMain(p);
	readGConfLabels(p);
	readGConfRender(p);
	readGConfOrbits(p);
	#endif
}

// Applies all the preferences in AppPreferences except window size/position.
void applyPreferences(AppPreferences* p) {
	// This should eventually call all the various update functions.
	// For now, some duplication is okay.

	// Apply any preferences that do not apply directly to GTK UI
	appCore->getSimulation()->setFaintestVisible(p->visualMagnitude);
	appCore->getRenderer()->setRenderFlags(p->renderFlags);
	appCore->getRenderer()->setLabelMode(p->labelMode);
	appCore->getRenderer()->setOrbitMask(p->orbitMask);
	appCore->getRenderer()->setAmbientLightLevel(p->ambientLight);
	appCore->getRenderer()->setStarStyle(p->starStyle);
	appCore->setHudDetail(p->hudDetail);
	if (p->showLocalTime) {
		appCore->setTimeZoneBias(-timezone + 3600 * daylight);
		appCore->setTimeZoneName(tzname[daylight]);
	}
	else {
		appCore->setTimeZoneBias(0);
		appCore->setTimeZoneName("UTC");
	}
	appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(p->altSurfaceName);
}

#ifdef GNOME
// GCONF CALLBACK: Reloads labels upon change
void confLabels(GConfClient*, guint, GConfEntry*, gpointer) {
	// Reload all the labels
	readGConfLabels(prefs);

	// Set label flags
	appCore->getRenderer()->setLabelMode(prefs->labelMode);
}

// GCONF CALLBACK: Reloads render flags upon change
void confRender(GConfClient*, guint, GConfEntry*, gpointer) {
	// Reload all the render flags
	readGConfRender(prefs);

	// Set render flags
	appCore->getRenderer()->setRenderFlags(prefs->renderFlags);
}

// GCONF CALLBACK: Reloads orbits upon change
void confOrbits(GConfClient*, guint, GConfEntry*, gpointer) {
	// Reload all the labels
	readGConfOrbits(prefs);

	// Set orbit flags
	appCore->getRenderer()->setOrbitMask(prefs->orbitMask);
}

// GCONF CALLBACK: Sets window width upon change
void confWinWidth(GConfClient* c, guint, GConfEntry* e, gpointer) {
	int w = gconf_client_get_int(c, gconf_entry_get_key(e), NULL);

	if (w != prefs->winWidth) {
		int winX, winY;
		gtk_window_get_size(GTK_WINDOW(mainWindow), &winX, &winY);
		gtk_window_resize(GTK_WINDOW(mainWindow), w + winX - prefs->winWidth, winY);
		prefs->winWidth = w;
	}
}

// GCONF CALLBACK: Sets window height upon change
void confWinHeight(GConfClient* c, guint, GConfEntry* e, gpointer) {
	int h = gconf_client_get_int(c, gconf_entry_get_key(e), NULL);

	if (h != prefs->winHeight) {
		int winX, winY;
		gtk_window_get_size(GTK_WINDOW(mainWindow), &winX, &winY);
		gtk_window_resize(GTK_WINDOW(mainWindow), winX, h + winY - prefs->winHeight);
		prefs->winHeight = h;
	}
}

// GCONF CALLBACK: Sets window X position upon change
void confWinX(GConfClient* c, guint, GConfEntry* e, gpointer) {
	int x = gconf_client_get_int(c, gconf_entry_get_key(e), NULL);

	if (x != prefs->winX) {
		prefs->winX = x;
		if (prefs->winX > 0 && prefs->winY > 0)
			gtk_window_move(GTK_WINDOW(mainWindow), prefs->winX, prefs->winY);
	}
}

// GCONF CALLBACK: Sets window Y position upon change
void confWinY(GConfClient* c, guint, GConfEntry* e, gpointer) {
	int y = gconf_client_get_int(c, gconf_entry_get_key(e), NULL);

	if (y != prefs->winY) {
		prefs->winY = y;
		if (prefs->winX > 0 && prefs->winY > 0)
			gtk_window_move(GTK_WINDOW(mainWindow), prefs->winX, prefs->winY);
	}
}

// GCONF CALLBACK: Reloads ambient light setting upon change
void confAmbientLight(GConfClient* c, guint, GConfEntry* e, gpointer) {
	prefs->ambientLight = gconf_client_get_float(c, gconf_entry_get_key(e), NULL);
	appCore->getRenderer()->setAmbientLightLevel(prefs->ambientLight);
}

// GCONF CALLBACK: Reloads visual magnitude setting upon change
void confVisualMagnitude(GConfClient* c, guint, GConfEntry* e, gpointer) {
	prefs->visualMagnitude = gconf_client_get_float(c, gconf_entry_get_key(e), NULL);
	appCore->getSimulation()->setFaintestVisible(prefs->visualMagnitude);
}

// GCONF CALLBACK: Sets "show local time" setting upon change
void confShowLocalTime(GConfClient* c, guint, GConfEntry* e, gpointer) {
	prefs->showLocalTime = gconf_client_get_bool(c, gconf_entry_get_key(e), NULL);
	if (prefs->showLocalTime) {
		appCore->setTimeZoneBias(-timezone + 3600 * daylight);
		appCore->setTimeZoneName(tzname[daylight]);
	}
	else {
		appCore->setTimeZoneBias(0);
		appCore->setTimeZoneName("UTC");
	}
}

// GCONF CALLBACK: Sets HUD detail when changed
void confHudDetail(GConfClient* c, guint, GConfEntry* e, gpointer) {
	prefs->hudDetail = gconf_client_get_int(c, gconf_entry_get_key(e), NULL);
	appCore->setHudDetail(prefs->hudDetail);
}

// GCONF CALLBACK: Sets window to fullscreen when change occurs
void confFullScreen(GConfClient* c, guint, GConfEntry* e, gpointer) {
	prefs->fullScreen = gconf_client_get_bool(c, gconf_entry_get_key(e), NULL);
	if (prefs->fullScreen)
		gtk_window_fullscreen(GTK_WINDOW(mainWindow));
}

// GCONF CALLBACK: Sets star style when changed
void confStarStyle(GConfClient* c, guint, GConfEntry* e, gpointer) {
	prefs->starStyle = (Renderer::StarStyle)gconf_client_get_int(c, gconf_entry_get_key(e), NULL);
	appCore->getRenderer()->setStarStyle(prefs->starStyle);
}

// GCONF CALLBACK: Sets alternate surface texture when changed
void confAltSurfaceName(GConfClient* c, guint, GConfEntry* e, gpointer) {
	prefs->altSurfaceName = gconf_client_get_string(c, gconf_entry_get_key(e), NULL);
	appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(prefs->altSurfaceName);
}
// End GConf callbacks
#endif

// CALLBACK: When window position changed
int moveWindowCallback(GtkWidget* w, gpointer) {
	int x, y;
	gtk_window_get_position(GTK_WINDOW(w), &x, &y);

	// Don't save window position if going into fullscreen
	if (prefs->fullScreen == false) {
		prefs->winX = x;
		prefs->winY = y;
	}

	// Saving of preferences was removed from here to the end of the main
	// function. It was much too CPU intensive to save after every slightest
	// change during a window move.

	// Return false so other handlers run too
	return FALSE;
}


// MAIN
int main(int argc, char* argv[])
{
    // Say we're not ready to render yet.
    bReady = false;

    // Set starting filename for screen captures
    captureFilename = new string(getcwd(NULL,0));
    *captureFilename+="/celestia.jpg";
    
    if (chdir(CONFIG_DATA_DIR) == -1)
    {
        cerr << "Cannot chdir to '" << CONFIG_DATA_DIR <<
            "', probably due to improper installation\n";
    }

	#ifdef GNOME
	// GNOME Initialization
	GnomeProgram *program;
	program = gnome_program_init("Celestia", VERSION, LIBGNOMEUI_MODULE,
	                             argc, argv, GNOME_PARAM_POPT_TABLE, options,
								 GNOME_PARAM_NONE);
	#else
	// GTK-Only Initialization
	gtk_init(&argc, &argv);
	#endif
	
    SetDebugVerbosity(verbose);

    appCore = new CelestiaCore();
    if (appCore == NULL)
    {
        cerr << "Out of memory.\n";
        return 1;
    }

    if (!appCore->initSimulation())
    {
        return 1;
    }

	#ifdef GNOME
    // Create the main window (GNOME)
    mainWindow = gnome_app_new("Celestia", AppName);
	#else
    // Create the main window (GTK)
	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	#endif

    mainBox = GTK_WIDGET(gtk_vbox_new(FALSE, 0));
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 0);

    g_signal_connect(GTK_OBJECT(mainWindow), "delete_event",
                       G_CALLBACK(gtk_main_quit), NULL);

    // Create the OpenGL widget
	gtk_gl_init (&argc, &argv);

	// Configure OpenGL
	// Try double-buffered visual
	GdkGLConfig* glconfig = gdk_gl_config_new_by_mode(static_cast<GdkGLConfigMode>
	                                                  (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE));

	if (glconfig == NULL)
	{
		g_print ("*** Cannot find the double-buffered visual.\n");
		g_print ("*** Trying single-buffered visual.\n");

		// Try single-buffered visual
		glconfig = gdk_gl_config_new_by_mode(static_cast<GdkGLConfigMode>
		                                     (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH));
		if (glconfig == NULL)
		{
			g_print ("*** No appropriate OpenGL-capable visual found.\n");
			exit (1);
		}
	}

	// Load saved settings
	AppPreferences preferences;
	prefs = &preferences;
	#ifdef GNOME
	client = gconf_client_get_default();
	gconf_client_add_dir(client, "/apps/celestia", GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	#endif
	loadSavedPreferences(prefs);

	if (prefs->winX > 0 && prefs->winY > 0)
		gtk_window_move(GTK_WINDOW(mainWindow), prefs->winX, prefs->winY);

	if (prefs->fullScreen)
		gtk_window_fullscreen(GTK_WINDOW(mainWindow));

	// Create area to be used for OpenGL display
	oglArea = gtk_drawing_area_new();
	
	// Set OpenGL-capability to the widget.
	gtk_widget_set_gl_capability(oglArea,
					glconfig,
					NULL,
					TRUE,
					GDK_GL_RGBA_TYPE);
	
    gtk_widget_set_events(GTK_WIDGET(oglArea),
                          GDK_EXPOSURE_MASK |
                          GDK_KEY_PRESS_MASK |
                          GDK_KEY_RELEASE_MASK |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);

    // Set the default size
	gtk_widget_set_size_request(oglArea, prefs->winWidth, prefs->winHeight);

    // Connect signal handlers
    g_signal_connect(GTK_OBJECT(oglArea), "expose_event",
                       G_CALLBACK(gldrawFunc), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "configure_event",
                       G_CALLBACK(reshapeFunc), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "realize",
                       G_CALLBACK(initFunc), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "button_press_event",
                       G_CALLBACK(glarea_button_press), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "button_release_event",
                       G_CALLBACK(glarea_button_release), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "scroll_event",
                       G_CALLBACK(glarea_mouse_scroll), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "motion_notify_event",
                       G_CALLBACK(glarea_motion_notify), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "key_press_event",
                       G_CALLBACK(glarea_key_press), NULL);
    g_signal_connect(GTK_OBJECT(oglArea), "key_release_event",
                       G_CALLBACK(glarea_key_release), NULL);

    g_signal_connect(GTK_OBJECT(mainWindow), "configure-event",
                       G_CALLBACK(moveWindowCallback), NULL);

    // Frames and boxes for the Options Dialog
    showFrame = gtk_frame_new("Show");
    g_assert(showFrame);
    showBox = gtk_vbox_new(FALSE, 0);
    g_assert(showBox);
    gtk_container_border_width(GTK_CONTAINER(showBox), CELSPACING);
    gtk_container_add(GTK_CONTAINER(showFrame), GTK_WIDGET(showBox));
    gtk_container_border_width(GTK_CONTAINER(showFrame), 0);
	
    labelFrame = gtk_frame_new("Label");
    g_assert(labelFrame);
    labelBox = gtk_vbox_new(FALSE, 0);
    g_assert(labelBox);
    gtk_container_border_width(GTK_CONTAINER(labelBox), CELSPACING);
    gtk_container_add(GTK_CONTAINER(labelFrame), GTK_WIDGET(labelBox));
    gtk_container_border_width(GTK_CONTAINER(labelFrame), 0);

	orbitFrame = gtk_frame_new("Orbits");
	g_assert(orbitFrame);
	orbitBox = gtk_vbox_new(FALSE, 0);
	g_assert(orbitBox);
	gtk_container_border_width(GTK_CONTAINER(orbitBox), CELSPACING);
	gtk_container_add(GTK_CONTAINER(orbitFrame), GTK_WIDGET(orbitBox));
	gtk_container_border_width(GTK_CONTAINER(orbitFrame), 0);

	// Create the main menu bar
	createMainMenu(mainWindow, &mainMenu);

	#ifdef GNOME
	// Set window contents (GNOME)
    gnome_app_set_contents((GnomeApp *)mainWindow, GTK_WIDGET(mainBox));
	gtk_window_set_default_icon_from_file("celestia.svg", NULL);
	#else
	// Set window contents (GTK)
    gtk_container_add(GTK_CONTAINER(mainWindow), GTK_WIDGET(mainBox));
	#endif

    gtk_box_pack_start(GTK_BOX(mainBox), mainMenu, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), oglArea, TRUE, TRUE, 0);

    g_signal_connect(GTK_OBJECT(mainBox), "event",
                       G_CALLBACK(resyncAll), NULL);

    // Set focus to oglArea widget
    GTK_WIDGET_SET_FLAGS(oglArea, GTK_CAN_FOCUS);
    gtk_widget_grab_focus(GTK_WIDGET(oglArea));

	// Set context menu callback for the core
	appCore->setContextMenuCallback(contextMenu);

	// Main call to execute redraw
	gtk_idle_add(glarea_idle, NULL);

    gtkWatcher = new GtkWatcher(appCore);

    g_assert(menuItemFactory);
    resyncAll();

	gtk_widget_show_all(mainWindow);
    bReady = true;

	// HACK: Now that window is drawn, set minimum window size
	gtk_widget_set_size_request(oglArea, 320, 240);
	
	// Apply loaded preferences
	applyPreferences(prefs);

	#ifdef GNOME
	// Add preference client notifiers.
	gconf_client_notify_add (client, "/apps/celestia/labels", confLabels, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/render", confRender, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/orbits", confOrbits, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/winWidth", confWinWidth, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/winHeight", confWinHeight, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/winX", confWinX, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/winY", confWinY, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/ambientLight", confAmbientLight, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/visualMagnitude", confVisualMagnitude, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/showLocalTime", confShowLocalTime, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/hudDetail", confHudDetail, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/fullScreen", confFullScreen, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/starStyle", confStarStyle, NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/celestia/altSurfaceName", confAltSurfaceName, NULL, NULL, NULL);
	#endif

	// Call Main GTK Loop
	gtk_main();
    
	// Clean up
	delete captureFilename;

	#ifdef GNOME
	// Save window position
	gconf_client_set_int(client, "/apps/celestia/winX", prefs->winX, NULL);
	gconf_client_set_int(client, "/apps/celestia/winY", prefs->winY, NULL);

	// Save window  size
    gconf_client_set_int(client, "/apps/celestia/winWidth", prefs->winWidth, NULL);
    gconf_client_set_int(client, "/apps/celestia/winHeight", prefs->winHeight, NULL);

	g_object_unref (G_OBJECT (client));
	#endif

    return 0;
}
