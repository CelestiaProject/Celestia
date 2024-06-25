/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-star.cpp,v 1.3 2006-09-05 04:57:51 suwalski Exp $
 */

#include <gtk/gtk.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <system_error>
#include <vector>

#include <celcompat/charconv.h>
#include <celengine/body.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/star.h>
#include <celengine/starbrowser.h>
#include <celengine/stardb.h>
#include <celengine/univcoord.h>
#include <celutil/greek.h>

#include "dialog-star.h"
#include "actions.h"
#include "common.h"

namespace celestia::gtk
{

namespace
{

constexpr std::array<const char*, 5> sbTitles
{
    "Name",
    "Distance(LY)",
    "App. Mag",
    "Abs. Mag",
    "Type",
};

constexpr std::array<const char*, 5> sbRadioLabels
{
    "Nearest",
    "Brightest (App.)",
    "Brightest (Abs.)",
    "With Planets",
    nullptr,
};

/* Local Data Structures */
struct sbData
{
    explicit sbData(AppData*);

    AppData* app;

    engine::StarBrowser browser;
    std::vector<engine::StarBrowserRecord> records;
    GtkListStore* starListStore{ nullptr };
    GtkWidget* entry{ nullptr };
    GtkWidget* scale{ nullptr };
};

sbData::sbData(AppData* appData) :
    app(appData),
    browser(appData->simulation->getUniverse())
{
}

/* HELPER: Clear and Add stars to the starListStore */
void
addStars(sbData* sb)
{
    const char *values[5];
    GtkTreeIter iter;

    std::uint32_t currentLength;
    UniversalCoord ucPos;

    /* Load the catalogs and set data */
    const StarDatabase* stardb = sb->app->simulation->getUniverse()->getStarCatalog();
    sb->browser.setPosition(sb->app->simulation->getObserver().getPosition());
    sb->browser.populate(sb->records);
    currentLength = sb->records.size();
    if (!sb->records.empty())
        sb->app->simulation->setSelection(Selection(const_cast<Star*>(sb->records.front().star)));
    ucPos = sb->app->simulation->getObserver().getPosition();

    gtk_list_store_clear(sb->starListStore);

    for (std::uint32_t i = 0; i < currentLength; i++)
    {
        char buf[20];
        const engine::StarBrowserRecord& record = sb->records[i];
        values[0] = g_strdup(ReplaceGreekLetterAbbr((stardb->getStarName(*record.star, true))).c_str());

        /* Calculate distance to star */
        std::sprintf(buf, " %.3f ", record.distance);
        values[1] = g_strdup(buf);

        std::sprintf(buf, " %.2f ", record.appMag);
        values[2] = g_strdup(buf);

        std::sprintf(buf, " %.2f ", record.star->getAbsoluteMagnitude());
        values[3] = g_strdup(buf);

        gtk_list_store_append(sb->starListStore, &iter);
        gtk_list_store_set(sb->starListStore, &iter,
                           0, values[0],
                           1, values[1],
                           2, values[2],
                           3, values[3],
                           4, record.star->getSpectralType(),
                           5, (gpointer)&record, -1);
    }
}

/* CALLBACK: When Star is selected in Star Browser */
void
listStarSelect(GtkTreeSelection* sel, AppData* app)
{
    GValue value = { 0, {{0}} }; /* Initialize GValue to 0 */
    GtkTreeIter iter;
    GtkTreeModel* model;

    /* IF prevents selection while list is being updated */
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;

    gtk_tree_model_get_value(model, &iter, 5, &value);
    auto record = (const engine::StarBrowserRecord*)g_value_get_pointer(&value);
    g_value_unset(&value);

    if (record)
        app->simulation->setSelection(Selection(const_cast<Star*>(record->star)));
}

/* CALLBACK: Refresh button is pressed. */
void
refreshBrowser(GtkWidget*, sbData* sb)
{
    addStars(sb);
}

/* CALLBACK: One of the RadioButtons is pressed */
void
radioClicked(GtkButton* r, gpointer choice)
{
    sbData* sb = (sbData*)g_object_get_data(G_OBJECT(r), "data");
    gint selection = GPOINTER_TO_INT(choice);

    switch (selection)
    {
        case 0:
            sb->browser.setComparison(engine::StarBrowser::Comparison::Nearest);
            sb->browser.setFilter(engine::StarBrowser::Filter::Visible);
            break;
        case 1:
            sb->browser.setComparison(engine::StarBrowser::Comparison::ApparentMagnitude);
            sb->browser.setFilter(engine::StarBrowser::Filter::Visible);
            break;
        case 2:
            sb->browser.setComparison(engine::StarBrowser::Comparison::AbsoluteMagnitude);
            sb->browser.setFilter(engine::StarBrowser::Filter::Visible);
            break;
        case 3:
            sb->browser.setComparison(engine::StarBrowser::Comparison::Nearest);
            sb->browser.setFilter(engine::StarBrowser::Filter::WithPlanets);
            break;
        default:
            return;
    }

    refreshBrowser(nullptr, sb);
}

/* CALLBACK: Maximum stars EntryBox changed. */
void
listStarEntryChange(GtkEntry *entry, GdkEventFocus *event, sbData* sb)
{
    /* If not called by the slider, but rather by user.
       Prevents infinite recursion. */
    if (event != nullptr)
    {
        std::string_view entryText = gtk_entry_get_text(entry);
        std::uint32_t numListStars;
        if (auto ec = celestia::compat::from_chars(entryText.data(), entryText.data() + entryText.size(), numListStars).ec;
            ec == std::errc{})
        {
            if (!sb->browser.setSize(numListStars))
            {
                /* Call self to set text (NULL event = no recursion) */
                listStarEntryChange(entry, nullptr, sb);
            }

            gtk_range_set_value(GTK_RANGE(sb->scale), static_cast<gdouble>(numListStars));
        }
        else
        {
            /* Call self to set text (NULL event = no recursion) */
            listStarEntryChange(entry, nullptr, sb);
        }
    }

    /* Update value of this box */
    char stars[4];
    std::sprintf(stars, "%" PRIu32, sb->browser.size());
    gtk_entry_set_text(entry, stars);
}

/* CALLBACK: Maximum stars RangeSlider changed. */
void
listStarSliderChange(GtkRange *range, sbData* sb)
{
    /* Update the value of the text entry box */
    sb->browser.setSize(static_cast<std::uint32_t>(gtk_range_get_value(GTK_RANGE(range))));
    listStarEntryChange(GTK_ENTRY(sb->entry), nullptr, sb);

    /* Refresh the browser listbox */
    refreshBrowser(nullptr, sb);
}

/* CALLBACK: Destroy Window */
void
starDestroy(GtkWidget* w, gint responseId, sbData* sb)
{
    gtk_widget_destroy(GTK_WIDGET(w));

    if (responseId == GTK_RESPONSE_DELETE_EVENT)
        delete sb;
}

} // end unnamed namespace

/* ENTRY: Navigation -> Star Browser... */
void
dialogStarBrowser(AppData* app)
{
    auto sb = new sbData(app);

    GtkWidget *browser = gtk_dialog_new_with_buttons("Star System Browser",
                                                     GTK_WINDOW(app->mainWindow),
                                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                     nullptr);
    app->simulation->setSelection(Selection());

    /* Star System Browser */
    GtkWidget *mainbox = gtk_dialog_get_content_area(GTK_DIALOG(browser));
    gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);

    GtkWidget *scrolled_win = gtk_scrolled_window_new (nullptr, nullptr);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(mainbox), scrolled_win, TRUE, TRUE, 0);

    /* Create listbox list */
    sb->starListStore = gtk_list_store_new(6,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           G_TYPE_POINTER);
    GtkWidget *starList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sb->starListStore));

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(starList), TRUE);
    gtk_container_add(GTK_CONTAINER(scrolled_win), starList);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* Add the columns */
    for (int i=0; i<5; i++) {
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes (sbTitles[i], renderer, "text", i, nullptr);
        if (i > 0 && i < 4) {
            /* Right align */
            gtk_tree_view_column_set_alignment(column, 1.0);
            g_object_set(G_OBJECT(renderer), "xalign", 1.0, nullptr);
        }
        gtk_tree_view_append_column(GTK_TREE_VIEW(starList), column);
    }

    /* Set up callback for when a star is selected */
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(starList));
    g_signal_connect(selection, "changed", G_CALLBACK(listStarSelect), app);

    /* From now on, it's the bottom-of-the-window controls */
    GtkWidget *frame = gtk_frame_new("Star Search Criteria");
    gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);

    GtkWidget *hbox = gtk_hbox_new(FALSE, CELSPACING);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    /* List viewing preference settings */
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *hbox2 = gtk_hbox_new(FALSE, CELSPACING);
    GtkWidget *label = gtk_label_new("Maximum Stars Displayed in List");
    gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, FALSE, 0);
    sb->entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(sb->entry), 3);
    gtk_entry_set_width_chars(GTK_ENTRY(sb->entry), 5);
    gtk_box_pack_start(GTK_BOX(hbox2), sb->entry, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);
    sb->scale = gtk_hscale_new_with_range(engine::StarBrowser::MinListStars, engine::StarBrowser::MaxListStars, 1);
    gtk_scale_set_draw_value(GTK_SCALE(sb->scale), FALSE);
    g_signal_connect(sb->scale, "value-changed", G_CALLBACK(listStarSliderChange), sb);
    g_signal_connect(sb->entry, "focus-out-event", G_CALLBACK(listStarEntryChange), sb);
    gtk_box_pack_start(GTK_BOX(vbox), sb->scale, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, FALSE, 0);

    /* Set initial Star Value */
    gtk_range_set_value(GTK_RANGE(sb->scale), sb->browser.size());
    if (sb->browser.size() == engine::StarBrowser::MinListStars)
    {
        /* Force update manually (scale won't trigger event) */
        listStarEntryChange(GTK_ENTRY(sb->entry), nullptr, sb);
        refreshBrowser(nullptr, sb);
    }

    /* Radio Buttons */
    vbox = gtk_vbox_new(TRUE, 0);
    makeRadioItems(sbRadioLabels.data(), vbox, G_CALLBACK(radioClicked), nullptr, sb);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    /* Common Buttons */
    hbox = gtk_hbox_new(TRUE, CELSPACING);
    if (buttonMake(hbox, "Center", (GCallback)actionCenterSelection, app))
        return;
    if (buttonMake(hbox, "Go To", (GCallback)actionGotoSelection, app))
        return;
    if (buttonMake(hbox, "Refresh", (GCallback)refreshBrowser, sb))
        return;
    gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

    g_signal_connect(browser, "response", G_CALLBACK(starDestroy), sb);

    gtk_widget_set_size_request(browser, -1, 400); /* Absolute Size, urghhh */
    gtk_widget_show_all(browser);
}

} // end namespace celestia::gtk
