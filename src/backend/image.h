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

#ifndef __CAPA_IMAGE_H__
#define __CAPA_IMAGE_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "control-group.h"

G_BEGIN_DECLS

#define CAPA_TYPE_IMAGE            (capa_image_get_type ())
#define CAPA_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_IMAGE, CapaImage))
#define CAPA_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_IMAGE, CapaImageClass))
#define CAPA_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_IMAGE))
#define CAPA_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_IMAGE))
#define CAPA_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_IMAGE, CapaImageClass))


typedef struct _CapaImage CapaImage;
typedef struct _CapaImagePrivate CapaImagePrivate;
typedef struct _CapaImageClass CapaImageClass;

struct _CapaImage
{
    GObject parent;

    CapaImagePrivate *priv;
};

struct _CapaImageClass
{
    GObjectClass parent_class;
};


GType capa_image_get_type(void) G_GNUC_CONST;

CapaImage *capa_image_new(const char *filename);

const char *capa_image_filename(CapaImage *image);

gboolean capa_image_load(CapaImage *image);

time_t capa_image_last_modified(CapaImage *image);
off_t capa_image_file_size(CapaImage *image);

GdkPixbuf *capa_image_thumbnail(CapaImage *image);

G_END_DECLS

#endif /* __CAPA_IMAGE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
