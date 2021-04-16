/* gtkegl.h
 *
 * Copyright (C) 2021, Celestia Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#pragma once

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

gboolean
gtk_egl_drawable_make_current             (GtkWidget *drawing_area);

void
gtk_egl_drawable_swap_buffers             (GtkWidget *drawing_area);

gboolean
gtk_widget_is_egl_capable                 (GtkWidget *drawing_area);

gboolean
gtk_widget_set_egl_capability             (GtkWidget *drawing_area);

void
gtk_egl_drawable_set_require_es           (GtkWidget *drawing_area,
                                           gboolean   require_es);

void
gtk_egl_drawable_set_require_version      (GtkWidget *drawing_area,
                                           gint major,
                                           gint minor);

void
gtk_egl_drawable_set_require_depth_size   (GtkWidget *drawing_area,
                                           gint       depth_size);

void
gtk_egl_drawable_set_require_stencil_size (GtkWidget *drawing_area,
                                           gint       stencil_size);

void
gtk_egl_drawable_set_require_rgba_sizes   (GtkWidget *drawing_area,
                                           gint       red_size,
                                           gint       green_size,
                                           gint       blue_size,
                                           gint       alpha_size);

void
gtk_egl_drawable_set_require_msaa_samples (GtkWidget *drawing_area,
                                           gint       msaa_samples);

#ifdef __cplusplus
}
#endif
