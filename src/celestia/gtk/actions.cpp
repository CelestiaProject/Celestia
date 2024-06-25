/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: actions.cpp,v 1.17 2008-01-25 01:05:14 suwalski Exp $
 */

#include <config.h>
#include <array>
#include <cstring>
#include <fstream>
#include <tuple>
#include <gtk/gtk.h>
#include <fmt/format.h>

#include <celengine/body.h>
#include <celengine/glsupport.h>
#include <celengine/simulation.h>
#include <celengine/render.h>
#include <celestia/celestiacore.h>
#include <celestia/configfile.h>
#include <celestia/helper.h>
#include <celestia/url.h>
#include <celcompat/charconv.h>
#include <celutil/filetype.h>
#include <celutil/gettext.h>
#ifdef USE_FFMPEG
#include <celestia/ffmpegcapture.h>
#endif

#include "actions.h"
#include "common.h"
#include "dialog-eclipse.h"
#include "dialog-goto.h"
#include "dialog-options.h"
#include "dialog-solar.h"
#include "dialog-star.h"
#include "dialog-time.h"
#include "dialog-tour.h"

#include "settings-file.h"

namespace celestia::gtk
{

namespace
{

/* Script opening helper called by actionOpenScript() */
void
openScript(const char* filename, AppData* app)
{
    /* Modified From Win32 HandleOpenScript */
    if (filename)
    {
        /* If you got here, a path and file has been specified.
         * filename contains full path to specified file. */
        app->core->runScript(filename);
    }
}

/* Image capturing helper called by actionCaptureImage() */
void
captureImage(const char* filename, AppData* app)
{
    ContentType type = DetermineFileType(filename);
    if (type != ContentType::JPEG && type != ContentType::PNG)
    {
        GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(app->mainWindow),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   _("Please use a name ending in '.jpg' or '.png'."));
        gtk_dialog_run(GTK_DIALOG(errBox));
        gtk_widget_destroy(errBox);
        return;
    }

    if (!app->core->saveScreenShot(filename))
    {
        GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(app->mainWindow),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   _("Error writing captured image."));
        gtk_dialog_run(GTK_DIALOG(errBox));
        gtk_widget_destroy(errBox);
    }
}

/* Image capturing helper called by actionCaptureImage() */
#ifdef USE_FFMPEG
void
captureMovie(const char* filename, const int resolution[], float fps,
             AVCodecID codec, float bitrate, AppData* app)
{
    auto* movieCapture = new FFMPEGCapture(app->renderer);
    movieCapture->setVideoCodec(codec);
    movieCapture->setBitRate(bitrate);
    if (codec == AV_CODEC_ID_H264)
        movieCapture->setEncoderOptions(app->core->getConfig()->x264EncoderOptions);
    else
        movieCapture->setEncoderOptions(app->core->getConfig()->ffvhEncoderOptions);

    bool success = movieCapture->start(filename, resolution[0], resolution[1], fps);
    if (success)
    {
        app->core->initMovieCapture(movieCapture);
    }
    else
    {
        delete movieCapture;

        GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(app->mainWindow),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Error initializing movie capture.");
        gtk_dialog_run(GTK_DIALOG(errBox));
        gtk_widget_destroy(errBox);
    }
}
#endif

/* Runs a dialog that displays text; should be replaced at some point with
   a more elegant solution. */
void
textInfoDialog(const char *txt, const char *title, AppData* app)
{
    /* I would use a gnome_message_box dialog for this, except they don't seem
     * to notice that the texts are so big that they create huge windows, and
     * would work better with a scrolled window. Deon */
    GtkWidget* dialog = gtk_dialog_new_with_buttons(title,
                                                    GTK_WINDOW(app->mainWindow),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                    NULL);

    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_window);

    GtkWidget *text = gtk_label_new(txt);
    gtk_widget_modify_font(text, pango_font_description_from_string("mono"));
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), GTK_WIDGET(text));
    gtk_widget_show(text);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400); /* Absolute Size, urghhh */
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* Calculates and sets the render-flag int */
void
setRenderFlag(AppData* a, std::uint64_t flag, gboolean state)
{
    std::uint64_t rf = (a->renderer->getRenderFlags() & ~flag) | (state ? flag : 0);
    a->renderer->setRenderFlags(rf);
}

/* Calculates and sets the orbit-mask int */
void
setOrbitMask(AppData* a, BodyClassification mask, gboolean state)
{
    BodyClassification om = (a->renderer->getOrbitMask() & ~mask) | (state ? mask : BodyClassification::EmptyMask);
    a->renderer->setOrbitMask(om);
}

/* Calculates and sets the label-mode int */
void
setLabelMode(AppData* a, int mode, gboolean state)
{
    int lm = (a->renderer->getLabelMode() & ~mode) | (state ? mode : 0);
    a->renderer->setLabelMode(lm);
}

#ifdef USE_FFMPEG
using MovieSize = std::array<int, 2>;

constexpr std::array MovieSizes
{
    MovieSize{ 160,  120  },
    MovieSize{ 320,  240  },
    MovieSize{ 640,  480  },
    MovieSize{ 720,  480  },
    MovieSize{ 720,  576  },
    MovieSize{ 1024, 768  },
    MovieSize{ 1280, 720  },
    MovieSize{ 1920, 1080 }
};

constexpr std::array MovieFramerates{ 15.0f, 23.976f, 24.0f, 25.0f, 29.97f, 30.0f, 60.0f };

struct MovieCodec
{
    AVCodecID   codecId;
    const char *codecDesc;
};

constexpr std::array<MovieCodec, 2> MovieCodecs
{
    MovieCodec{ AV_CODEC_ID_FFVHUFF, N_("Lossless")      },
    MovieCodec{ AV_CODEC_ID_H264,    N_("Lossy (H.264)") }
};

void
insert_text_event(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data)
{
    for (int i = 0; i < length; i++)
    {
        if (!std::isdigit(static_cast<unsigned char>(text[i])))
        {
            g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
            return;
        }
    }
}
#endif

} // end unnamed namespace

/* File -> Copy URL */
void
actionCopyURL(GtkAction*, AppData* app)
{
    GtkClipboard* cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    CelestiaState appState(app->core);
    appState.captureState();
    Url url(appState);
    gtk_clipboard_set_text(cb, url.getAsString().c_str(), -1);
}


/* File -> Open URL */
void
actionOpenURL(GtkAction*, AppData* app)
{
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Enter cel:// URL",
                                                    GTK_WINDOW(app->mainWindow),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                                    NULL);

    /* Create a new entry box with default text, all selected */
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 80);
    gtk_entry_set_text(GTK_ENTRY(entry), "cel://");
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);

    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
    gtk_box_pack_start(GTK_BOX(content_area), entry, TRUE, TRUE, CELSPACING);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        app->core->goToUrl(gtk_entry_get_text(GTK_ENTRY(entry)));

    gtk_widget_destroy(dialog);
}


/* File -> Open Script... */
void
actionOpenScript(GtkAction*, AppData* app)
{
    GtkWidget* fs = gtk_file_chooser_dialog_new("Open Script.",
                                                GTK_WINDOW(app->mainWindow),
                                                GTK_FILE_CHOOSER_ACTION_OPEN,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                NULL);

    #if GTK_CHECK_VERSION(2, 7, 0)
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fs), TRUE);
    #endif /* GTK_CHECK_VERSION */

    gtk_dialog_set_default_response(GTK_DIALOG(fs), GTK_RESPONSE_ACCEPT);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fs), g_get_home_dir());

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Celestia Scripts");

    gtk_file_filter_add_pattern(filter, "*.cel");

    #ifdef CELX
    gtk_file_filter_add_pattern(filter, "*.celx");
    gtk_file_filter_add_pattern(filter, "*.clx");
    #endif /* CELX */

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fs), filter);

    if (gtk_dialog_run(GTK_DIALOG(fs)) == GTK_RESPONSE_ACCEPT)
    {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
        openScript(filename, app);
        g_free(filename);
    }

    gtk_widget_destroy(fs);
}

/* File -> Capture Image... */
void
actionCaptureImage(GtkAction*, AppData* app)
{
    GtkWidget* fs = gtk_file_chooser_dialog_new("Save Image to File",
                                                GTK_WINDOW(app->mainWindow),
                                                GTK_FILE_CHOOSER_ACTION_SAVE,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                                NULL);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PNG and JPEG Images");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fs), filter);

    #if GTK_CHECK_VERSION(2, 7, 0)
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fs), TRUE);
    #endif /* GTK_CHECK_VERSION */

    gtk_dialog_set_default_response(GTK_DIALOG(fs), GTK_RESPONSE_ACCEPT);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fs), g_get_home_dir());

    if (gtk_dialog_run(GTK_DIALOG(fs)) == GTK_RESPONSE_ACCEPT)
    {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
        gtk_widget_destroy(fs);
        for (int i=0; i < 10 && gtk_events_pending ();i++)
            gtk_main_iteration ();
        captureImage(filename, app);
        g_free(filename);
    }
    else
    {
        gtk_widget_destroy(fs);
    }
}

/* File -> Capture Movie... */
void
actionCaptureMovie(GtkAction*, AppData* app)
{
#ifdef USE_FFMPEG
    // TODO: The menu item should be disable so that the user doesn't even
    // have the opportunity to record two movies simultaneously; the only
    // thing missing to make this happen is notification when recording
    // is complete.
    if (app->core->isCaptureActive())
    {
        GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(app->mainWindow),
                               GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_ERROR,
                               GTK_BUTTONS_OK,
                               "Stop current movie capture before starting another one.");
        gtk_dialog_run(GTK_DIALOG(errBox));
        gtk_widget_destroy(errBox);
        return;
    }

    GtkWidget* fs = gtk_file_chooser_dialog_new("Save Matroska Movie to File",
                                                GTK_WINDOW(app->mainWindow),
                                                GTK_FILE_CHOOSER_ACTION_SAVE,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                                NULL);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Matroska Files");
    gtk_file_filter_add_pattern(filter, "*.mkv");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fs), filter);

    #if GTK_CHECK_VERSION(2, 7, 0)
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fs), TRUE);
    #endif /* GTK_CHECK_VERSION */

    gtk_dialog_set_default_response(GTK_DIALOG(fs), GTK_RESPONSE_ACCEPT);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fs), g_get_home_dir());

    GtkWidget* hbox = gtk_hbox_new(FALSE, CELSPACING);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);

    GtkWidget* rlabel = gtk_label_new("Resolution:");
    gtk_box_pack_start(GTK_BOX(hbox), rlabel, TRUE, TRUE, 0);

    GtkWidget* vscombo = gtk_combo_box_text_new();
    for (const auto& size : MovieSizes)
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vscombo),
                                       fmt::format("{} x {}", size[0], size[1]).c_str());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(vscombo), 0);
    gtk_box_pack_start(GTK_BOX(hbox), vscombo, FALSE, FALSE, 0);

    GtkWidget* flabel = gtk_label_new("Frame Rate:");
    gtk_box_pack_start(GTK_BOX(hbox), flabel, TRUE, TRUE, 0);

    GtkWidget* frcombo = gtk_combo_box_text_new();
    for (float i : MovieFramerates)
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(frcombo),
                                       fmt::format("{:.3f}", i).c_str());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(frcombo), 0);
    gtk_box_pack_start(GTK_BOX(hbox), frcombo, FALSE, FALSE, 0);

    GtkWidget* vclabel = gtk_label_new("Video Codec:");
    gtk_box_pack_start(GTK_BOX(hbox), vclabel, TRUE, TRUE, 0);

    GtkWidget* vccombo = gtk_combo_box_text_new();
    for (const auto &mcodec : MovieCodecs)
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vccombo),
                                       mcodec.codecDesc);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(vccombo), 0);
    gtk_box_pack_start(GTK_BOX(hbox), vccombo, FALSE, FALSE, 0);

    GtkWidget* brlabel = gtk_label_new("Bitrate:");
    gtk_box_pack_start(GTK_BOX(hbox), brlabel, TRUE, TRUE, 0);
    GtkWidget* brentry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(brentry), "400000");
    g_signal_connect(G_OBJECT(brentry), "insert-text", G_CALLBACK(insert_text_event), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), brentry, TRUE, TRUE, 0);

    gtk_widget_show_all(hbox);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(fs), hbox);

    if (gtk_dialog_run(GTK_DIALOG(fs)) == GTK_RESPONSE_ACCEPT)
    {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
        int vsidx = gtk_combo_box_get_active(GTK_COMBO_BOX(vscombo));
        int fridx = gtk_combo_box_get_active(GTK_COMBO_BOX(frcombo));
        int vcidx = gtk_combo_box_get_active(GTK_COMBO_BOX(vccombo));
        const gchar *brtext = gtk_entry_get_text(GTK_ENTRY(brentry));
        const int *resolution = MovieSizes[vsidx].data();
        float fps = MovieFramerates[fridx];
        AVCodecID codec = MovieCodecs[vcidx].codecId;
        float bitrate = 400000;
        const gchar *last = &brtext[gtk_entry_get_text_length(GTK_ENTRY(brentry))];
        celestia::compat::from_chars(brtext, last, bitrate);

        gtk_widget_destroy(fs);
        for (int i=0; i < 10 && gtk_events_pending ();i++)
            gtk_main_iteration ();
        captureMovie(filename, resolution, fps, codec, bitrate, app);
        g_free(filename);
    }
    else
    {
        gtk_widget_destroy(fs);
    }
#else
    GtkWidget* errBox = gtk_message_dialog_new(GTK_WINDOW(app->mainWindow),
                           GTK_DIALOG_DESTROY_WITH_PARENT,
                           GTK_MESSAGE_ERROR,
                           GTK_BUTTONS_OK,
                           "Movie support was not included. To use re-build with --enable-theora.");
    gtk_dialog_run(GTK_DIALOG(errBox));
    gtk_widget_destroy(errBox);
#endif
}

/* File -> Run Demo... */
void
actionRunDemo(GtkAction*, AppData* app)
{
    const auto& demoScriptFile = app->core->getConfig()->paths.demoScriptFile;
    if (!demoScriptFile.empty())
    {
        app->core->cancelScript();
        app->core->runScript(demoScriptFile);
    }
}

void
actionQuit(GtkAction*, AppData* app)
{
    saveSettingsFile(app);
    gtk_main_quit();
}

void
actionSelectSol(GtkAction*, AppData* app)
{
    app->core->charEntered('H');
}

void
actionTourGuide(GtkAction*, AppData* app)
{
    dialogTourGuide(app);
}

void
actionSearchObject(GtkAction*, AppData* app)
{
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Select Object",
                                                    GTK_WINDOW(app->mainWindow),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                    NULL);

    GtkWidget* box = gtk_hbox_new(FALSE, CELSPACING);
    gtk_container_set_border_width(GTK_CONTAINER(box), CELSPACING);
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), box, TRUE, TRUE, 0);

    GtkWidget* label = gtk_label_new("Object name");
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    GtkWidget* entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(GTK_WIDGET(dialog));

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        const gchar* name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (name != NULL)
        {
            Selection sel = app->simulation->findObjectFromPath(name, true);
            if (!sel.empty())
                app->simulation->setSelection(sel);
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
actionGotoObject(GtkAction*, AppData* app)
{
    dialogGotoObject(app);
}

void
actionCenterSelection(GtkAction*, AppData* app)
{
    app->core->charEntered('c');
}

void
actionGotoSelection(GtkAction*, AppData* app)
{
    app->core->charEntered('G');
}

void
actionFollowSelection(GtkAction*, AppData* app)
{
    app->core->charEntered('F');
}

void
actionSyncSelection(GtkAction*, AppData* app)
{
    app->core->charEntered('Y');
}

void
actionTrackSelection(GtkAction*, AppData* app)
{
    app->core->charEntered('T');
}

void
actionSystemBrowser(GtkAction*, AppData* app)
{
    dialogSolarBrowser(app);
}

void
actionStarBrowser(GtkAction*, AppData* app)
{
    dialogStarBrowser(app);
}

void
actionEclipseFinder(GtkAction*, AppData* app)
{
    dialogEclipseFinder(app);
}

void
actionTimeFaster(GtkAction*, AppData* app)
{
    app->core->charEntered('L');
}

void
actionTimeSlower(GtkAction*, AppData* app)
{
    app->core->charEntered('K');
}

void
actionTimeFreeze(GtkAction*, AppData* app)
{
    app->core->charEntered(' ');
}

void
actionTimeReal(GtkAction*, AppData* app)
{
    app->core->charEntered('\\');
}

void
actionTimeReverse(GtkAction*, AppData* app)
{
    app->core->charEntered('J');
}

void
actionTimeSet(GtkAction*, AppData* app)
{
    dialogSetTime(app);
}

void
actionTimeLocal(GtkAction* action, AppData* app)
{
    app->showLocalTime = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
    updateTimeZone(app, app->showLocalTime);
}

void
actionViewerSize(GtkAction*, AppData* app)
{
    GtkWidget* dialog;
    int newX, currentX, currentY, winX, winY, screenX, i = 1, position = -1;
    char res[32];

    screenX = gdk_screen_get_width(gdk_screen_get_default());
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(app->glArea), &allocation);
    currentX = allocation.width;
    currentY = allocation.height;

    dialog = gtk_dialog_new_with_buttons("Set Viewer Size...",
                                         GTK_WINDOW(app->mainWindow),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK, GTK_RESPONSE_OK,
                                         NULL);

    GtkWidget* label = gtk_label_new("Dimensions for Main Window:");
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), label, TRUE, TRUE, 0);

    GtkWidget* menubox = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(content_area), menubox, TRUE, TRUE, 0);

    while (resolutions[i] != -1)
    {
        if (position == -1 && resolutions[i-1] < currentX && resolutions[i] >= currentX)
        {
            std::sprintf(res, "%d x %d (current)", currentX, currentY);
            position = i - 1;
        }
        else if (resolutions[i] < screenX)
        {
            std::sprintf(res, "%d x %d", resolutions[i], int(0.75 * resolutions[i]));
            i++;
        }
        else
            break;

        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(menubox), res);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(menubox), position);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(menubox));

        if (active > -1 && active != position)
        {
            /* Adjust for default entry */
            if (active > position) active--;

            newX = resolutions[active + 1];
            gtk_window_get_size(GTK_WINDOW(app->mainWindow), &winX, &winY);

            /* Resizing takes into account border, titlebar, and menubar
               sizes. Without them only an allocation can be requested */
            gtk_window_resize(GTK_WINDOW(app->mainWindow), newX + winX - currentX, int(0.75 * newX) + winY - currentY);
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
actionFullScreen(GtkAction* action, AppData* app)
{
    int positionX, positionY;
    app->fullScreen = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));

    if (app->fullScreen)
    {
        /* Save size/position, so original numbers are available for prefs */
        GtkAllocation allocation;
        gtk_widget_get_allocation(GTK_WIDGET(app->glArea), &allocation);
        g_object_set_data(G_OBJECT(app->mainWindow), "sizeX", GINT_TO_POINTER(allocation.width));
        g_object_set_data(G_OBJECT(app->mainWindow), "sizeY", GINT_TO_POINTER(allocation.height));
        gtk_window_get_position(GTK_WINDOW(app->mainWindow), &positionX, &positionY);
        g_object_set_data(G_OBJECT(app->mainWindow), "positionX", GINT_TO_POINTER(positionX));
        g_object_set_data(G_OBJECT(app->mainWindow), "positionY", GINT_TO_POINTER(positionY));

        gtk_window_fullscreen(GTK_WINDOW(app->mainWindow));
    }
    else
        gtk_window_unfullscreen(GTK_WINDOW(app->mainWindow));

    /* Enable/Disable the Viewer Size action */
    gtk_action_set_sensitive(gtk_action_group_get_action(app->agMain, "ViewerSize"), !app->fullScreen);
}

void
actionViewOptions(GtkAction*, AppData* app)
{
    dialogViewOptions(app);
}

void
actionStarsMore(GtkAction*, AppData* app)
{
    app->core->charEntered(']');
}

void
actionStarsFewer(GtkAction*, AppData* app)
{
    app->core->charEntered('[');
}

void
actionMenuBarVisible(GtkToggleAction* action, AppData* app)
{
    g_object_set(G_OBJECT(app->mainMenu), "visible", gtk_toggle_action_get_active(action), NULL);
}

void
actionMultiSplitH(GtkAction*, AppData* app)
{
    app->core->splitView(celestia::View::HorizontalSplit);
}

void
actionMultiSplitV(GtkAction*, AppData* app)
{
    app->core->splitView(celestia::View::VerticalSplit);
}

void
actionMultiCycle(GtkAction*, AppData* app)
{
    /* Pass a Tab character */
    app->core->charEntered('\011');
}

void
actionMultiDelete(GtkAction*, AppData* app)
{
    app->core->deleteView();
}

void
actionMultiSingle(GtkAction*, AppData* app)
{
    app->core->singleView();
}

void
actionMultiShowFrames(GtkToggleAction* action, AppData* app)
{
    app->core->setFramesVisible(gtk_toggle_action_get_active(action));
}

void
actionMultiShowActive(GtkToggleAction* action, AppData* app)
{
    app->core->setActiveFrameVisible(gtk_toggle_action_get_active(action));
}

void
actionMultiSyncTime(GtkToggleAction* action, AppData* app)
{
    app->simulation->setSyncTime(gtk_toggle_action_get_active(action));
}

void
actionHelpControls(GtkAction*, AppData* app)
{
    char *txt = readFromFile("controls.txt");
    textInfoDialog(txt, "Mouse and Keyboard Controls", app);
    g_free(txt);
}

void
actionHelpOpenGL(GtkAction*, AppData* app)
{
    std::string s = Helper::getRenderInfo(app->renderer);
    textInfoDialog(s.c_str(), "Renderer Info", app);
}

void
actionHelpAbout(GtkAction*, AppData* app)
{
    const gchar* authors[] = {
        "Chris Laurel <claurel@shatters.net>",
        "Clint Weisbrod <cweisbrod@cogeco.ca>",
        "Fridger Schrempp <fridger.schrempp@desy.de>",
        "Bob Ippolito <bob@redivi.com>",
        "Christophe Teyssier <chris@teyssier.org>",
        "Hank Ramsey <hramsey@users.sourceforce.net>",
        "Grant Hutchison <grantcelestia@xemaps.com>",
        "Pat Suwalski <pat@suwalski.net>",
        "Toti <>",
        "Da-Woon Jung <dirkpitt2050@users.sf.net>",
        "Vincent Giangiulio <vince.gian@free.fr>",
        NULL
    };

    GdkPixbuf *logo = gdk_pixbuf_new_from_file ("celestia-logo.png", NULL);

    std::string comments = fmt::format("GTK+ Front-End, built using gtk+ version {}.{}",
                                       GTK_MAJOR_VERSION,
                                       GTK_MINOR_VERSION);

    gtk_show_about_dialog(GTK_WINDOW(app->mainWindow),
                         "title", _("About Celestia"),
                         "program-name", "Celestia",
                         "version", VERSION,
                         "copyright", "Copyright \xc2\xa9 2001-2023 Celestia Development Team",
                         "comments", comments.c_str(),
                         "website", "https://celestiaproject.space",
                         "authors", authors,
                         "license", readFromFile("COPYING"),
                         "logo", logo,
                         NULL);
    g_object_unref(logo);
}

void
actionVerbosity(GtkRadioAction* action, GtkRadioAction*, AppData* app)
{
    int value = gtk_radio_action_get_current_value(action);
    app->core->setHudDetail(value);
}

void
actionStarStyle(GtkRadioAction* action, GtkRadioAction*, AppData* app)
{
    int value = gtk_radio_action_get_current_value(action);
    app->renderer->setStarStyle((Renderer::StarStyle)value);
}

void
actionAmbientLight(GtkRadioAction* action, GtkRadioAction*, AppData* app)
{
    float value = amLevels[gtk_radio_action_get_current_value(action)];
    app->renderer->setAmbientLightLevel(value);
}

/* Render-Flag Actions */
void
actionRenderAA(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowSmoothLines, gtk_toggle_action_get_active(action));
}

void
actionRenderAtmospheres(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowAtmospheres, gtk_toggle_action_get_active(action));
}

void
actionRenderAutoMagnitude(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowAutoMag, gtk_toggle_action_get_active(action));
}

void
actionRenderCelestialGrid(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowCelestialSphere, gtk_toggle_action_get_active(action));
}

void
actionRenderClouds(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowCloudMaps, gtk_toggle_action_get_active(action));
}

void
actionRenderCometTails(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowCometTails, gtk_toggle_action_get_active(action));
}

void
actionRenderConstellationBoundaries(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowBoundaries, gtk_toggle_action_get_active(action));
}

void
actionRenderConstellations(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowDiagrams, gtk_toggle_action_get_active(action));
}

void
actionRenderEclipticGrid(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowEclipticGrid, gtk_toggle_action_get_active(action));
}

void
actionRenderEclipseShadows(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowEclipseShadows, gtk_toggle_action_get_active(action));
}

void
actionRenderGalacticGrid(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowGalacticGrid, gtk_toggle_action_get_active(action));
}

void
actionRenderGalaxies(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowGalaxies, gtk_toggle_action_get_active(action));
}

void
actionRenderGlobulars(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowGlobulars, gtk_toggle_action_get_active(action));
}

void
actionRenderHorizontalGrid(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowHorizonGrid, gtk_toggle_action_get_active(action));
}

void
actionRenderMarkers(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowMarkers, gtk_toggle_action_get_active(action));
}

void
actionRenderNebulae(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowNebulae, gtk_toggle_action_get_active(action));
}

void
actionRenderNightLights(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowNightMaps, gtk_toggle_action_get_active(action));
}

void
actionRenderOpenClusters(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowOpenClusters, gtk_toggle_action_get_active(action));
}

void
actionRenderOrbits(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowOrbits, gtk_toggle_action_get_active(action));
}

void
actionRenderFadingOrbits(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowFadingOrbits, gtk_toggle_action_get_active(action));
}

void
actionRenderPlanets(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowPlanets, gtk_toggle_action_get_active(action));
}

void
actionRenderDwarfPlanets(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowDwarfPlanets, gtk_toggle_action_get_active(action));
}

void
actionRenderMoons(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowMoons, gtk_toggle_action_get_active(action));
}

void
actionRenderMinorMoons(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowMinorMoons, gtk_toggle_action_get_active(action));
}

void
actionRenderAsteroids(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowAsteroids, gtk_toggle_action_get_active(action));
}

void
actionRenderComets(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowComets, gtk_toggle_action_get_active(action));
}

void
actionRenderSpacecrafts(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowSpacecrafts, gtk_toggle_action_get_active(action));
}

void
actionRenderPlanetRings(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowPlanetRings, gtk_toggle_action_get_active(action));
}

void
actionRenderRingShadows(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowRingShadows, gtk_toggle_action_get_active(action));
}

void
actionRenderStars(GtkToggleAction* action, AppData* app)
{
    setRenderFlag(app, Renderer::ShowStars, gtk_toggle_action_get_active(action));
}

void
actionOrbitAsteroids(GtkToggleAction* action, AppData* app)
{
    setOrbitMask(app, BodyClassification::Asteroid, gtk_toggle_action_get_active(action));
}

void
actionOrbitComets(GtkToggleAction* action, AppData* app)
{
    setOrbitMask(app, BodyClassification::Comet, gtk_toggle_action_get_active(action));
}

void
actionOrbitMoons(GtkToggleAction* action, AppData* app)
{
    setOrbitMask(app, BodyClassification::Moon, gtk_toggle_action_get_active(action));
}

void
actionOrbitPlanets(GtkToggleAction* action, AppData* app)
{
    setOrbitMask(app, BodyClassification::Planet, gtk_toggle_action_get_active(action));
}

void
actionOrbitSpacecraft(GtkToggleAction* action, AppData* app)
{
    setOrbitMask(app, BodyClassification::Spacecraft, gtk_toggle_action_get_active(action));
}

void
actionLabelAsteroids(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::AsteroidLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelComets(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::CometLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelConstellations(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::ConstellationLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelGalaxies(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::GalaxyLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelGlobulars(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::GlobularLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelLocations(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::LocationLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelMoons(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::MoonLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelMinorMoons(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::MinorMoonLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelNebulae(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::NebulaLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelOpenClusters(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::OpenClusterLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelPlanets(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::PlanetLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelDwarfPlanets(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::DwarfPlanetLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelSpacecraft(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::SpacecraftLabels, gtk_toggle_action_get_active(action));
}

void
actionLabelStars(GtkToggleAction* action, AppData* app)
{
    setLabelMode(app, Renderer::StarLabels, gtk_toggle_action_get_active(action));
}

/* Synchronizes the Label Actions with the state of the core */
void
resyncLabelActions(AppData* app)
{
    /* Simply for readability */
    int f = app->renderer->getLabelMode();

    for (int i = Renderer::StarLabels; i <= Renderer::GlobularLabels; i *= 2)
    {
        const char* actionName;
        switch (i)
        {
            case Renderer::StarLabels: actionName = "LabelStars"; break;
            case Renderer::PlanetLabels: actionName = "LabelPlanets"; break;
            case Renderer::DwarfPlanetLabels: actionName = "LabelDwarfPlanets"; break;
            case Renderer::MoonLabels: actionName = "LabelMoons"; break;
            case Renderer::MinorMoonLabels: actionName = "LabelMinorMoons"; break;
            case Renderer::ConstellationLabels: actionName = "LabelConstellations"; break;
            case Renderer::GalaxyLabels: actionName = "LabelGalaxies"; break;
            case Renderer::AsteroidLabels: actionName = "LabelAsteroids"; break;
            case Renderer::SpacecraftLabels: actionName = "LabelSpacecraft"; break;
            case Renderer::LocationLabels: actionName = "LabelLocations"; break;
            case Renderer::CometLabels: actionName = "LabelComets"; break;
            case Renderer::NebulaLabels: actionName = "LabelNebulae"; break;
            case Renderer::OpenClusterLabels: actionName = "LabelOpenClusters"; break;
            case Renderer::GlobularLabels: actionName = "LabelGlobulars"; break;
            case Renderer::I18nConstellationLabels: /* Not used yet */
            default: actionName = nullptr; break;
        }

        if (actionName != nullptr)
        {
            /* Get the action */
            GtkAction* action = gtk_action_group_get_action(app->agLabel, actionName);

            /* The current i anded with the labelMode gives state of flag */
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), (i & f));
        }
    }
}

/* Synchronizes the Render Actions with the state of the core */
void
resyncRenderActions(AppData* app)
{
    /* Simply for readability */
    const std::uint64_t rf = app->renderer->getRenderFlags();

    /* Unlike the other interfaces, which go through each menu item and set
     * the corresponding renderFlag, we go the other way and set the menu
     * based on the renderFlag. Last one is ShowPlanetRings. */

    for (std::uint64_t i = Renderer::ShowStars; i <= Renderer::ShowPlanetRings; i *= 2)
    {
        const char* actionName;
        switch (i)
        {
            case Renderer::ShowStars: actionName = "RenderStars"; break;
            case Renderer::ShowPlanets: actionName = "RenderPlanets"; break;
            case Renderer::ShowDwarfPlanets: actionName = "RenderDwarfPlanets"; break;
            case Renderer::ShowMoons: actionName = "RenderMoons"; break;
            case Renderer::ShowMinorMoons: actionName = "RenderMinorMoons"; break;
            case Renderer::ShowAsteroids: actionName = "RenderAsteroids"; break;
            case Renderer::ShowComets: actionName = "RenderComets"; break;
            case Renderer::ShowSpacecrafts: actionName = "RenderSpacecrafts"; break;
            case Renderer::ShowGalaxies: actionName = "RenderGalaxies"; break;
            case Renderer::ShowDiagrams: actionName = "RenderConstellations"; break;
            case Renderer::ShowCloudMaps: actionName = "RenderClouds"; break;
            case Renderer::ShowOrbits: actionName = "RenderOrbits"; break;
            case Renderer::ShowFadingOrbits: actionName = "RenderFadingOrbits"; break;
            case Renderer::ShowCelestialSphere: actionName = "RenderCelestialGrid"; break;
            case Renderer::ShowNightMaps: actionName = "RenderNightLights"; break;
            case Renderer::ShowAtmospheres: actionName = "RenderAtmospheres"; break;
            case Renderer::ShowSmoothLines: actionName = "RenderAA"; break;
            case Renderer::ShowEclipseShadows: actionName = "RenderEclipseShadows"; break;
            case Renderer::ShowPlanetRings: actionName = "RenderPlanetRings"; break;
            case Renderer::ShowRingShadows: actionName = "RenderRingShadows"; break;
            case Renderer::ShowBoundaries: actionName = "RenderConstellationBoundaries"; break;
            case Renderer::ShowAutoMag: actionName = "RenderAutoMagnitude"; break;
            case Renderer::ShowCometTails: actionName = "RenderCometTails"; break;
            case Renderer::ShowMarkers: actionName = "RenderMarkers"; break;
            case Renderer::ShowPartialTrajectories: actionName = nullptr; break; /* Not useful yet */
            case Renderer::ShowNebulae: actionName = "RenderNebulae"; break;
            case Renderer::ShowOpenClusters: actionName = "RenderOpenClusters"; break;
            case Renderer::ShowGlobulars: actionName = "RenderGlobulars"; break;
            case Renderer::ShowCloudShadows: actionName = nullptr; break; /* Not implemented yet */
            case Renderer::ShowGalacticGrid: actionName = "RenderGalacticGrid"; break;
            case Renderer::ShowEclipticGrid: actionName = "RenderEclipticGrid"; break;
            case Renderer::ShowHorizonGrid: actionName = "RenderHorizontalGrid"; break;
            case Renderer::ShowEcliptic: actionName = nullptr; break; /* Not implemented yet */
            default: actionName = nullptr; break;
        }

        if (actionName != nullptr)
        {
            /* Get the action */
            GtkAction* action = gtk_action_group_get_action(app->agRender, actionName);

            /* The current i anded with the renderFlags gives state of flag */
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), (i & rf) != 0);
        }
    }
}

/* Synchronizes the Orbit Actions with the state of the core */
void
resyncOrbitActions(AppData* app)
{
    /* Simply for readability */
    BodyClassification om = app->renderer->getOrbitMask();
    constexpr std::array orbitActions
    {
        std::make_tuple(BodyClassification::Planet, "OrbitPlanets"),
        std::make_tuple(BodyClassification::Moon, "OrbitMoons"),
        std::make_tuple(BodyClassification::Asteroid, "OrbitAsteroids"),
        std::make_tuple(BodyClassification::Comet, "OrbitComets"),
        std::make_tuple(BodyClassification::Spacecraft, "OrbitSpacecraft"),
    };

    for (auto [classification, actionName] : orbitActions)
    {
        GtkAction* action = gtk_action_group_get_action(app->agOrbit, actionName);
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), util::is_set(om, classification));
    }
}

/* Synchronizes the Verbosity Actions with the state of the core */
void
resyncVerbosityActions(AppData* app)
{
    const char* actionName;
    switch (app->core->getHudDetail())
    {
        case 0: actionName = "HudNone"; break;
        case 1: actionName = "HudTerse"; break;
        case 2: actionName = "HudVerbose"; break;
        default: return;
    }

    /* Get the action, set the widget */
    GtkAction* action = gtk_action_group_get_action(app->agVerbosity, actionName);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
}

/* Synchronizes the TimeZone Action with the state of the core */
void
resyncTimeZoneAction(AppData* app)
{
    /* Get the action, set the widget */
    GtkAction* action = gtk_action_group_get_action(app->agMain, "TimeLocal");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), app->showLocalTime);
}

/* Synchronizes the Ambient Light Actions with the state of the core */
void
resyncAmbientActions(AppData* app)
{
    GtkAction* action;

    float ambient = app->renderer->getAmbientLightLevel();

    /* Try to be smart about being close to the value of a float */
    if (ambient > amLevels[0] && ambient < (amLevels[1] / 2.0))
        action = gtk_action_group_get_action(app->agAmbient, "AmbientNone");

    else if (ambient < amLevels[1] + ((amLevels[2] - amLevels[1]) / 2.0))
        action = gtk_action_group_get_action(app->agAmbient, "AmbientLow");

    else
        action = gtk_action_group_get_action(app->agAmbient, "AmbientMedium");

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
}

/* Synchronizes the Verbosity Actions with the state of the core */
void
resyncStarStyleActions(AppData* app)
{
    GtkAction* action;
    const char* actionName;

    switch (app->renderer->getStarStyle())
    {
        case Renderer::FuzzyPointStars: actionName = "StarsFuzzy"; break;
        case Renderer::PointStars: actionName = "StarsPoints"; break;
        case Renderer::ScaledDiscStars: actionName = "StarsDiscs"; break;
        default: return;
    }

    /* Get the action, set the widget */
    action = gtk_action_group_get_action(app->agStarStyle, actionName);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
}

/* Placeholder for when galaxy brightness is added as an action */
void
resyncGalaxyGainActions(AppData* app)
{
    /* no-op */
}

/* Synchronizes the Texture Resolution with the state of the core */
void
resyncTextureResolutionActions(AppData* app)
{
    /* no-op */
}

} // end namespace celestia::gtk
