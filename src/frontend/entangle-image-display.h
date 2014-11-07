/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2015 Daniel P. Berrange
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __ENTANGLE_IMAGE_DISPLAY_H__
#define __ENTANGLE_IMAGE_DISPLAY_H__

#include <gtk/gtk.h>

#include "entangle-image-loader.h"

#include "entangle-image-display-enums.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_IMAGE_DISPLAY            (entangle_image_display_get_type ())
#define ENTANGLE_IMAGE_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_IMAGE_DISPLAY, EntangleImageDisplay))
#define ENTANGLE_IMAGE_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_IMAGE_DISPLAY, EntangleImageDisplayClass))
#define ENTANGLE_IS_IMAGE_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_IMAGE_DISPLAY))
#define ENTANGLE_IS_IMAGE_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_IMAGE_DISPLAY))
#define ENTANGLE_IMAGE_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_IMAGE_DISPLAY, EntangleImageDisplayClass))


typedef struct _EntangleImageDisplay EntangleImageDisplay;
typedef struct _EntangleImageDisplayPrivate EntangleImageDisplayPrivate;
typedef struct _EntangleImageDisplayClass EntangleImageDisplayClass;

struct _EntangleImageDisplay
{
    GtkDrawingArea parent;

    EntangleImageDisplayPrivate *priv;
};

struct _EntangleImageDisplayClass
{
    GtkDrawingAreaClass parent_class;

};

GType entangle_image_display_get_type(void) G_GNUC_CONST;

EntangleImageDisplay* entangle_image_display_new(void);

void entangle_image_display_set_image(EntangleImageDisplay *display,
                                      EntangleImage *image);
EntangleImage *entangle_image_display_get_image(EntangleImageDisplay *display);

void entangle_image_display_set_background(EntangleImageDisplay *display,
                                           const gchar *background);
gchar *entangle_image_display_get_background(EntangleImageDisplay *display);

void entangle_image_display_set_image_list(EntangleImageDisplay *display,
                                           GList *images);
GList *entangle_image_display_get_image_list(EntangleImageDisplay *display);

void entangle_image_display_set_autoscale(EntangleImageDisplay *displsy,
                                          gboolean autoscale);
gboolean entangle_image_display_get_autoscale(EntangleImageDisplay *display);


void entangle_image_display_set_scale(EntangleImageDisplay *display,
                                      gdouble scale);
gdouble entangle_image_display_get_scale(EntangleImageDisplay *display);

void entangle_image_display_set_aspect_ratio(EntangleImageDisplay *display,
                                             gdouble aspect);
gdouble entangle_image_display_get_aspect_ratio(EntangleImageDisplay *display);

void entangle_image_display_set_mask_opacity(EntangleImageDisplay *display,
                                             gdouble opacity);
gdouble entangle_image_display_get_mask_opacity(EntangleImageDisplay *display);

void entangle_image_display_set_mask_enabled(EntangleImageDisplay *display,
                                             gboolean enabled);
gboolean entangle_image_display_get_mask_enabled(EntangleImageDisplay *display);

void entangle_image_display_set_focus_point(EntangleImageDisplay *display,
                                            gboolean enabled);
gboolean entangle_image_display_get_focus_point(EntangleImageDisplay *display);

gboolean entangle_image_display_get_loaded(EntangleImageDisplay *display);

typedef enum {
    ENTANGLE_IMAGE_DISPLAY_GRID_NONE,
    ENTANGLE_IMAGE_DISPLAY_GRID_CENTER_LINES,
    ENTANGLE_IMAGE_DISPLAY_GRID_RULE_OF_3RDS,
    ENTANGLE_IMAGE_DISPLAY_GRID_QUARTERS,
    ENTANGLE_IMAGE_DISPLAY_GRID_RULE_OF_5THS,
    ENTANGLE_IMAGE_DISPLAY_GRID_GOLDEN_SECTIONS,
} EntangleImageDisplayGrid;

void entangle_image_display_set_grid_display(EntangleImageDisplay *display,
                                             EntangleImageDisplayGrid mode);
EntangleImageDisplayGrid entangle_image_display_get_grid_display(EntangleImageDisplay *display);

G_END_DECLS

#endif /* __ENTANGLE_IMAGE_DISPLAY_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
