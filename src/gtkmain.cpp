// gtkmain.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// GTk front end for Celestia.
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
#include "gl.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtkgl/gtkglarea.h>
#include <gnome.h>
#include "../config.h"
#include "celestia.h"
#include "vecmath.h"
#include "quaternion.h"
#include "util.h"
#include "timer.h"
#include "mathlib.h"
#include "astro.h"
#include "celestiacore.h"


char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;

// Timer info.
static double currentTime = 0.0;
static Timer* timer = NULL;

// Mouse motion tracking
static int lastX = 0;
static int lastY = 0;

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

static GtkItemFactory* menuItemFactory = NULL;

#if 0
static void SetRenderFlag(int flag, bool state)
{
    renderer->setRenderFlags((renderer->getRenderFlags() & ~flag) |
                             (state ? flag : 0));
}

static void SetLabelFlag(int flag, bool state)
{
    renderer->setLabelMode((renderer->getLabelMode() & ~flag) |
                           (state ? flag : 0));
}
#endif

enum
{
    Menu_ShowGalaxies        = 2001,
    Menu_ShowOrbits          = 2002,
    Menu_ShowConstellations  = 2003,
    Menu_ShowAtmospheres     = 2004,
    Menu_PlanetLabels        = 2005,
    Menu_StarLabels          = 2006,
    Menu_ConstellationLabels = 2007,
};

static void menuSelectSol()
{
    appCore->charEntered('H');
}

static void menuCenter()
{
    appCore->charEntered('C');
}

static void menuGoto()
{
    appCore->charEntered('G');
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
    // sim->setTimeScale(1.0);
}

static void menuReverse()
{
    appCore->charEntered('J');
}

static gint menuShowGalaxies(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowGalaxies, on);
    appCore->charEntered('U');
    return TRUE;
}

static gint menuShowOrbits(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowOrbits, on);
    appCore->charEntered('O');
    return TRUE;
}

static gint menuShowAtmospheres(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowCloudMaps, on);
    appCore->charEntered('I');
    return TRUE;
}

static gint menuShowConstellations(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowDiagrams, on);
    appCore->charEntered('/');
    return TRUE;
}

static gint menuStarLabels(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::StarLabels, on);
    appCore->charEntered('B');
    return TRUE;
}

static gint menuConstellationLabels(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::ConstellationLabels, on);
    appCore->charEntered('=');
    return TRUE;
}

static gint menuPlanetLabels(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::MajorPlanetLabels, on);
    appCore->charEntered('N');
    return TRUE;
}


static void menuMoreStars()
{
    appCore->charEntered(']');
}

static void menuLessStars()
{
    appCore->charEntered('[');
}

static void menuRunDemo()
{
    appCore->charEntered('D');
}

static void menuAbout()
{
    const gchar* authors[] = {
        "Chris Laurel <claurel@shatters.net>",
        NULL
    };
    GtkWidget* about;
    about = gnome_about_new("Celestia",
                            VERSION,
                            "(c) 2001 Chris Laurel",
                            authors,
                            "3D Space Simulation",
                            NULL);
    if (about == NULL)
        return;

    gtk_window_set_modal(GTK_WINDOW(about), TRUE);
    gtk_widget_show(about);
}


static void menuSelectObject()
{
    GtkWidget* dialog = gnome_dialog_new("Find Object",
                                         GNOME_STOCK_BUTTON_OK,
                                         GNOME_STOCK_BUTTON_CANCEL,
                                         NULL);
    if (dialog == NULL)
        return;

    GtkWidget* label = gtk_label_new("Enter object name:");
    if (label == NULL)
        return;
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG (dialog)->vbox),
                       label,
                       TRUE,
                       TRUE,
                       0);

    GtkWidget* entry = gtk_entry_new();
    if (entry == NULL)
        return;
    gtk_box_pack_start(GTK_BOX(GNOME_DIALOG (dialog)->vbox),
                       entry,
                       TRUE,
                       TRUE,
                       0);

    gtk_widget_show(label);
    gtk_widget_show(entry);

    gnome_dialog_close_hides(GNOME_DIALOG(dialog), true);
    // gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gint whichButton = gnome_dialog_run(GNOME_DIALOG(dialog));
    if (whichButton == 0)
    {
        gchar* name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (name != NULL)
        {
            Selection sel = appCore->getSimulation()->findObject(name);
            if (!sel.empty())
                appCore->getSimulation()->setSelection(sel);
        }
    }

    gnome_dialog_close(GNOME_DIALOG(dialog));
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

static gint TourGuideGoto(GtkWidget* w, gpointer data)
{
    Simulation* sim = appCore->getSimulation();
    if (selectedDest != NULL && sim != NULL)
    {
        Selection sel = sim->findObjectFromPath(selectedDest->target);
        if (!sel.empty())
        {
            sim->follow();
            sim->setSelection(sel);
            if (selectedDest->distance <= 0)
            {
                // Use the default distance
                sim->gotoSelection(5.0,
                                   Vec3f(0, 1, 0),
                                   astro::ObserverLocal);
            }
            else
            {
                sim->gotoSelection(5.0,
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
    GtkWidget* dialog = gnome_dialog_new("Tour Guide...",
                                         GNOME_STOCK_BUTTON_OK,
                                         NULL);
    if (dialog == NULL)
        return;

    GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
    if (hbox == NULL)
        return;

    GtkWidget* label = gtk_label_new("Select your destination:");
    if (label == NULL)
        return;
    gtk_box_pack_start(GTK_BOX(hbox),
                       label,
                       FALSE,
                       TRUE,
                       0);

    GtkWidget* menubox = gtk_option_menu_new();
    if (menubox == NULL)
        return;
    gtk_box_pack_start(GTK_BOX(hbox),
                       menubox,
                       FALSE,
                       TRUE,
                       0);

    GtkWidget* gotoButton = gtk_button_new_with_label("   Go To   ");
    if (gotoButton == NULL)
        return;
    gtk_box_pack_start(GTK_BOX(hbox),
                       gotoButton,
                       TRUE,
                       FALSE,
                       0);

    gtk_box_pack_start(GTK_BOX (GNOME_DIALOG (dialog)->vbox),
                       hbox,
                       FALSE,
                       TRUE,
                       0);

    gtk_widget_show(hbox);
    

    GtkWidget* descLabel = gtk_label_new("");
    if (descLabel == NULL)
        return;
    gtk_label_set_line_wrap(GTK_LABEL(descLabel), TRUE);
    gtk_label_set_justify(GTK_LABEL(descLabel), GTK_JUSTIFY_FILL);
    gtk_box_pack_start(GTK_BOX (GNOME_DIALOG (dialog)->vbox),
                       descLabel,
                       TRUE,
                       TRUE,
                       0);

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

    gtk_signal_connect(GTK_OBJECT(menu),
                       "selection-done",
                       GTK_SIGNAL_FUNC(TourGuideSelect),
                       GTK_OBJECT(descLabel));
    gtk_signal_connect(GTK_OBJECT(gotoButton),
                       "pressed",
                       GTK_SIGNAL_FUNC(TourGuideGoto),
                       NULL);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(menubox), menu);

    gtk_widget_set_usize(dialog, 400, 300);
    gtk_widget_show(label);
    gtk_widget_show(menubox);
    gtk_widget_show(descLabel);
    gtk_widget_show(gotoButton);
    // gtk_widget_set_usize(descLabel, 250, 150);

    gnome_dialog_run_and_close(GNOME_DIALOG(dialog));

    // gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    // gtk_widget_show(dialog);
}



static GtkItemFactoryEntry menuItems[] =
{
    { "/_File",  NULL,                      NULL,          0, "<Branch>" },
    { "/File/Quit", "<control>Q",           gtk_main_quit, 0, NULL },
    { "/_Navigation", NULL,                 NULL,          0, "<Branch>" },
    { "/Navigation/Select Sol", "H",        menuSelectSol, 0, NULL },
    { "/Navigation/Tour Guide", NULL,       menuTourGuide, 0, NULL },
    { "/Navigation/Select Object...", NULL, menuSelectObject, 0, NULL },
    { "/Navigation/sep1", NULL,             NULL,          0, "<Separator>" },
    { "/Navigation/Center Selection", "C",  menuCenter,    0, NULL },
    { "/Navigation/Goto Selection", "G",    menuGoto,      0, NULL },
    { "/Navigation/Follow Selection", "F",  menuFollow,    0, NULL },
    { "/_Time", NULL,                       NULL,          0, "<Branch>" },
    { "/Time/10x Faster", "L",              menuFaster,    0, NULL },
    { "/Time/10x Slower", "K",              menuSlower,    0, NULL },
    { "/Time/Pause", " ",               menuPause,     0, NULL },
    { "/Time/Real Time", NULL,              menuRealTime,  0, NULL },
    { "/Time/Reverse", "J",                 menuReverse,   0, NULL },
    { "/_Render", NULL,                     NULL,          0, "<Branch>" },
    { "/Render/Show Galaxies", "U",        NULL,          Menu_ShowGalaxies, "<ToggleItem>" },
    { "/Render/Show Atmospheres", "I",     NULL,          Menu_ShowAtmospheres, "<ToggleItem>" },
    { "/Render/Show Orbits", "O",          NULL,          Menu_ShowOrbits, "<ToggleItem>" },
    { "/Render/Show Constellations", "-",  NULL,          Menu_ShowConstellations, "<ToggleItem>" },
    { "/Render/sep1", NULL,                 NULL,          0, "<Separator>" },
    { "/Render/More Stars", "]",            menuMoreStars, 0, NULL },
    { "/Render/Fewer Stars", "[",           menuLessStars, 0, NULL },
    { "/Render/sep2", NULL,                 NULL,          0, "<Separator>" },
    { "/Render/Label Planets", "N",         NULL,          Menu_PlanetLabels, "<ToggleItem>" },
    { "/Render/Label Stars", "B",           NULL,          Menu_StarLabels, "<ToggleItem>" },
    { "/Render/LabelConstellations", "=",   NULL,          Menu_ConstellationLabels, "<ToggleItem>" },
    { "/_Help", NULL,                       NULL,          0, "<LastBranch>" },
    { "/Help/Run Demo", "D",                menuRunDemo,   0, NULL },  
    { "/Help/About", NULL,                  menuAbout,     0, NULL },
};


void setupCheckItem(GtkItemFactory* factory, int action, GtkSignalFunc func)
{
    GtkWidget* w = gtk_item_factory_get_widget_by_action(factory, action);
    if (w != NULL)
    {
        gtk_signal_connect(GTK_OBJECT(w), "toggled",
                           GTK_SIGNAL_FUNC(func),
                           NULL);
    }
}


void createMainMenu(GtkWidget* window, GtkWidget** menubar)
{
    gint nItems = sizeof(menuItems) / sizeof(menuItems[0]);

    GtkAccelGroup* accelGroup = gtk_accel_group_new();
    menuItemFactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
                                           "<main>",
                                           accelGroup);
    gtk_item_factory_create_items(menuItemFactory, nItems, menuItems, NULL);

    gtk_window_add_accel_group(GTK_WINDOW(window), accelGroup);
    if (menubar != NULL)
        *menubar = gtk_item_factory_get_widget(menuItemFactory, "<main>");

    setupCheckItem(menuItemFactory, Menu_ShowGalaxies,
                   GTK_SIGNAL_FUNC(menuShowGalaxies));
    setupCheckItem(menuItemFactory, Menu_ShowConstellations,
                   GTK_SIGNAL_FUNC(menuShowConstellations));
    setupCheckItem(menuItemFactory, Menu_ShowAtmospheres,
                   GTK_SIGNAL_FUNC(menuShowAtmospheres));
    setupCheckItem(menuItemFactory, Menu_ShowOrbits,
                   GTK_SIGNAL_FUNC(menuShowOrbits));
    setupCheckItem(menuItemFactory, Menu_PlanetLabels,
                   GTK_SIGNAL_FUNC(menuPlanetLabels));
    setupCheckItem(menuItemFactory, Menu_StarLabels,
                   GTK_SIGNAL_FUNC(menuStarLabels));
    setupCheckItem(menuItemFactory, Menu_ConstellationLabels,
                   GTK_SIGNAL_FUNC(menuConstellationLabels));
}


gint reshapeFunc(GtkWidget* widget, GdkEventConfigure* event)
{
    if (gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    {
        int w = widget->allocation.width;
        int h = widget->allocation.height;
        appCore->resize(w, h);
    }

    return TRUE;
}


bool bReady = false;


/*
 * Definition of Gtk callback functions
 */

gint initFunc(GtkWidget* widget)
{
    if (gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    {
        if (!appCore->initRenderer())
        {
            cerr << "Failed to initialize renderer.\n";
            return TRUE;
        }

        appCore->start((double) time(NULL) / 86400.0 + (double) astro::Date(1970, 1, 1));
    }
        
    return TRUE;
}

void Display(GtkWidget* widget)
{
    if (bReady)
    {
        appCore->draw();
        gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
    }
}


gint glarea_idle(void*)
{
    double lastTime = currentTime;
    currentTime = timer->getTime();
    double dt = currentTime - lastTime;

    appCore->tick(dt);
    Display(GTK_WIDGET(oglArea));

    return TRUE;
}


gint glarea_draw(GtkWidget* widget, GdkEventExpose* event)
{
    // Draw only the last expose
    if (event->count > 0)
        return TRUE;

    if (gtk_gl_area_make_current(GTK_GL_AREA(widget)))
        Display(widget);

    return TRUE;
}


gint glarea_motion_notify(GtkWidget* widget, GdkEventMotion* event)
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

    appCore->mouseMove(x - lastX, y - lastY, buttons);

    lastX = x;
    lastY = y;

    return TRUE;
}

gint glarea_button_press(GtkWidget* widget, GdkEventButton* event)
{
    if (event->button == 4)
    {
        appCore->mouseWheel(-1.0f, 0);
    }
    else if (event->button == 5)
    {
        appCore->mouseWheel(1.0f, 0);
    }
    else if (event->button <= 3)
    {
        lastX = (int) event->x;
        lastY = (int) event->y;
        if (event->button == 1)
            appCore->mouseButtonDown(event->x, event->y, CelestiaCore::LeftButton);
        else if (event->button == 2)
            appCore->mouseButtonDown(event->x, event->y, CelestiaCore::MiddleButton);
        else if (event->button == 3)
            appCore->mouseButtonDown(event->x, event->y, CelestiaCore::RightButton);
    }

    return TRUE;
}

gint glarea_button_release(GtkWidget* widget, GdkEventButton* event)
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
    }

    if (k >= 0)
    {
        if (down)
            appCore->keyDown(k);
        else
            appCore->keyUp(k);
        return true;
    }
    else
    {
        return false;
    }
}


gint glarea_key_press(GtkWidget* widget, GdkEventKey* event)
{
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_press_event");
    switch (event->keyval)
    {
    case GDK_Escape:
        appCore->charEntered('\033');
        break;

    default:
        if (!handleSpecialKey(event->keyval, true))
        {
            if (event->string != NULL)
            {
                char* s = event->string;

                while (*s != '\0')
                {
                    char c = *s++;
                    appCore->charEntered(c);
                }
            }
        }
    }

    return TRUE;
}


gint glarea_key_release(GtkWidget* widget, GdkEventKey* event)
{
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_release_event");
    return handleSpecialKey(event->keyval, false);
}


int main(int argc, char* argv[])
{
    // Say we're not ready to render yet.
    bReady = false;

    if (chdir(CONFIG_DATA_DIR) == -1)
    {
        cerr << "Cannot chdir to '" << CONFIG_DATA_DIR <<
            "', probably due to improper installation\n";
    }

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

    timer = CreateTimer();


    // Now initialize OpenGL and Gnome
    gnome_init("Celestia", "1.1.2", argc, argv);

    // Check if OpenGL is supported
    if (gdk_gl_query() == FALSE)
    {
        g_print("OpenGL not supported\n");
        return 0;
    }

    // Create the main window
    mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainWindow), AppName);
    gtk_container_set_border_width(GTK_CONTAINER(mainWindow), 1);

    mainBox = GTK_WIDGET(gtk_vbox_new(FALSE, 0));
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 0);

    gtk_signal_connect(GTK_OBJECT(mainWindow), "delete_event",
                       GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_quit_add_destroy(1, GTK_OBJECT(mainWindow));

    // Create the OpenGL widget
    oglArea = GTK_WIDGET(gtk_gl_area_new(oglAttributeList));
    gtk_widget_set_events(GTK_WIDGET(oglArea),
                          GDK_EXPOSURE_MASK |
                          GDK_KEY_PRESS_MASK |
                          GDK_KEY_RELEASE_MASK |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);

    // Connect signal handlers
    gtk_signal_connect(GTK_OBJECT(oglArea), "expose_event",
                       GTK_SIGNAL_FUNC(glarea_draw), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "configure_event",
                       GTK_SIGNAL_FUNC(reshapeFunc), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "realize",
                       GTK_SIGNAL_FUNC(initFunc), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "button_press_event",
                       GTK_SIGNAL_FUNC(glarea_button_press), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "button_release_event",
                       GTK_SIGNAL_FUNC(glarea_button_release), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "motion_notify_event",
                       GTK_SIGNAL_FUNC(glarea_motion_notify), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "key_press_event",
                       GTK_SIGNAL_FUNC(glarea_key_press), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "key_release_event",
                       GTK_SIGNAL_FUNC(glarea_key_release), NULL);

    // Set the minimum size
    // gtk_widget_set_usize(GTK_WIDGET(oglArea), 200, 150);
    // Set the default size
    gtk_gl_area_size(GTK_GL_AREA(oglArea), 640, 480);

    createMainMenu(mainWindow, &mainMenu);

    // gtk_container_add(GTK_CONTAINER(mainWindow), GTK_WIDGET(oglArea));
    gtk_container_add(GTK_CONTAINER(mainWindow), GTK_WIDGET(mainBox));
    gtk_box_pack_start(GTK_BOX(mainBox), mainMenu, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), oglArea, TRUE, TRUE, 0);
    gtk_widget_show(GTK_WIDGET(oglArea));
    gtk_widget_show(GTK_WIDGET(mainBox));
    gtk_widget_show(GTK_WIDGET(mainMenu));
    gtk_widget_show(GTK_WIDGET(mainWindow));

    // Set focus to oglArea widget
    GTK_WIDGET_SET_FLAGS(oglArea, GTK_CAN_FOCUS);
    gtk_widget_grab_focus(GTK_WIDGET(oglArea));

    gtk_idle_add(glarea_idle, NULL);

    bReady = true;

    gtk_main();

    return 0;
}
