/* gtkegl.h
 *
 * Copyright (C) 2021, Celestia Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#if GTK_MAJOR_VERSION >= 3
#include <gdk/gdkwayland.h>
#endif /* GTK_MAJOR_VERSION >= 3 */
#include <EGL/egl.h>
#include "gtkegl.h"

/* In Gtk2 mode we support X11 only */
#if GTK_MAJOR_VERSION == 2
# define GDK_WINDOWING_X11
# define GDK_IS_X11_DISPLAY(gdk_display)        TRUE
#endif /* GTK_MAJOR_VERSION == 2 */

struct _GtkEGLPrivate
{
    EGLDisplay *egl_display;
    EGLSurface *egl_surface;
    EGLContext *egl_context;
    gint8       require_version_major;
    gint8       require_version_minor;
    gint8       require_depth_size;
    gint8       require_stencil_size;
    gint8       require_red_size;
    gint8       require_green_size;
    gint8       require_blue_size;
    gint8       require_alpha_size;
    gint8       require_msaa_samples;
    gboolean    require_es    : 1;
    gboolean    realized      : 1;
};

typedef struct _GtkEGLPrivate GtkEGLPrivate;

static void
gtk_egl_private_init(GtkEGLPrivate *private)
{
    private->realized               = FALSE;
    private->require_es             = FALSE;
    private->require_version_major  = 0;
    private->require_version_minor  = 0;
    private->require_depth_size     = 0;
    private->require_stencil_size   = 0;
    private->require_msaa_samples   = 0;
    private->require_red_size       = 0;
    private->require_green_size     = 0;
    private->require_blue_size      = 0;
    private->require_alpha_size     = 0;
}

static void
gtk_egl_widget_realize(GtkWidget        *widget,
                       GtkEGLPrivate    *private);

static gboolean
gtk_egl_widget_configure_event(GtkWidget     *widget,
                               GdkEvent      *ev,
                               GtkEGLPrivate *private);

static void
gtk_egl_widget_size_allocate(GtkWidget      *widget,
                             GtkAllocation  *allocation,
                             GtkEGLPrivate  *private);

static void
gtk_egl_widget_unrealize(GtkWidget      *widget,
                         GtkEGLPrivate  *private);

static GQuark egl_property_quark = 0;

#define GTK_EGL_GET_PRIVATE(widget) \
        ((GtkEGLPrivate*) g_object_get_qdata(G_OBJECT(widget), egl_property_quark))

/**
 * Make EGL context attached to a widget current EGL context.
 *
 * See eglMakeCurrent for more information.
 *
 * @param widget EGL capable GtkWidget.
 */
gboolean
gtk_egl_drawable_make_current(GtkWidget *widget)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    if (private == NULL)
        return FALSE;

    eglMakeCurrent(private->egl_display,
                   private->egl_surface,
                   private->egl_surface,
                   private->egl_context);
    return TRUE;
}

/**
 * Swap back-buffers attached to a widget.
 *
 * See eglSwapBuffers for more information.
 *
 * @param widget EGL capable GtkWidget.
 */
void
gtk_egl_drawable_swap_buffers(GtkWidget *widget)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    if (private != NULL)
        eglSwapBuffers(private->egl_display, private->egl_surface);
}

/**
 * Check if a widget is EGL capable.
 *
 * @param widget a GtkWidget.
 */
gboolean
gtk_widget_is_egl_capable(GtkWidget *widget)
{
    return egl_property_quark != 0 &&
           g_object_get_qdata(G_OBJECT(widget), egl_property_quark) != NULL;
}

/**
 * Make a widget EGL capable.
 *
 * Initialize EGL subsystem and create EGL context for drawing operations.
 *
 * @param widget a GtkWidget.
 */
gboolean
gtk_widget_set_egl_capability(GtkWidget *widget)
{
    GtkEGLPrivate *private = NULL;

    if (gtk_widget_is_egl_capable(widget))
        return TRUE;

#if !GTK_CHECK_VERSION(3, 10, 0)
    /* No double buffering as we might have triple buffering */
    gtk_widget_set_double_buffered(widget, FALSE);
#endif

    private = g_new(GtkEGLPrivate, 1);
    g_assert(private != NULL);
    gtk_egl_private_init(private);

    if (egl_property_quark == 0)
        egl_property_quark = g_quark_from_static_string("egl-drawable-private");
    g_object_set_qdata(G_OBJECT(widget), egl_property_quark, private);

    /* Connect signal handlers to realize OpenGL-capable widget. */
    g_signal_connect(G_OBJECT(widget), "realize",
                     G_CALLBACK(gtk_egl_widget_realize),
                     private);

    /* gtk_widget sends configure_event when it is realized. */
    g_signal_connect(G_OBJECT(widget), "configure_event",
                     G_CALLBACK(gtk_egl_widget_configure_event),
                     private);

    /* Connect "size_allocate" signal handler to synchronize OpenGL
     * and window resizing request streams.
     */
    g_signal_connect(G_OBJECT(widget), "size_allocate",
                     G_CALLBACK(gtk_egl_widget_size_allocate),
                     private);

    g_signal_connect(G_OBJECT(widget), "unrealize",
                     G_CALLBACK(gtk_egl_widget_unrealize),
                     private);

    return TRUE;
}

/**
 * Request OpenGL ES or desktop OpenGL usage a in EGL capable widget.
 *
 * @param widget a EGL capable GtkWidget.
 * @param require_es if TRUE then use OpenGL ES, else use OpenGL.
 */
void
gtk_egl_drawable_set_require_es(GtkWidget *widget,
                                gboolean   require_es)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    g_assert(private != NULL);

    private->require_es = require_es;
}

/**
 * Request OpenGL ES version to use in a EGL capable widget.
 *
 * Currently can be called to request OpenGL ES 2.0 context
 * instead of 1.1 created by default.
 *
 * @param widget a EGL capable GtkWidget.
 * @param major major OpenGL ES version.
 * @param minor minor OpenGL ES version.
 */
void
gtk_egl_drawable_set_require_version(GtkWidget *widget,
                                     gint major,
                                     gint minor)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    g_assert(private != NULL);

    private->require_version_major = major;
    private->require_version_minor = minor;
}

/**
 * Request minimal depth buffer size.
 *
 * @param widget a EGL capable GtkWidget.
 * @param depth_size required depth buffer size in bits.
 */
void
gtk_egl_drawable_set_require_depth_size(GtkWidget *widget,
                                        gint       depth_size)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    g_assert(private != NULL);

    private->require_depth_size = depth_size;
}

/**
 * Request minimal stencil buffer size.
 *
 * @param widget a EGL capable GtkWidget.
 * @param depth_size required stencil buffer size in bits.
 */
void
gtk_egl_drawable_set_require_stencil_size(GtkWidget *widget,
                                          gint       stencil_size)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    g_assert(private != NULL);

    private->require_stencil_size = stencil_size;
}

/**
 * Request minimal color buffer sizes for R,G,B and alpha channels.
 *
 * @param widget a EGL capable GtkWidget.
 * @param red_size required red channel size in bits.
 * @param green_size required green channel size in bits.
 * @param blue_size required blue channel size in bits.
 * @param alpha_size required alpha channel size in bits.
 */
void
gtk_egl_drawable_set_require_rgba_sizes   (GtkWidget *widget,
                                           gint       red_size,
                                           gint       green_size,
                                           gint       blue_size,
                                           gint       alpha_size)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    g_assert(private != NULL);

    private->require_red_size   = red_size;
    private->require_green_size = green_size;
    private->require_blue_size  = blue_size;
    private->require_alpha_size = alpha_size;
}

/**
 * Request a MSAA capable color buffer creation.
 *
 * @param widget a EGL capable GtkWidget.
 * @param msaa_samples number of samples used by MSAA.
 */
void
gtk_egl_drawable_set_require_msaa_samples(GtkWidget *widget,
                                          gint       msaa_samples)
{
    GtkEGLPrivate *private = GTK_EGL_GET_PRIVATE(widget);
    g_assert(private != NULL);

    private->require_msaa_samples = msaa_samples;
}

#define MIN_ATTR_LIST_SIZE 20
static void
gtk_egl_build_screen_attributes_list(GtkEGLPrivate *private,
                                     EGLint        *attributes,
                                     int            count)
{
    g_assert(count >= MIN_ATTR_LIST_SIZE);

    int i = 0;
    attributes[i++] = EGL_RENDERABLE_TYPE;
    if (private->require_es)
    {
        if (private->require_version_major == 2)
            attributes[i] = EGL_OPENGL_ES2_BIT;
        else
            attributes[i] = EGL_OPENGL_ES_BIT;
    }
    else
    {
        attributes[i] = EGL_OPENGL_BIT;
    }
    i++;

    if (private->require_depth_size > 0)
    {
        attributes[i++] = EGL_DEPTH_SIZE;
        attributes[i++] = private->require_depth_size;
    }

    if (private->require_stencil_size > 0)
    {
        attributes[i++] = EGL_STENCIL_SIZE;
        attributes[i++] = private->require_stencil_size;
    }

    if (private->require_msaa_samples > 0)
    {
        attributes[i++] = EGL_SAMPLE_BUFFERS;
        attributes[i++] = 1;
        attributes[i++] = EGL_SAMPLES;
        attributes[i++] = private->require_msaa_samples;
    }

    {
        attributes[i++] = EGL_ALPHA_SIZE;
        attributes[i++] = 8;
        attributes[i++] = EGL_RED_SIZE;
        attributes[i++] = 8;
        attributes[i++] = EGL_GREEN_SIZE;
        attributes[i++] = 8;
        attributes[i++] = EGL_BLUE_SIZE;
        attributes[i++] = 8;
    }

    attributes[i] = EGL_NONE;
    g_assert(i < MIN_ATTR_LIST_SIZE);
}

static void
gtk_egl_build_context_attributes_list(GtkEGLPrivate *private,
                                      EGLint        *attributes,
                                      int            count)
{
    g_assert(count >= 3);
    int i = 0;
    if (private->require_es && private->require_version_major == 2)
    {
        attributes[0] = EGL_CONTEXT_CLIENT_VERSION;
        attributes[1] = 2;
        i = 2;
    }
    attributes[i] = EGL_NONE;
}

/* drawing area signal handlers */
static void
gtk_egl_widget_realize(GtkWidget        *widget,
                       GtkEGLPrivate    *private)
{
    EGLConfig egl_config;
    EGLint n_config;
    EGLint attributes[MIN_ATTR_LIST_SIZE] = {EGL_NONE};

    if (private->realized)
        return;

    gtk_egl_build_screen_attributes_list(private, attributes, sizeof(attributes));

    GdkWindow  *gdk_window  = gtk_widget_get_window(widget);
    g_assert(gdk_window != NULL);
    GdkDisplay *gdk_display = gtk_widget_get_display(widget);
    g_assert(gdk_display != NULL);

    EGLDisplay *egl_display = NULL;
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_display))
        egl_display = eglGetDisplay((EGLNativeDisplayType) GDK_DISPLAY_XDISPLAY(gdk_display));
#endif /* GDK_WINDOWING_X11 */
#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(gdk_display))
        egl_display = eglGetDisplay((EGLNativeDisplayType) gdk_wayland_display_get_wl_display(gdk_display));
#endif /* GDK_WINDOWING_WAYLAND */

    if (egl_display == NULL)
    {
        g_print("eglGetDisplay() returned NULL or was not called!\n");
        return;
    }

    if (!eglInitialize(egl_display, NULL, NULL))
    {
        g_print("eglInitialize() failed!\n");
        return;
    }

    if (!eglChooseConfig(egl_display, attributes, &egl_config, 1, &n_config))
    {
        g_print("eglChooseConfig() failed!\n");
        return;
    }

    EGLenum api = private->require_es ? EGL_OPENGL_ES_API : EGL_OPENGL_API;
    if (!eglBindAPI(api))
    {
        g_print("eglBindAPI(0x%X) failed!\n", api);
        return;
    }

    EGLSurface *egl_surface = eglCreateWindowSurface(egl_display, egl_config, GDK_WINDOW_XID(gdk_window), NULL);

    gtk_egl_build_context_attributes_list(private, attributes, sizeof(attributes));
    EGLContext *egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, attributes);

    private->egl_display = egl_display;
    private->egl_surface = egl_surface;
    private->egl_context = egl_context;
    private->realized    = TRUE;
}

static gboolean
gtk_egl_widget_configure_event(GtkWidget     *widget,
                               GdkEvent      *ev,
                               GtkEGLPrivate *private)
{
    /* Realize if OpenGL-capable window is not realized yet */
    if (!private->realized)
        gtk_egl_widget_realize(widget, private);

    return FALSE;
}

static void
gtk_egl_widget_size_allocate(GtkWidget     *widget,
                             GtkAllocation *allocation,
                             GtkEGLPrivate *private)
{
    /* Synchronize OpenGL and window resizing request streams. */
    if (gtk_widget_get_realized(widget) && private->realized)
        eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}

static void
gtk_egl_widget_unrealize(GtkWidget     *widget,
                         GtkEGLPrivate *private)
{
    eglMakeCurrent(private->egl_display,
                   private->egl_surface,
                   private->egl_surface,
                   private->egl_context);

    eglTerminate(private->egl_display);
    g_object_set_qdata(G_OBJECT(widget), egl_property_quark, NULL);
    g_free(private);
}
