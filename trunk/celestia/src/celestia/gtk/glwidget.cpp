/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: glwidget.cpp,v 1.2 2006-07-24 17:31:24 christey Exp $
 */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>

#include <celestia/celestiacore.h>

#include "glwidget.h"
#include "actions.h"
#include "common.h"


/* Declarations: Callbacks */
static gint glarea_idle(AppData* app);
static gint glarea_configure(GtkWidget* widget, GdkEventConfigure*, AppData* app);
static gint glarea_expose(GtkWidget* widget, GdkEventExpose* event, AppData* app);
static gint glarea_motion_notify(GtkWidget*, GdkEventMotion* event, AppData* app);
static gint glarea_mouse_scroll(GtkWidget*, GdkEventScroll* event, AppData* app);
static gint glarea_button_press(GtkWidget*, GdkEventButton* event, AppData* app);
static gint glarea_button_release(GtkWidget*, GdkEventButton* event, AppData* app);
static gint glarea_key_press(GtkWidget* widget, GdkEventKey* event, AppData* app);
static gint glarea_key_release(GtkWidget* widget, GdkEventKey* event, AppData* app);

/* Declarations: Helpers */
static gint glDrawFrame(AppData* app);
static bool handleSpecialKey(int key, int state, bool down, AppData* app);


/* ENTRY: Initialize/Bind all glArea Callbacks */
void initGLCallbacks(AppData* app)
{
	g_signal_connect(GTK_OBJECT(app->glArea), "expose_event",
	                 G_CALLBACK(glarea_expose), app);
	g_signal_connect(GTK_OBJECT(app->glArea), "configure_event",
	                 G_CALLBACK(glarea_configure), app);
	g_signal_connect(GTK_OBJECT(app->glArea), "button_press_event",
	                 G_CALLBACK(glarea_button_press), app);
	g_signal_connect(GTK_OBJECT(app->glArea), "button_release_event",
	                 G_CALLBACK(glarea_button_release), app);
	g_signal_connect(GTK_OBJECT(app->glArea), "scroll_event",
	                 G_CALLBACK(glarea_mouse_scroll), app);
	g_signal_connect(GTK_OBJECT(app->glArea), "motion_notify_event",
	                 G_CALLBACK(glarea_motion_notify), app);
	g_signal_connect(GTK_OBJECT(app->glArea), "key_press_event",
	                 G_CALLBACK(glarea_key_press), app);
	g_signal_connect(GTK_OBJECT(app->glArea), "key_release_event",
	                 G_CALLBACK(glarea_key_release), app);
	
	/* Main call to execute redraw during GTK main loop */
	g_idle_add((GSourceFunc)glarea_idle, app);

}


/* CALLBACK: GL Function for main update (in GTK idle loop) */
static gint glarea_idle(AppData* app)
{
	app->core->tick();
	return glDrawFrame(app);
}


/* CALLBACK: GL Function for event "configure_event" */
static gint glarea_configure(GtkWidget* widget, GdkEventConfigure*, AppData* app)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
		return FALSE;

	app->core->resize(widget->allocation.width, widget->allocation.height);

	/* GConf changes only saved upon exit, since caused a lot of CPU activity
	 * while saving intermediate steps. */

	gdk_gl_drawable_gl_end (gldrawable);
	return TRUE;
}


/* CALLBACK: GL Function for event "expose_event" */
static gint glarea_expose(GtkWidget*, GdkEventExpose* event, AppData* app)
{
	/* Draw only the last expose */
	if (event->count > 0)
		return TRUE;

	/* Redraw -- draw checks are made in function */
	return glDrawFrame(app);
}


/* CALLBACK: GL Function for event "motion_notify_event" */
static gint glarea_motion_notify(GtkWidget*, GdkEventMotion* event, AppData* app)
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

	app->core->mouseMove(x - app->lastX, y - app->lastY, buttons);

	app->lastX = x;
	app->lastY = y;

	return TRUE;
}


/* CALLBACK: GL Function for event "scroll_event" */
static gint glarea_mouse_scroll(GtkWidget*, GdkEventScroll* event, AppData* app)
{
	if (event->direction == GDK_SCROLL_UP)
		app->core->mouseWheel(-1.0f, 0);
	else 
		app->core->mouseWheel(1.0f, 0);

	return TRUE;
}


/* CALLBACK: GL Function for event "button_press_event" */
static gint glarea_button_press(GtkWidget*, GdkEventButton* event, AppData* app)
{
	app->lastX = (int) event->x;
	app->lastY = (int) event->y;

	if (event->button == 1)
		app->core->mouseButtonDown(event->x, event->y, CelestiaCore::LeftButton);
	else if (event->button == 2)
		app->core->mouseButtonDown(event->x, event->y, CelestiaCore::MiddleButton);
	else if (event->button == 3)
		app->core->mouseButtonDown(event->x, event->y, CelestiaCore::RightButton);

	return TRUE;
}


/* CALLBACK: GL Function for event "button_release_event" */
static gint glarea_button_release(GtkWidget*, GdkEventButton* event, AppData* app)
{
	app->lastX = (int) event->x;
	app->lastY = (int) event->y;

	if (event->button == 1)
		app->core->mouseButtonUp(event->x, event->y, CelestiaCore::LeftButton);
	else if (event->button == 2)
		app->core->mouseButtonUp(event->x, event->y, CelestiaCore::MiddleButton);
	else if (event->button == 3)
		app->core->mouseButtonUp(event->x, event->y, CelestiaCore::RightButton);

	return TRUE;
}


/* CALLBACK: GL Function for event "key_press_event" */
static gint glarea_key_press(GtkWidget* widget, GdkEventKey* event, AppData* app)
{
	gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_press_event");

	switch (event->keyval)
	{
		case GDK_Escape:
			app->core->charEntered('\033');
			break;
		case GDK_BackSpace:
			app->core->charEntered('\b');
			break;
		case GDK_Tab:
			/* Tab has to be handled specially because keyDown and keyUp
			 * do not trigger auto-completion. */
			app->core->charEntered(event->keyval);
			break;
		case GDK_ISO_Left_Tab:
			/* This is what Celestia calls BackTab */
			app->core->charEntered(CelestiaCore::Key_BackTab);
			break;
		/* Temporary until galaxy brightness added as GtkAction */
		case GDK_bracketleft:
			app->core->charEntered('(');
			break;
		case GDK_bracketright:
			app->core->charEntered(')');
			break;
		default:
			if (!handleSpecialKey(event->keyval, event->state, true, app))
			{
				if ((event->string != NULL) && (*(event->string)))
				{
					/* See if our key accelerators will handle this event. */
					if((!app->core->getTextEnterMode()) && gtk_accel_groups_activate (G_OBJECT (app->mainWindow), event->keyval, GDK_SHIFT_MASK))
						return TRUE;

					char* s = event->string;

					while (*s != '\0')
					{
						char c = *s++;
						app->core->charEntered(c);
					}
				}
			}
			if (event->state & GDK_MOD1_MASK)
				return FALSE;
	}

	return TRUE;
}


/* CALLBACK: GL Function for event "key_release_event" */
static gint glarea_key_release(GtkWidget* widget, GdkEventKey* event, AppData* app)
{
	gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_release_event");
	return handleSpecialKey(event->keyval, event->state, false, app);
}


/* HELPER: GL Common Draw function.
 *         If everything checks out, call appCore->draw() */
static gint glDrawFrame(AppData* app)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context(app->glArea);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(app->glArea);

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		return FALSE;

	if (app->bReady)
	{
		app->core->draw();
		gdk_gl_drawable_swap_buffers(GDK_GL_DRAWABLE(gldrawable));
	}

	gdk_gl_drawable_gl_end(gldrawable);
	return TRUE;
}


/* HELPER: Lookup function for keypress-action. Any key that is not part of
 *         the menu system must be listed here. */
static bool handleSpecialKey(int key, int state, bool down, AppData* app)
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
			if (down) actionCaptureImage(NULL, app);
			break;
		case GDK_F11:
			k = CelestiaCore::Key_F11;
			break;
		case GDK_F12:
			k = CelestiaCore::Key_F12;
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
			app->core->keyDown(k, (state & GDK_SHIFT_MASK) 
			                   ? CelestiaCore::ShiftKey 
			                   : 0);
		else
			app->core->keyUp(k);
		return (k < 'A' || k > 'Z');
	}
	else
	{
		return false;
	}
}
