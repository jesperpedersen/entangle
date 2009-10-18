/*
 *  Capa: Capa Assists Photograph Aquisition
 *
 *  Copyright (C) 2009 Daniel P. Berrange
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

#ifndef __CAPA_IMAGE_DISPLAY_H__
#define __CAPA_IMAGE_DISPLAY_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CAPA_TYPE_IMAGE_DISPLAY            (capa_image_display_get_type ())
#define CAPA_IMAGE_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_IMAGE_DISPLAY, CapaImageDisplay))
#define CAPA_IMAGE_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_IMAGE_DISPLAY, CapaImageDisplayClass))
#define CAPA_IS_IMAGE_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_IMAGE_DISPLAY))
#define CAPA_IS_IMAGE_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_IMAGE_DISPLAY))
#define CAPA_IMAGE_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_IMAGE_DISPLAY, CapaImageDisplayClass))


typedef struct _CapaImageDisplay CapaImageDisplay;
typedef struct _CapaImageDisplayPrivate CapaImageDisplayPrivate;
typedef struct _CapaImageDisplayClass CapaImageDisplayClass;

struct _CapaImageDisplay
{
    GtkDrawingArea parent;

    CapaImageDisplayPrivate *priv;
};

struct _CapaImageDisplayClass
{
    GtkDrawingAreaClass parent_class;

};

GType capa_image_display_get_type(void) G_GNUC_CONST;

CapaImageDisplay* capa_image_display_new(void);

G_END_DECLS

#endif /* __CAPA_IMAGE_DISPLAY_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
