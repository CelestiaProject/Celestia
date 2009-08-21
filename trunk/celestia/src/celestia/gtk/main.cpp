/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: main.cpp,v 1.9 2008-01-21 04:55:19 suwalski Exp $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <time.h>

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif /* WIN32 */

#include <GL/glew.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>

#include <celengine/astro.h>
#include <celengine/celestia.h>

#include <celengine/galaxy.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celutil/debug.h>

/* Includes for the GNOME front-end */
#ifdef GNOME
#include <gnome.h>
#include <libgnomeui/libgnomeui.h>
#include <gconf/gconf-client.h>
#endif /* GNOME */

/* Includes for the GTK front-end */
#include "common.h"
#include "glwidget.h"
#include "menu-context.h"
#include "splash.h"
#include "ui.h"

/* Includes for the settings interface */
#ifdef GNOME
#include "settings-gconf.h"
#else
#include "settings-file.h"
#endif /* GNOME */

#ifndef DEBUG
#define G_DISABLE_ASSERT
#endif /* DEBUG */


using namespace std;


/* Function Definitions */
static void createMainMenu(GtkWidget* window, AppData* app);
static void initRealize(GtkWidget* widget, AppData* app);


/* Command-Line Options */
static gchar* configFile = NULL;
static gchar* installDir = NULL;
static gchar** extrasDir = NULL;
static gboolean fullScreen = FALSE;
static gboolean noSplash = FALSE;

/* Command-Line Options specification */
static GOptionEntry optionEntries[] =
{
	{ "conf", 'c', 0, G_OPTION_ARG_FILENAME, &configFile, "Alternate configuration file", "file" },
	{ "dir", 'd', 0, G_OPTION_ARG_FILENAME, &installDir, "Alternate installation directory", "directory" },
	{ "extrasdir", 'e', 0, G_OPTION_ARG_FILENAME_ARRAY, &extrasDir, "Additional \"extras\" directory", "directory" },
	{ "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &fullScreen, "Start full-screen", NULL },
	{ "nosplash", 's', 0, G_OPTION_ARG_NONE, &noSplash, "Disable splash screen", NULL },
	{ NULL, NULL, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
};


/* Initializes GtkActions and creates main menu */
static void createMainMenu(GtkWidget* window, AppData* app)
{
	GtkUIManager *ui_manager;
	GtkAccelGroup *accel_group;
	GError *error;
	
	app->agMain = gtk_action_group_new ("MenuActions");
	app->agRender = gtk_action_group_new("RenderActions");
	app->agLabel = gtk_action_group_new("LabelActions");
	app->agOrbit = gtk_action_group_new("OrbitActions");
	app->agVerbosity = gtk_action_group_new("VerbosityActions");
	app->agStarStyle = gtk_action_group_new("StarStyleActions");
	app->agAmbient = gtk_action_group_new("AmbientActions");
	
	/* All actions have the AppData structure passed */
	gtk_action_group_add_actions(app->agMain, actionsPlain, G_N_ELEMENTS(actionsPlain), app);
	gtk_action_group_add_toggle_actions(app->agMain, actionsToggle, G_N_ELEMENTS(actionsToggle), app);
	gtk_action_group_add_radio_actions(app->agVerbosity, actionsVerbosity, G_N_ELEMENTS(actionsVerbosity), 0, G_CALLBACK(actionVerbosity), app);
	gtk_action_group_add_radio_actions(app->agStarStyle, actionsStarStyle, G_N_ELEMENTS(actionsStarStyle), 0, G_CALLBACK(actionStarStyle), app);
	gtk_action_group_add_radio_actions(app->agAmbient, actionsAmbientLight, G_N_ELEMENTS(actionsAmbientLight), 0, G_CALLBACK(actionAmbientLight), app);
	gtk_action_group_add_toggle_actions(app->agRender, actionsRenderFlags, G_N_ELEMENTS(actionsRenderFlags), app);
	gtk_action_group_add_toggle_actions(app->agLabel, actionsLabelFlags, G_N_ELEMENTS(actionsLabelFlags), app);
	gtk_action_group_add_toggle_actions(app->agOrbit, actionsOrbitFlags, G_N_ELEMENTS(actionsOrbitFlags), app);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, app->agMain, 0);
	gtk_ui_manager_insert_action_group(ui_manager, app->agRender, 0);
	gtk_ui_manager_insert_action_group(ui_manager, app->agLabel, 0);
	gtk_ui_manager_insert_action_group(ui_manager, app->agOrbit, 0);
	gtk_ui_manager_insert_action_group(ui_manager, app->agStarStyle, 0);
	gtk_ui_manager_insert_action_group(ui_manager, app->agAmbient, 0);
	gtk_ui_manager_insert_action_group(ui_manager, app->agVerbosity, 0);

	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW (window), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_file(ui_manager, "celestiaui.xml", &error))
	{
		g_message("Building menus failed: %s", error->message);
		g_error_free(error);
		exit(EXIT_FAILURE);
	}

	app->mainMenu = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
}


/* Our own watcher. Celestiacore will call notifyChange() to tell us
 * we need to recheck the check menu items and option buttons. */

class GtkWatcher : public CelestiaWatcher
{
	public:
	    GtkWatcher(CelestiaCore*, AppData*);
	    virtual void notifyChange(CelestiaCore*, int);
	private:
		AppData* app;
};

GtkWatcher::GtkWatcher(CelestiaCore* _appCore, AppData* _app) :
    CelestiaWatcher(*_appCore), app(_app)
{
}

void GtkWatcher::notifyChange(CelestiaCore*, int property)
{
	if (property & CelestiaCore::LabelFlagsChanged)
		resyncLabelActions(app);

	else if (property & CelestiaCore::RenderFlagsChanged)
	{
		resyncRenderActions(app);
		resyncOrbitActions(app);
		resyncStarStyleActions(app);
		resyncTextureResolutionActions(app);
	}

	else if (property & CelestiaCore::VerbosityLevelChanged)
		resyncVerbosityActions(app);

	else if (property & CelestiaCore::TimeZoneChanged)
		resyncTimeZoneAction(app);

	else if (property & CelestiaCore::AmbientLightChanged)
		resyncAmbientActions(app);

	/*
	else if (property & CelestiaCore::FaintestChanged) DEPRECATED?
	else if (property & CelestiaCore::HistoryChanged)
	*/

	else if (property == CelestiaCore::TextEnterModeChanged)
	{
		if (app->core->getTextEnterMode() != 0)
		{
			/* Grey-out the menu */
			gtk_widget_set_sensitive(app->mainMenu, FALSE);
			
			/* Disable any actions that will interfere in typing and
			   autocomplete */
			gtk_action_group_set_sensitive(app->agMain, FALSE);
			gtk_action_group_set_sensitive(app->agRender, FALSE);
			gtk_action_group_set_sensitive(app->agLabel, FALSE);
		}
		else
		{
			/* Set the menu normal */
			gtk_widget_set_sensitive(app->mainMenu, TRUE);
			
			/* Re-enable action groups */
			gtk_action_group_set_sensitive(app->agMain, TRUE);
			gtk_action_group_set_sensitive(app->agRender, TRUE);
			gtk_action_group_set_sensitive(app->agLabel, TRUE);
		}
	}
	
	else if (property & CelestiaCore::GalaxyLightGainChanged)
		resyncGalaxyGainActions(app);
}

/* END Watcher */


/* CALLBACK: Event "realize" on the main GL area. Things that go here are those
 *           that require the glArea to be set up. */
static void initRealize(GtkWidget* widget, AppData* app)
{
	GLenum glewErr = glewInit();
	   { 
	     if (GLEW_OK != glewErr)
		{
		GtkWidget *message;
			message = gtk_message_dialog_new(GTK_WINDOW(app->mainWindow),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			"Celestia was unable to initialize OpenGL extensions. Graphics quality will be reduced. Only Basic render path will be available.");
		gtk_dialog_run(GTK_DIALOG(message));
		gtk_widget_destroy(message);
		}
	}

	if (!app->core->initRenderer())
	{
		cerr << "Failed to initialize renderer.\n";
	}

	/* Read/Apply Settings */
	#ifdef GNOME
	applySettingsGConfMain(app, app->client);
	#else
	applySettingsFileMain(app, app->settingsFile);
	#endif /* GNOME */
	
	/* Synchronize all actions with core settings */
	resyncLabelActions(app);
	resyncRenderActions(app);
	resyncOrbitActions(app);
	resyncVerbosityActions(app);
	resyncAmbientActions(app);
	resyncStarStyleActions(app);
	
	/* If full-screen at startup, make it so. */
	if (app->fullScreen)
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(app->agMain, "FullScreen")), TRUE);

	/* If framerate limiting is off, set it so. */
	if (!app->renderer->getVideoSync())
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(app->agMain, "VideoSync")), FALSE);

	/* If URL at startup, make it so. */
	if (app->startURL != NULL)
		app->core->setStartURL(app->startURL);
	
	/* Set simulation time */
	app->core->start((double)time(NULL) / 86400.0 + (double)astro::Date(1970, 1, 1));
	updateTimeZone(app, app->showLocalTime);
	
	/* Setting time zone name not very useful, but makes space for "LT" status in
	 * the top-right corner. Set to some default. */
	app->core->setTimeZoneName("UTC");

	/* Set the cursor to a crosshair */
	gdk_window_set_cursor(widget->window, gdk_cursor_new(GDK_CROSSHAIR));
}


/* MAIN */
int main(int argc, char* argv[])
{
	/* Initialize the structure that holds the application's vitals. */
	AppData* app = g_new0(AppData, 1);

	/* Not ready to render yet. */
	app->bReady = FALSE;

	/* Initialize variables in the AppData structure. */
	app->lastX = 0;
	app->lastY = 0;
	app->showLocalTime = FALSE;
	app->fullScreen = FALSE;
	app->startURL = NULL;

	/* Watcher enables sending signals from inside of core */
	GtkWatcher* gtkWatcher;

	/* Command line option parsing */
	GError *error = NULL;
	GOptionContext* context = g_option_context_new("");
	g_option_context_add_main_entries(context, optionEntries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_parse(context, &argc, &argv, &error);

	if (error != NULL)
	{
		g_print("Error in command line options. Use --help for full list.\n");
		exit(1);
	}
	
	/* At this point, the argument count should be 1 or 2, with the lastX
	 * potentially being a cel:// URL. */
	
	/* If there's an argument left, assume it's a URL. This happens here
	 * because it's after the saved prefs are applied. The appCore gets
	 * initialized elsewhere. */
	if (argc > 1)
		app->startURL = argv[argc - 1];
	
	if (installDir == NULL)
		installDir = (gchar*)CONFIG_DATA_DIR;

	if (chdir(installDir) == -1)
		cerr << "Cannot chdir to '" << installDir << "', probably due to improper installation.\n";

	#ifdef GNOME
	/* GNOME Initialization */
	GnomeProgram *program;
	program = gnome_program_init("Celestia", VERSION, LIBGNOMEUI_MODULE,
	                             argc, argv, GNOME_PARAM_NONE);
	#else
	/* GTK-Only Initialization */
	gtk_init(&argc, &argv);
	#endif

	/* Turn on the splash screen */
	SplashData* ss = splashStart(app, !noSplash);
	splashSetText(ss, "Initializing...");

	SetDebugVerbosity(0);

	/* Force number displays into C locale. */
	setlocale(LC_NUMERIC, "C");
	setlocale(LC_ALL, "");

	#ifndef WIN32
	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
	#endif /* WIN32 */

	app->core = new CelestiaCore();
	if (app->core == NULL)
	{
		cerr << "Failed to initialize Celestia core.\n";
		return 1;
	}

	app->renderer = app->core->getRenderer();
	g_assert(app->renderer);
	
	/* Parse simulation arguments */
	string cf;
	if (configFile != NULL)
		cf = string(configFile);
	
	string* altConfig = (configFile != NULL) ? &cf : NULL;

	vector<string> configDirs;
	if (extrasDir != NULL)
	{
		/* Add each extrasDir to the vector */
		int i = 0;
		while (extrasDir[i] != NULL)
		{
			configDirs.push_back(extrasDir[i]);
			i++;
		}
	}

	/* Initialize the simulation */	
	if (!app->core->initSimulation(altConfig, &configDirs, ss->notifier))
		return 1;
	
	app->simulation = app->core->getSimulation();
	g_assert(app->simulation);
	
	#ifdef GNOME
	/* Create the main window (GNOME) */
	app->mainWindow = gnome_app_new("Celestia", "Celestia");
	#else
	/* Create the main window (GTK) */
	app->mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(app->mainWindow), "Celestia");
	#endif /* GNOME */

	/* Set pointer to AppData structure. This is for when a function is in a
	 * *real* bind to get at this structure. */
	g_object_set_data(G_OBJECT(app->mainWindow), "CelestiaData", app);
	
	GtkWidget* mainBox = GTK_WIDGET(gtk_vbox_new(FALSE, 0));
	gtk_container_set_border_width(GTK_CONTAINER(mainBox), 0);

	g_signal_connect(GTK_OBJECT(app->mainWindow), "destroy",
	                 G_CALLBACK(actionQuit), app);

	/* Initialize the OpenGL widget */
	gtk_gl_init (&argc, &argv);

	/* Configure OpenGL. Try double-buffered visual. */
	GdkGLConfig* glconfig = gdk_gl_config_new_by_mode(static_cast<GdkGLConfigMode>
	                                                  (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE));

	if (glconfig == NULL)
	{
		g_print("*** Cannot find the double-buffered visual.\n");
		g_print("*** Trying single-buffered visual.\n");

		/* Try single-buffered visual */
		glconfig = gdk_gl_config_new_by_mode(static_cast<GdkGLConfigMode>
		                                     (GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH));
		if (glconfig == NULL)
		{
			g_print ("*** No appropriate OpenGL-capable visual found.\n");
			exit(1);
		}
	}

	/* Initialize settings system */
	#ifdef GNOME
	initSettingsGConf(app);
	#else
	initSettingsFile(app);
	#endif /* GNOME */
	
	/* Create area to be used for OpenGL display */
	app->glArea = gtk_drawing_area_new();
	
	/* Set OpenGL-capability to the widget. */
	gtk_widget_set_gl_capability(app->glArea,
	                             glconfig,
	                             NULL,
	                             TRUE,
	                             GDK_GL_RGBA_TYPE);
	
	gtk_widget_set_events(GTK_WIDGET(app->glArea),
	                      GDK_EXPOSURE_MASK |
	                      GDK_KEY_PRESS_MASK |
	                      GDK_KEY_RELEASE_MASK |
	                      GDK_BUTTON_PRESS_MASK |
	                      GDK_BUTTON_RELEASE_MASK |
	                      GDK_POINTER_MOTION_MASK);

	/* Load settings the can be applied before the simulation is initialized */
	#ifdef GNOME
	applySettingsGConfPre(app, app->client);
	#else
	applySettingsFilePre(app, app->settingsFile);
	#endif /* GNOME */

	/* Full-Screen option from the command line (overrides above). */
	if (fullScreen)
		app->fullScreen = TRUE;

	/* Initialize handlers to all events in the glArea */
	initGLCallbacks(app);

	/* Handler than completes initialization when the glArea is realized */
	g_signal_connect(GTK_OBJECT(app->glArea), "realize",
	                 G_CALLBACK(initRealize), app);

	/* Create the main menu bar */
	createMainMenu(app->mainWindow, app);
	
	/* Initialize the context menu */
	initContext(app);

	/* Set context menu callback for the core */
	app->core->setContextMenuCallback(menuContext);
	
	#ifdef GNOME
	/* Set window contents (GNOME) */
	gnome_app_set_contents((GnomeApp *)app->mainWindow, GTK_WIDGET(mainBox));
	#else
	/* Set window contents (GTK) */
	gtk_container_add(GTK_CONTAINER(app->mainWindow), GTK_WIDGET(mainBox));
	#endif /* GNOME */

	gtk_box_pack_start(GTK_BOX(mainBox), app->mainMenu, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainBox), app->glArea, TRUE, TRUE, 0);

	gtk_window_set_default_icon_from_file("celestia-logo.png", NULL);

	/* Set focus to glArea widget */
	GTK_WIDGET_SET_FLAGS(app->glArea, GTK_CAN_FOCUS);
	gtk_widget_grab_focus(GTK_WIDGET(app->glArea));

	/* Initialize the Watcher */
	gtkWatcher = new GtkWatcher(app->core, app);

	/* Unload the splash screen */
	splashEnd(ss);

	gtk_widget_show_all(app->mainWindow);

	/* HACK: Now that window is drawn, set minimum window size */
	gtk_widget_set_size_request(app->glArea, 320, 240);

	#ifdef GNOME
	initSettingsGConfNotifiers(app);
	#endif /* GNOME */
	
	/* Set the ready flag */
	app->bReady = TRUE;
	
	/* Call Main GTK Loop */
	gtk_main();
    
	g_free(app);

	return 0;
}


#ifdef WIN32
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
	return main(__argc, __argv);
}
#endif /* WIN32 */
