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
#include <celengine/gl.h>
#include <celengine/glext.h>
#include <celengine/celestia.h>
#include <celengine/starbrowser.h>

#ifndef DEBUG
#  define G_DISABLE_ASSERT
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtkgl/gtkglarea.h>
#include <gnome.h>
#include "celmath/vecmath.h"
#include "celmath/quaternion.h"
#include "celmath/mathlib.h"
#include "celengine/astro.h"
#include "celutil/util.h"
#include "celutil/filetype.h"
#include "celutil/debug.h"
#include "imagecapture.h"
#include "celestiacore.h"
#include "celengine/simulation.h"


char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;
static Renderer* appRenderer = NULL;
static Simulation* appSim = NULL;

// Mouse motion tracking
static int lastX = 0;
static int lastY = 0;

typedef struct _checkFunc CheckFunc;
typedef int (*Callback)(int);

struct _checkFunc
{
  GtkCheckMenuItem *widget;
  char *path;
  Callback func;
  int active;
  int funcdata;
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

static GtkItemFactory* menuItemFactory = NULL;

#if 0
static void SetRenderFlag(int flag, bool state)
{
    appRenderer->setRenderFlags((appRenderer->getRenderFlags() & ~flag) |
                             (state ? flag : 0));
}

static void SetLabelFlag(int flag, bool state)
{
    appRenderer->setLabelMode((appRenderer->getLabelMode() & ~flag) |
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
    Menu_ShowClouds          = 2005,
    Menu_ShowCelestialSphere = 2006,
    Menu_ShowNightSideMaps   = 2007,
    Menu_MajorPlanetLabels   = 2008,
    Menu_MinorPlanetLabels   = 2009,
    Menu_StarLabels          = 2010,
    Menu_GalaxyLabels        = 2011,
    Menu_ConstellationLabels = 2012,
    Menu_PixelShaders        = 2013,
    Menu_VertexShaders       = 2014,
    Menu_ShowLocTime         = 2015,
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

static gint menuShowGalaxies(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowGalaxies, on);
    appCore->charEntered('U');
    return TRUE;
}

static gint menuShowOrbits(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowOrbits, on);
    appCore->charEntered('O');
    return TRUE;
}

static gint menuShowClouds(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowCloudMaps, on);
    appCore->charEntered('I');
    return TRUE;
}

static gint menuShowAtmospheres(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowAtmospheres, on);
    appCore->charEntered('\001'); //Ctrl+A
    return TRUE;
}

static gint menuPixelShaders(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowNightSideMaps, on);
    appCore->charEntered('\020'); //Ctrl+P
    return TRUE;
}

static gint menuVertexShaders(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowNightSideMaps, on);
    appCore->charEntered('\026'); //Ctrl+V
    return TRUE;
}

static gint menuShowNightSideMaps(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowNightSideMaps, on);
    appCore->charEntered('\014'); //Ctrl+L
    return TRUE;
}

static gint menuShowCelestialSphere(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowCelestialSphere, on);
    appCore->charEntered(';');
    return TRUE;
}

static gint menuShowConstellations(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetRenderFlag(Renderer::ShowDiagrams, on);
    appCore->charEntered('/');
    return TRUE;
}

static gint menuShowLocTime(GtkWidget* w, gpointer data)
{
    bool on = (GTK_CHECK_MENU_ITEM(w)->active != FALSE);
    if (on)
    {
	appCore->setTimeZoneBias(-timezone);
	appCore->setTimeZoneName(tzname[daylight?0:1]);
    }
    else
    {
	appCore->setTimeZoneBias(0);
	appCore->setTimeZoneName("UTC");
    }
    return TRUE;
}

static gint menuStarLabels(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::StarLabels, on);
    appCore->charEntered('B');
    return TRUE;
}

static gint menuGalaxyLabels(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::StarLabels, on);
    appCore->charEntered('E');
    return TRUE;
}

static gint menuConstellationLabels(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::ConstellationLabels, on);
    appCore->charEntered('=');
    return TRUE;
}

static gint menuMajorPlanetLabels(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::MajorPlanetLabels, on);
    appCore->charEntered('N');
    return TRUE;
}

static gint menuMinorPlanetLabels(GtkWidget* w, gpointer data)
{
    // bool on = (GTK_CHECK_MENU_ITEM(w)->active == 1);
    // SetLabelFlag(Renderer::MinorPlanetLabels, on);
    appCore->charEntered('M');
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

static void menuNoAmbient()
{
    appCore->getRenderer()->setAmbientLightLevel(0.0f);
}

static void menuLowAmbient()
{
    appCore->getRenderer()->setAmbientLightLevel(0.02f);
}

static void menuMedAmbient()
{
    appCore->getRenderer()->setAmbientLightLevel(0.1f);
}

static void menuHiAmbient()
{
    appCore->getRenderer()->setAmbientLightLevel(0.25f);
}

static void menuShowInfo()
{
    appCore->charEntered('V');
}

static void menuRunDemo()
{
    appCore->charEntered('D');
}

static void menuAbout()
{
    const gchar* authors[] = {
        "Chris Laurel <claurel@shatters.net>",
	"Deon Ramsey <miavir@furry.de>",
        "Clint Weisbrod <cweisbrod@adelphia.net>",
        NULL
    };
    const gchar* logo = gnome_pixmap_file("gnome-hello-logo.png");
    static GtkWidget* about;
    if (about != NULL) 
    {
	// Try to de-iconify and raise the dialog. 

	gdk_window_show(about->window);
	gdk_window_raise(about->window);
    }
    else
    {
	about = gnome_about_new("Celestia",
				VERSION,
				"(c) 2001-2002 Chris Laurel",
				authors,
				"3D Space Simulation",
				logo);
	if (about == NULL)
	    return;

	gtk_window_set_modal(GTK_WINDOW(about), TRUE);
	gnome_dialog_set_parent((GnomeDialog*) about, GTK_WINDOW(mainWindow));
	gtk_widget_show(about);
    }
}


static GtkWidget* fileSelector = NULL;
static gchar* captureFilename = NULL;

void storeCaptureFilename(GtkFileSelection* selector, gpointer)
{
    captureFilename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileSelector));
    if (captureFilename == NULL)
        return;

    string filename(captureFilename);

    // Get the dimensions of the current viewport
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    bool success = false;
    ContentType type = DetermineFileType(filename);
    if (type == Content_JPEG)
    {
        success = CaptureGLBufferToJPEG(filename,
                                        viewport[0], viewport[1],
                                        viewport[2], viewport[3]);
    }
    else if (type == Content_PNG)
    {
        success = CaptureGLBufferToPNG(string(filename),
                                       viewport[0], viewport[1],
                                       viewport[2], viewport[3]);
    }
    else
    {
        DPRINTF("Unknown file type for screen capture.\n");
    }

    if (!success)
    {
        GtkWidget* errBox = gnome_message_box_new("Error writing captured image.",
                                                  GNOME_MESSAGE_BOX_ERROR,
                                                  GNOME_STOCK_BUTTON_OK,
                                                  NULL);
        gtk_widget_show(errBox);
    }
}



static void menuCaptureImage()
{
    captureFilename = NULL;
    fileSelector = gtk_file_selection_new("Select capture file.");

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->ok_button),
                       "clicked",
                       GTK_SIGNAL_FUNC(storeCaptureFilename),
                       NULL);
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->ok_button),
                              "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(fileSelector));
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fileSelector)->cancel_button),
                              "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy),
                              GTK_OBJECT(fileSelector));

    gtk_widget_show(fileSelector);
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
    gnome_dialog_set_parent((GnomeDialog*) dialog, GTK_WINDOW(mainWindow));
    gnome_dialog_set_default((GnomeDialog*) dialog, GNOME_YES);
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


class GotoObjectDialog
{
public:
    GotoObjectDialog();
    ~GotoObjectDialog();
    bool init();

    GtkWidget* dialog;
    GtkWidget* nameEntry;
    GtkWidget* latEntry;
    GtkWidget* longEntry;
    GtkWidget* distEntry;
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


static gint GotoObject(GtkWidget* w, gpointer data)
{
    GotoObjectDialog* gotoObjectDlg = reinterpret_cast<GotoObjectDialog*>(data);
    if (gotoObjectDlg == NULL)
        return FALSE;

    gchar* objectName = gtk_entry_get_text(GTK_ENTRY(gotoObjectDlg->nameEntry));
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

    return TRUE;
}


GotoObjectDialog::GotoObjectDialog() :
    dialog(NULL),
    nameEntry(NULL),
    latEntry(NULL),
    longEntry(NULL),
    distEntry(NULL)
{
}

GotoObjectDialog::~GotoObjectDialog()
{
}


bool GotoObjectDialog::init()
{
    dialog = gnome_dialog_new("Goto Object",
			      GNOME_STOCK_BUTTON_CANCEL,
                              NULL);
    nameEntry = gtk_entry_new();
    latEntry = gtk_entry_new();
    longEntry = gtk_entry_new();
    distEntry = gtk_entry_new();
    
    if (dialog == NULL ||
        nameEntry == NULL ||
        latEntry == NULL ||
        longEntry == NULL ||
        distEntry == NULL)
    {
        // Potential memory leak here . . .
        return false;
    }

    GtkWidget* hbox = NULL;
    GtkWidget* label = NULL;

    // Object name label and entry
    hbox = gtk_hbox_new(FALSE, 6);
    if (hbox == NULL)
        return false;
    label = gtk_label_new("Object name:");
    if (label == NULL)
        return false;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), nameEntry, FALSE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(nameEntry);
    gtk_box_pack_start(GTK_BOX (GNOME_DIALOG (dialog)->vbox),
                       hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    // Latitude and longitude
    hbox = gtk_hbox_new(FALSE, 6);
    if (hbox == NULL)
        return false;
    label = gtk_label_new("Latitude:");
    if (label == NULL)
        return false;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), latEntry, FALSE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(latEntry);

    label = gtk_label_new("Longitude:");
    if (label == NULL)
        return false;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), longEntry, FALSE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(longEntry);
    gtk_box_pack_start(GTK_BOX (GNOME_DIALOG (dialog)->vbox),
                       hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    // Distance
    hbox = gtk_hbox_new(FALSE, 6);
    if (hbox == NULL)
        return false;
    label = gtk_label_new("Distance:");
    if (label == NULL)
        return false;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), distEntry, FALSE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(distEntry);
    gtk_box_pack_start(GTK_BOX (GNOME_DIALOG (dialog)->vbox),
                       hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    // Goto button
    GtkWidget* gotoButton = gtk_button_new_with_label("   Go To   ");
    if (gotoButton == NULL)
        return false;
    gtk_widget_show(gotoButton);
    gtk_box_pack_start(GTK_BOX (GNOME_DIALOG (dialog)->vbox),
                       gotoButton, FALSE, TRUE, 0);

    gtk_signal_connect(GTK_OBJECT(gotoButton),
                       "pressed",
                       GTK_SIGNAL_FUNC(GotoObject),
                       this);

    gnome_dialog_set_parent((GnomeDialog*) dialog, GTK_WINDOW(mainWindow));
    gnome_dialog_set_default((GnomeDialog*) dialog, GNOME_NO);
    gnome_dialog_run_and_close(GNOME_DIALOG(dialog));

    return true;
}


static void menuGotoObject()
{
    GotoObjectDialog* gotoObjectDlg = new GotoObjectDialog();

    gotoObjectDlg->init();
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

    gnome_dialog_set_parent((GnomeDialog*) dialog, GTK_WINDOW(mainWindow));
    gnome_dialog_set_default((GnomeDialog*) dialog, GNOME_YES);
    gnome_dialog_run_and_close(GNOME_DIALOG(dialog));

    // gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    // gtk_widget_show(dialog);
}


char *readFromFile(char *fname)
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
    if (*dialog != NULL) 
    {
	// Try to de-iconify and raise the dialog. 
	gdk_window_show((*dialog)->window);
	gdk_window_raise((*dialog)->window);
	gnome_dialog_run_and_close(GNOME_DIALOG(*dialog));
    }
    else
    {
	*dialog = gnome_dialog_new(title,
				   GNOME_STOCK_BUTTON_OK,
				   NULL);
	if (*dialog == NULL)
	    {
	    DPRINTF("Unable to open '%s' dialog!\n",title);
	    return;
	    }

	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (*dialog)->vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scrolled_window);

	GtkWidget *text = gtk_text_new (NULL, NULL);
	gtk_text_set_editable (GTK_TEXT (text), FALSE);
	gtk_text_set_word_wrap (GTK_TEXT (text), FALSE);
	gtk_text_set_line_wrap (GTK_TEXT (text), FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled_window), text);

	gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL, txt, -1);
	
	gtk_widget_show (text);

	gtk_widget_set_usize(*dialog, 500, 400); //Absolute Size, urghhh

	gnome_dialog_set_parent((GnomeDialog*) *dialog, GTK_WINDOW(mainWindow));
	gnome_dialog_set_default((GnomeDialog*) *dialog, GNOME_YES);
	gnome_dialog_close_hides((GnomeDialog*) *dialog, TRUE);
	gtk_window_set_modal(GTK_WINDOW(*dialog), FALSE);
	gnome_dialog_run_and_close(GNOME_DIALOG(*dialog));
    }
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



static char *sstitles[]=
{
    "Name",
    "Type"
};

static char *cstitles[]=
{
    " Name ",
    " Distance(LY) ",
    " App. Mag ",
    " Abs. Mag ",
    " Type "
};

static char *radioLabels[]=
{
    "Nearest",
    "Brightest(App)",
    "Brightest(Abs)",
    "With Planets"
};

static const char *tmp[5];

static GtkWidget *ctree=NULL;
static GtkWidget *clist=NULL;

void addPlanetarySystemToTree(const PlanetarySystem* sys, GtkCTreeNode *parent, int lev)
{
    GtkCTreeNode *child;
    for (int i = 0; i < sys->getSystemSize(); i++)
    {
	Body* world = sys->getBody(i);
	tmp[0] = g_strdup(world->getName().c_str());
        if (lev == 2)
        {
            if (world->getRadius() >= 1000.0)
                tmp[1]="Planet";
            else
                tmp[1]="Planetoid";
        }
        else
        {
            if (world->getRadius() > 0.1)
                tmp[1]="Moon";
            else
                tmp[1]="Satellite";
        }
	const PlanetarySystem* satellites = world->getSatellites();
	child=gtk_ctree_insert_node(GTK_CTREE(ctree), parent, NULL,
                                    (char**) tmp , 0 , NULL, NULL, NULL, NULL,
                                    (satellites ? FALSE : TRUE), TRUE);
        gtk_ctree_node_set_row_data(GTK_CTREE(ctree), child, world);
	gtk_ctree_collapse (GTK_CTREE(ctree), child);
	g_free((char*) tmp[0]);
	if (satellites != NULL)
            addPlanetarySystemToTree(satellites, child, lev + 1);
    }
}


static const Star *nearestStar;
static Selection browserSel;

static gint listSelect(GtkCList *list, gint row, gint column, gpointer dummy, gpointer dumm2)
{
    Star *selStar =(Star *)gtk_clist_get_row_data(list,row);
    if (selStar)
    {
        browserSel.select(selStar);
	return TRUE;
    }
    return FALSE;
}


static gint treeSelect(GtkCTree *tree, GtkCTreeNode *node, gint column, gpointer dummy)
{
    Body *body;
    if (column < 0)
    {
        /* This will happen when the tree attempts to autoselect a node
           as it is just being switched to, but is still empty */
        return FALSE;
    }
    if ((body=(Body *)gtk_ctree_node_get_row_data(tree,node)))
    {
        if (body == (Body *) nearestStar)
            browserSel.select((Star *) nearestStar);
        else
            browserSel.select(body);
	return TRUE;
    }
    DPRINTF("Unable to find body for this node.\n");
    return FALSE;
}


static gint selectBrowsed()
{
    if (!browserSel.empty())
    {
        appSim->setSelection(browserSel);
        return TRUE;
    }
    return FALSE;
}


static gint centerBrowsed()
{
    if (!selectBrowsed())
        return FALSE;
    appCore->charEntered('C');
    return TRUE;
}


static gint gotoBrowsed()
{
    if (!selectBrowsed())
        return FALSE;
    appCore->charEntered('G');
    return TRUE;
}


static gint buttonMake(GtkWidget *hbox, char *txt, GtkSignalFunc func, gpointer data)
{
    GtkWidget* button = gtk_button_new_with_label(txt);
    if (button == NULL)
    {
	DPRINTF("Unable to get GTK Elements.\n");
	return 1;
    }
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX (hbox),
                       button, TRUE, TRUE, 0);

    gtk_signal_connect(GTK_OBJECT(button),
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
    vector<const Star*> *stars = sbrowser.listStars(100);
    gtk_clist_freeze(GTK_CLIST(clist));
    for (unsigned int i = 0; i < currentLength; i++)
        gtk_clist_remove(GTK_CLIST(clist), 0);
    currentLength=(*stars).size();
    browserSel.select((Star *)(*stars)[0]);
    UniversalCoord ucPos = appSim->getObserver().getPosition();
    
    for (unsigned int i = 0; i < currentLength; i++)
    {
        char buf[20];
        const Star *star=(*stars)[i];
        tmp[0] = g_strdup((stardb->getStarName(*star)).c_str());

        sprintf(buf, " %.3f ", ucPos.distanceTo(star->getPosition()));
        tmp[1] = g_strdup(buf);

        Vec3f r = star->getPosition() - ucPos;
        sprintf(buf, " %.2f ", astro::absToAppMag(star->getAbsoluteMagnitude(),r.length()));
        tmp[2] = g_strdup(buf);

        sprintf(buf, " %.2f ", star->getAbsoluteMagnitude());
        tmp[3] = g_strdup(buf);

        star->getStellarClass().str(buf, sizeof buf);
        tmp[4] = g_strdup(buf);

        gint row = gtk_clist_append(GTK_CLIST(clist), (char**) tmp);
        gtk_clist_set_row_data(GTK_CLIST(clist), row, (gpointer) star);
        for (unsigned int j = 0; j < 5; j++)
            g_free((void*) tmp[j]);
    }
    gtk_clist_thaw(GTK_CLIST(clist));
    delete stars;
}


static int radioClicked(GtkButton *button, int pred)
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
    StarDatabase *stardb=appSim->getUniverse()->getStarCatalog();
    g_assert(stardb);
    gtk_clist_freeze(GTK_CLIST(ctree));
    gtk_ctree_post_recursive(GTK_CTREE(ctree),NULL,(GtkCTreeFunc)gtk_ctree_remove_node,NULL);
    if (solarSys != NULL)
	nearestStar=solarSys->getStar();
    else
        nearestStar=sbrowser.nearestStar();
    tmp[0] = const_cast<char*>(stardb->getStarName(*nearestStar).c_str());

    char buf[30];
    sprintf(buf, "%s Star", (nearestStar->getStellarClass().str().c_str()));
    tmp[1] = buf;

    GtkCTreeNode *top = gtk_ctree_insert_node (GTK_CTREE(ctree), NULL, NULL,
                                               (char**) tmp , 0 , NULL, NULL,
                                               NULL, NULL, FALSE, TRUE);
    gtk_ctree_node_set_row_data(GTK_CTREE(ctree), top, (gpointer)nearestStar);

    if (solarSys != NULL)
    {
	const PlanetarySystem* planets = solarSys->getPlanets();
	if (planets != NULL)
	    addPlanetarySystemToTree(planets, top, 2);
    }
    gtk_clist_thaw((GTK_CLIST(ctree)));
}


static const Star *tmpSel=NULL;

int nbookSwitch(GtkNotebook *nbook, gpointer dummy, gint page, gpointer dummy2)
{
    if (page == 1) // Page switch to the Solar System Browser 
    {
        sbrowser.refresh();
        loadNearestStarSystem();
        tmpSel=browserSel.star;
        browserSel.star=(Star *)nearestStar;
        browserSel.body=NULL;
    }
    else
        browserSel.select((Star *)tmpSel);
    return(TRUE);
}


static void menuBrowser()
{
    GtkWidget *browser= gnome_dialog_new("Celestial Browser",
				         GNOME_STOCK_BUTTON_OK,
				         NULL);
    browserSel.select((Star *)NULL);
    if (browser == NULL)
    {
	DPRINTF("Unable to open celestial browser dialog!\n");
	return;
    }
    
    GtkWidget *nbook = gtk_notebook_new();
    GtkWidget *label = gtk_label_new("Stars");
    GtkWidget  *vbox = gtk_vbox_new(FALSE, 3);
    GtkWidget  *hbox = gtk_hbox_new(FALSE, 3);
    GtkWidget *scrolled_win;
    if ((nbook==NULL) || (label==NULL) || (vbox==NULL) || (hbox==NULL))
    {
	DPRINTF("Unable to get GTK Elements.\n");
	return;
    }

    // Star System Browser
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), vbox, label);
    gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (nbook), vbox,
					TRUE, FALSE, GTK_PACK_START);
    gtk_widget_show (label);
    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    clist = gtk_clist_new_with_titles(5, cstitles);
    gtk_clist_column_titles_passive (GTK_CLIST(clist));
    gtk_clist_set_auto_sort (GTK_CLIST(clist), FALSE);
    gtk_clist_set_reorderable (GTK_CLIST(clist), FALSE);
    GtkWidget *align=gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    if ((align==NULL) || (clist==NULL) || (scrolled_win==NULL))
    {
	DPRINTF("Unable to get GTK Elements.\n");
	return;
    }
    gtk_clist_set_column_justification(GTK_CLIST(clist), 0, GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
    gtk_clist_set_column_justification(GTK_CLIST(clist), 1, GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
    gtk_clist_set_column_justification(GTK_CLIST(clist), 2, GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 2, TRUE);
    gtk_clist_set_column_justification(GTK_CLIST(clist), 3, GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 3, TRUE);
    gtk_clist_set_column_justification(GTK_CLIST(clist), 4, GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 4, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 5);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_ALWAYS);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (clist));
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
    gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_BROWSE);
    gtk_clist_set_shadow_type (GTK_CLIST(clist), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(align),GTK_WIDGET(hbox));
    sbrowser.setSimulation(appSim);
    addStars();
    tmpSel=browserSel.star;
    gtk_signal_connect(GTK_OBJECT(clist), "select-row",
		       GTK_SIGNAL_FUNC(listSelect), NULL);
    gtk_clist_select_row(GTK_CLIST(clist), 0, 0);
    
    GSList *group=NULL;
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, TRUE, 5);
    for(unsigned int i=0;i<4;i++)
    {
        GtkWidget *button=gtk_radio_button_new_with_label(group, radioLabels[i]);
        if (button==NULL)
        {
            DPRINTF("Unable to get GTK Elements.\n");
            return;
        }
        group=gtk_radio_button_group (GTK_RADIO_BUTTON (button));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),(i==0)?1:0);
        gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
        gtk_widget_show (button);
        gtk_signal_connect(GTK_OBJECT(button), "pressed",
                           GTK_SIGNAL_FUNC(radioClicked), (gpointer)i );
    }
    if (buttonMake(hbox, "   Refresh   ", (GtkSignalFunc)refreshBrowser, NULL))
        return;
    
    gtk_widget_show (clist);
    gtk_widget_show (align);
    gtk_widget_show (hbox);
    gtk_widget_show (scrolled_win);

    gtk_widget_show (label);
    gtk_widget_show (vbox);

    // Solar System Browser
    label = gtk_label_new("Solar System");
    vbox = gtk_vbox_new(TRUE, 3);
    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    if ((label==NULL) || (vbox==NULL) || (scrolled_win==NULL))
    {
	DPRINTF("Unable to get GTK Elements.\n");
	return;
    }
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), vbox, label);
    gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (nbook), vbox,
					TRUE, FALSE, GTK_PACK_START);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(nbook), TRUE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(nbook), FALSE);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (nbook), GTK_POS_TOP);
    gtk_container_set_border_width (GTK_CONTAINER (nbook), 10);
    gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (browser)->vbox), nbook, TRUE, TRUE, 0);

    gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 5);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_ALWAYS);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
    gtk_widget_show (scrolled_win);

    ctree = GTK_WIDGET (gtk_ctree_new_with_titles (2, 0, sstitles));
    gtk_clist_column_titles_passive (GTK_CLIST(ctree));
    gtk_clist_set_auto_sort (GTK_CLIST(ctree), FALSE);
    gtk_clist_set_reorderable (GTK_CLIST(ctree), FALSE);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (ctree));
    gtk_clist_set_column_auto_resize (GTK_CLIST (ctree), 0, TRUE);
    gtk_clist_set_column_width (GTK_CLIST (ctree), 1, 200);
    gtk_clist_set_selection_mode (GTK_CLIST (ctree), GTK_SELECTION_BROWSE);
    gtk_ctree_set_line_style (GTK_CTREE(ctree), GTK_CTREE_LINES_SOLID);
    gtk_ctree_set_expander_style (GTK_CTREE(ctree), GTK_CTREE_EXPANDER_SQUARE);
    gtk_widget_show (vbox);
    loadNearestStarSystem();
    gtk_signal_connect(GTK_OBJECT(ctree), "tree-select-row",
		       GTK_SIGNAL_FUNC(treeSelect), NULL);
    gtk_signal_connect(GTK_OBJECT(nbook), "switch-page",
		       GTK_SIGNAL_FUNC(nbookSwitch), NULL);
    gtk_widget_show (ctree);
    gtk_widget_show (nbook);

    // Common Buttons
    hbox = gtk_hbox_new(FALSE, 3);
    gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (browser)->vbox), hbox, FALSE, FALSE, 0);
    if (buttonMake(hbox, "   Select   ", (GtkSignalFunc)selectBrowsed, NULL))
        return;
    if (buttonMake(hbox, "   Center   ", (GtkSignalFunc)centerBrowsed, NULL))
        return;
    if (buttonMake(hbox, "   Go To   ", (GtkSignalFunc)gotoBrowsed, NULL))
        return;

    gtk_widget_show (hbox);

    gtk_widget_set_usize(browser, 500, 400); // Absolute Size, urghhh

    gnome_dialog_set_parent((GnomeDialog*) browser, GTK_WINDOW(mainWindow));
    gnome_dialog_set_default((GnomeDialog*) browser, GNOME_YES);
    gnome_dialog_close_hides((GnomeDialog*) browser, FALSE);
    gtk_window_set_modal(GTK_WINDOW(browser), FALSE);
    gnome_dialog_run_and_close(GNOME_DIALOG(browser));
    clist=NULL;
    ctree=NULL;
    browserSel.select((Star *)NULL);
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


char *timeOptions[]=
{
    "UTC",
    NULL,
    NULL
};

char *monthOptions[]=
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

static gint zonechosen(GtkMenuItem *item, int zone)
{
    tzone=zone;
    return TRUE;
}

static gint monthchosen(GtkMenuItem *item, int month)
{
    if (monthLoc)
	*monthLoc=month;
    return TRUE;
}


static void chooseOption(GtkWidget *hbox, char *str, char *choices[], int *val, char *sep, GtkSignalFunc chosen)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *label = gtk_label_new(str);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    GtkWidget *optmenu = gtk_option_menu_new ();
    GtkWidget *menu = gtk_menu_new ();
    GSList *group=NULL;
    for(unsigned int i=0; choices[i]; i++)
	{
	GtkWidget *menu_item = gtk_radio_menu_item_new_with_label (group, choices[i]);
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", chosen, (gpointer) (i+1));
	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));
	gtk_menu_append (GTK_MENU (menu), menu_item);
	if ((((int)i+1))==*val)
	    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
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
}


static void intSpin(GtkWidget *hbox, char *str, int min, int max, int *val, char *sep)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *label = gtk_label_new(str);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    GtkAdjustment *adj = (GtkAdjustment *) gtk_adjustment_new ((float)*val, (float) min, (float) max,
							      1.0, 5.0, 0.0);
    GtkWidget *spinner = gtk_spin_button_new (adj, 1.0, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (spinner), TRUE);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
    gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinner),
                                     GTK_SHADOW_IN);
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
    gtk_signal_connect(GTK_OBJECT(adj), "value-changed",
		       GTK_SIGNAL_FUNC(intAdjChanged), val);
}


static void menuSetTime()
{
    int second;
    GtkWidget *stimedialog = gnome_dialog_new("Set Time",
			                      GNOME_STOCK_BUTTON_OK,
					      "Set Current Time",
			                      GNOME_STOCK_BUTTON_CANCEL,
			                      NULL);
    tzone=1;
    if (appCore->getTimeZoneBias())
	tzone=2;
	
    if (stimedialog == NULL)
    {
	DPRINTF("Unable to open 'Set Time' dialog!\n");
	return;
    }

    GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
    GtkWidget *frame = gtk_frame_new("Time");
    GtkWidget *align=gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    
    if ((hbox == NULL) || (frame == NULL) || (align == NULL))
    {
	DPRINTF("Unable to get GTK Elements.\n");
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
    gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (stimedialog)->vbox), frame, FALSE, FALSE, 0);
    intSpin(hbox, "Hour", 1, 24, &date.hour, ":");
    intSpin(hbox, "Minute", 1, 60, &date.minute, ":");
    second=(int)date.seconds;
    intSpin(hbox, "Second", 1, 60, &second, "  ");
    chooseOption(hbox, "Timzone", timeOptions, &tzone, NULL, GTK_SIGNAL_FUNC(zonechosen));
    gtk_widget_show_all(hbox);
    hbox = gtk_hbox_new(FALSE, 6);
    frame = gtk_frame_new("Date");
    align=gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 7);
    
    if ((hbox == NULL) || (frame == NULL) || (align == NULL))
    {
	DPRINTF("Unable to get GTK Elements.\n");
	return;
    }
    chooseOption(hbox,"Month", monthOptions, &date.month, " ", GTK_SIGNAL_FUNC(monthchosen));
    intSpin(hbox,"Day", 1, 31, &date.day, ",");
    intSpin(hbox,"Year", -9999, 9999, &date.year, " "); /* (Hopefully,
			    noone will need to go beyond these :-) */
    gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (stimedialog)->vbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(align),GTK_WIDGET(hbox));
    gtk_container_add(GTK_CONTAINER(frame),GTK_WIDGET(align));
    gtk_widget_show(align);
    gtk_widget_show(frame);
    gtk_widget_show_all(hbox);

    gnome_dialog_set_parent((GnomeDialog*) stimedialog, GTK_WINDOW(mainWindow));
    gnome_dialog_set_default((GnomeDialog*) stimedialog, GNOME_YES);
    gnome_dialog_close_hides((GnomeDialog*) stimedialog, FALSE);
    gtk_window_set_modal(GTK_WINDOW(stimedialog), FALSE);
    gint button=gnome_dialog_run_and_close(GNOME_DIALOG(stimedialog));
    monthLoc=NULL;
    if (button==1)  // Set current time and exit.
    {
	time_t curtime=time(NULL);
	appSim->setTime((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
	appSim->update(0.0);
    }
    else if (button==0)  // Set entered time and exit
    {
	appSim->setTime((double) date - ((tzone==1) ? 0 : astro::secondsToJulianDate(appCore->getTimeZoneBias())));
	appSim->update(0.0);
    }
}



static GtkItemFactoryEntry menuItems[] =
{
    { "/_File",  NULL,                      NULL,          0, "<Branch>" },
    { "/File/Capture Image...", "F10",      menuCaptureImage,0, NULL },
    { "/File/Quit", "<control>Q",           gtk_main_quit, 0, NULL },
    { "/_Navigation", NULL,                 NULL,          0, "<Branch>" },
    { "/Navigation/Select Sol", "H",        menuSelectSol, 0, NULL },
    { "/Navigation/Tour Guide", NULL,       menuTourGuide, 0, NULL },
    { "/Navigation/Select Object...", NULL, menuSelectObject, 0, NULL },
    { "/Navigation/Goto Object...", NULL,   menuGotoObject,0, NULL },
    { "/Navigation/sep1", NULL,             NULL,          0, "<Separator>" },
    { "/Navigation/Center Selection", "C",  menuCenter,    0, NULL },
    { "/Navigation/Goto Selection", "G",    menuGoto,      0, NULL },
    { "/Navigation/Follow Selection", "F",  menuFollow,    0, NULL },
    { "/Navigation/Sync Orbit", "Y",        menuSync,      0, NULL },
    { "/Navigation/Track Selection", "T",   menuTrack,     0, NULL },
    { "/Navigation/sep2", NULL,             NULL,          0, "<Separator>" },
    { "/Navigation/Celestial Browser", NULL,menuBrowser,   0, NULL },
    { "/_Time", NULL,                       NULL,          0, "<Branch>" },
    { "/Time/10x Faster", "L",              menuFaster,    0, NULL },
    { "/Time/10x Slower", "K",              menuSlower,    0, NULL },
    { "/Time/Pause", "space",               menuPause,     0, NULL },
    { "/Time/Real Time", "backslash",       menuRealTime,  0, NULL },
    { "/Time/Reverse", "J",                 menuReverse,   0, NULL },
    { "/Time/Set Time", NULL,               menuSetTime,   0, NULL },
    { "/Time/sep1", NULL,                   NULL,          0, "<Separator>" },
    { "/Time/Show Local Time", "I",         NULL,          Menu_ShowLocTime, "<ToggleItem>" },
    { "/_Render", NULL,                     NULL,          0, "<Branch>" },
    { "/Render/Show Galaxies", "U",         NULL,          Menu_ShowGalaxies, "<ToggleItem>" },
    { "/Render/Show Atmospheres", "<control>A",NULL,       Menu_ShowAtmospheres, "<ToggleItem>" },
    { "/Render/Show Clouds", "I",           NULL,          Menu_ShowClouds, "<ToggleItem>" },
    { "/Render/Show Orbits", "O",           NULL,          Menu_ShowOrbits, "<ToggleItem>" },
    { "/Render/Show Constellations", "slash",NULL,         Menu_ShowConstellations, "<ToggleItem>" },
    { "/Render/Show Coordinate Sphere", "semicolon",NULL,  Menu_ShowCelestialSphere, "<ToggleItem>" },
    { "/Render/Show Night Side Lights", "<control>L",NULL, Menu_ShowNightSideMaps, "<ToggleItem>" },
    { "/Render/sep1", NULL,                 NULL,          0, "<Separator>" },
    { "/Render/More Stars Visible", "bracketleft",  menuMoreStars, 0, NULL },
    { "/Render/Fewer Stars Visible", "bracketright", menuLessStars, 0, NULL },
    { "/Render/sep2", NULL,                 NULL,          0, "<Separator>" },
    { "/Render/Label Major Planets", "N",   NULL,          Menu_MajorPlanetLabels, "<ToggleItem>" },
    { "/Render/Label Minor Planets", "M",   NULL,          Menu_MinorPlanetLabels, "<ToggleItem>" },
    { "/Render/Label Stars", "B",           NULL,          Menu_StarLabels, "<ToggleItem>" },
    { "/Render/Label Galaxies", "E",        NULL,          Menu_GalaxyLabels, "<ToggleItem>" },
    { "/Render/Label Constellations", "equal",NULL,        Menu_ConstellationLabels, "<ToggleItem>" },
    { "/Render/Show Info Text", "V",        menuShowInfo,  0, NULL },
    { "/Render/sep3", NULL,                 NULL,          0, "<Separator>" },
    { "/Render/_Ambient", NULL,             NULL,          0, "<Branch>" },
    { "/Render/Ambient/None", NULL,         menuNoAmbient, 0, NULL },
    { "/Render/Ambient/Low", NULL,          menuLowAmbient,0, NULL },
    { "/Render/Ambient/Medium", NULL,       menuMedAmbient,0, NULL },
    { "/Render/Ambient/High", NULL,         menuHiAmbient, 0, NULL },
    { "/_Help", NULL,                       NULL,          0, "<LastBranch>" },
    { "/Help/Run Demo", "D",                menuRunDemo,   0, NULL },  
    { "/Help/sep1", NULL,                   NULL,          0, "<Separator>" },
    { "/Help/Controls", NULL,       	    menuControls,  0, NULL },
    { "/Help/OpenGL Info", NULL,            menuOpenGL,    0, NULL },
    { "/Help/License", NULL,       	    menuLicense,   0, NULL },
    { "/Help/sep2", NULL,                   NULL,          0, "<Separator>" },
    { "/Help/About", NULL,                  menuAbout,     0, NULL },
};


static GtkItemFactoryEntry optMenuItems[] =
{
    { "/Render/Use Pixel Shaders", "<control>P", NULL,     Menu_PixelShaders, "<ToggleItem>" },
    { "/Render/Use Vertex Shaders", "<control>V", NULL,    Menu_PixelShaders, "<ToggleItem>" },
};


int checkLocalTime(int dummy)
{
	return(appCore->getTimeZoneBias()!=0);
}


int checkPixelShaders(int dummy)
{
	return(appRenderer->getFragmentShaderEnabled());
}


int checkVertexShaders(int dummy)
{
	return(appRenderer->getVertexShaderEnabled());
}


int checkShowGalaxies(int dummy)
{
	return((appRenderer->getRenderFlags() & Renderer::ShowGalaxies) == Renderer::ShowGalaxies);
}


int checkRenderFlag(int flag)
{
	return((appRenderer->getRenderFlags() & flag) == flag);
}


int checkLabelFlag(int flag)
{
	return((appRenderer->getLabelMode() & flag) == flag);
}


static CheckFunc checks[] =
{
    { NULL,	"/Time/Show Local Time",	checkLocalTime,		1, 0 },
    { NULL,	"/Render/Use Pixel Shaders",	checkPixelShaders,	0, 0 },
    { NULL,	"/Render/Use Vertex Shaders",	checkVertexShaders,	0, 0 },
    { NULL,	"/Render/Show Galaxies",	checkRenderFlag,	1, Renderer::ShowGalaxies },
    { NULL,	"/Render/Show Atmospheres",	checkRenderFlag,	1, Renderer::ShowAtmospheres },
    { NULL,	"/Render/Show Clouds",		checkRenderFlag,	1, Renderer::ShowCloudMaps },
    { NULL,	"/Render/Show Orbits",		checkRenderFlag,	1, Renderer::ShowOrbits },
    { NULL,	"/Render/Show Constellations",	checkRenderFlag,	1, Renderer::ShowDiagrams },
    { NULL,	"/Render/Show Coordinate Sphere",checkRenderFlag,	1, Renderer::ShowCelestialSphere },
    { NULL,	"/Render/Show Night Side Lights",checkRenderFlag,	1, Renderer::ShowNightMaps },
    { NULL,	"/Render/Label Major Planets",	checkLabelFlag,		1, Renderer::MajorPlanetLabels },
    { NULL,	"/Render/Label Minor Planets",	checkLabelFlag,		1, Renderer::MinorPlanetLabels },
    { NULL,	"/Render/Label Stars",		checkLabelFlag,		1, Renderer::StarLabels },
    { NULL,	"/Render/Label Galaxies",	checkLabelFlag,		1, Renderer::GalaxyLabels },
    { NULL,	"/Render/Label Constellations",	checkRenderFlag,	1, Renderer::ConstellationLabels },
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
    gtk_accel_group_attach (accelGroup, GTK_OBJECT (window));
    gtk_item_factory_create_items(menuItemFactory, nItems, menuItems, NULL);
    appRenderer=appCore->getRenderer();
    g_assert(appRenderer);
    appSim = appCore->getSimulation();
    g_assert(appSim);

    if (appRenderer->fragmentShaderSupported())
	{
	gtk_item_factory_create_item(menuItemFactory, &optMenuItems[0], NULL, 1);
	checks[1].active=1;
	}
    if (appRenderer->vertexShaderSupported())
	{
	gtk_item_factory_create_item(menuItemFactory, &optMenuItems[1], NULL, 1);
	checks[2].active=1;
	}

    if (menubar != NULL)
        *menubar = gtk_item_factory_get_widget(menuItemFactory, "<main>");

    setupCheckItem(menuItemFactory, Menu_ShowGalaxies,
                   GTK_SIGNAL_FUNC(menuShowGalaxies));
    setupCheckItem(menuItemFactory, Menu_ShowConstellations,
                   GTK_SIGNAL_FUNC(menuShowConstellations));
    setupCheckItem(menuItemFactory, Menu_ShowAtmospheres,
                   GTK_SIGNAL_FUNC(menuShowAtmospheres));
    setupCheckItem(menuItemFactory, Menu_ShowClouds,
                   GTK_SIGNAL_FUNC(menuShowClouds));
    setupCheckItem(menuItemFactory, Menu_ShowNightSideMaps,
                   GTK_SIGNAL_FUNC(menuShowNightSideMaps));
    setupCheckItem(menuItemFactory, Menu_ShowOrbits,
                   GTK_SIGNAL_FUNC(menuShowOrbits));
    setupCheckItem(menuItemFactory, Menu_MajorPlanetLabels,
                   GTK_SIGNAL_FUNC(menuMajorPlanetLabels));
    setupCheckItem(menuItemFactory, Menu_MinorPlanetLabels,
                   GTK_SIGNAL_FUNC(menuMinorPlanetLabels));
    setupCheckItem(menuItemFactory, Menu_StarLabels,
                   GTK_SIGNAL_FUNC(menuStarLabels));
    setupCheckItem(menuItemFactory, Menu_GalaxyLabels,
                   GTK_SIGNAL_FUNC(menuGalaxyLabels));
    setupCheckItem(menuItemFactory, Menu_ConstellationLabels,
                   GTK_SIGNAL_FUNC(menuConstellationLabels));
    setupCheckItem(menuItemFactory, Menu_ShowCelestialSphere,
                   GTK_SIGNAL_FUNC(menuShowCelestialSphere));
    setupCheckItem(menuItemFactory, Menu_ShowLocTime,
                   GTK_SIGNAL_FUNC(menuShowLocTime));
    if (appRenderer->fragmentShaderSupported())
	setupCheckItem(menuItemFactory, Menu_PixelShaders,
		       GTK_SIGNAL_FUNC(menuPixelShaders));
    if (appRenderer->vertexShaderSupported())
	setupCheckItem(menuItemFactory, Menu_VertexShaders,
		       GTK_SIGNAL_FUNC(menuVertexShaders));
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

	time_t curtime=time(NULL);
        appCore->start((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
	localtime(&curtime); /* Only doing this to set timezone as a side
			       effect*/
	appCore->setTimeZoneBias(-timezone);
	appCore->setTimeZoneName(tzname[daylight?0:1]);
	timeOptions[1]=tzname[daylight?0:1];
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
    appCore->tick();
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
    case GDK_F7:
        k = CelestiaCore::Key_F7;
        break;
    case GDK_F10:
        if (down)
            menuCaptureImage();
        break;
    case GDK_KP_0:
        k = CelestiaCore::Key_NumPad0;
        break;
    case GDK_KP_1:
        k = CelestiaCore::Key_NumPad1;
        break;
    case GDK_KP_2:
        k = CelestiaCore::Key_NumPad2;
        break;
    case GDK_KP_3:
        k = CelestiaCore::Key_NumPad3;
        break;
    case GDK_KP_4:
        k = CelestiaCore::Key_NumPad4;
        break;
    case GDK_KP_5:
        k = CelestiaCore::Key_NumPad5;
        break;
    case GDK_KP_6:
        k = CelestiaCore::Key_NumPad6;
        break;
    case GDK_KP_7:
        k = CelestiaCore::Key_NumPad7;
        break;
    case GDK_KP_8:
        k = CelestiaCore::Key_NumPad8;
        break;
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


gint glarea_key_press(GtkWidget* widget, GdkEventKey* event)
{
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_press_event");
    switch (event->keyval)
    {
    case GDK_Escape:
        appCore->charEntered('\033');
        break;

    case 'q':
    case 'Q':
        if(event->state & GDK_CONTROL_MASK)
            {  /* Why isn't Ctrl-Q sending char 21 ? I have no idea, but as
                  long as it isn't, this will work. */
            gtk_main_quit();
            }
        // Intentional fallthrough if *not* Ctrl-Q

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


    // Now initialize OpenGL and Gnome
    gnome_init("Celestia", VERSION, argc, argv);

    // Check if OpenGL is supported
    if (gdk_gl_query() == FALSE)
    {
        g_print("OpenGL not supported\n");
        return 0;
    }

    // Create the main window
    mainWindow=gnome_app_new("Celestia",AppName);
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
    gnome_app_set_contents((GnomeApp *)mainWindow, GTK_WIDGET(mainBox));
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

    g_assert(menuItemFactory);
    // Check all the toggle items that they are set on/off correctly
    int i = sizeof(checks) / sizeof(checks[0]);
    for(CheckFunc *cfunc=&checks[--i];i>=0;cfunc=&checks[--i])
    {
	if (!cfunc->active)
	    continue;
	g_assert(cfunc->path);
	cfunc->widget=GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(menuItemFactory, cfunc->path));
	if (!cfunc->widget)
	{
	    cfunc->active=0;
	    DPRINTF("Menu item %s status checking deactivated due to being unable to find it!", cfunc->path);
	    continue;
	}
	g_assert(cfunc->func);
	int res=(*cfunc->func)(cfunc->funcdata);
	if (res)
	{
    	    if (cfunc->widget->active == FALSE)
		{
		// Change state of widget *without* causing a message to be
		// sent (which would change the state again).
		gtk_widget_hide(GTK_WIDGET(cfunc->widget));
		cfunc->widget->active=TRUE;
		gtk_widget_show(GTK_WIDGET(cfunc->widget));
		}
	}
    }
    gtk_main();

    return 0;
}
