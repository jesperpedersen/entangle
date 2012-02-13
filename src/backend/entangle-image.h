/*
 *  Entangle: Entangle Assists Photograph Aquisition
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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

#ifndef __ENTANGLE_IMAGE_H__
#define __ENTANGLE_IMAGE_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gexiv2.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "entangle-control-group.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_IMAGE            (entangle_image_get_type ())
#define ENTANGLE_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_IMAGE, EntangleImage))
#define ENTANGLE_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_IMAGE, EntangleImageClass))
#define ENTANGLE_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_IMAGE))
#define ENTANGLE_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_IMAGE))
#define ENTANGLE_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_IMAGE, EntangleImageClass))


typedef struct _EntangleImage EntangleImage;
typedef struct _EntangleImagePrivate EntangleImagePrivate;
typedef struct _EntangleImageClass EntangleImageClass;

struct _EntangleImage
{
    GObject parent;

    EntangleImagePrivate *priv;
};

struct _EntangleImageClass
{
    GObjectClass parent_class;
};


GType entangle_image_get_type(void) G_GNUC_CONST;

EntangleImage *entangle_image_new_file(const char *filename);
EntangleImage *entangle_image_new_pixbuf(GdkPixbuf *pixbuf);

const char *entangle_image_get_filename(EntangleImage *image);

time_t entangle_image_get_last_modified(EntangleImage *image);
off_t entangle_image_get_file_size(EntangleImage *image);

GdkPixbuf *entangle_image_get_pixbuf(EntangleImage *image);
void entangle_image_set_pixbuf(EntangleImage *image,
                               GdkPixbuf *pixbuf);

GExiv2Metadata *entangle_image_get_metadata(EntangleImage *image);
void entangle_image_set_metadata(EntangleImage *image,
                                 GExiv2Metadata *metadata);

G_END_DECLS

#endif /* __ENTANGLE_IMAGE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
