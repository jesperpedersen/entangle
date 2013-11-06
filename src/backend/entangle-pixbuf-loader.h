/*
 *  Entangle: Tethered Camera Control & Capture
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

#ifndef __ENTANGLE_PIXBUF_LOADER_H__
#define __ENTANGLE_PIXBUF_LOADER_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gexiv2/gexiv2.h>

#include "entangle-colour-profile.h"
#include "entangle-image.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_PIXBUF_LOADER            (entangle_pixbuf_loader_get_type ())
#define ENTANGLE_PIXBUF_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PIXBUF_LOADER, EntanglePixbufLoader))
#define ENTANGLE_PIXBUF_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_PIXBUF_LOADER, EntanglePixbufLoaderClass))
#define ENTANGLE_IS_PIXBUF_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PIXBUF_LOADER))
#define ENTANGLE_IS_PIXBUF_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_PIXBUF_LOADER))
#define ENTANGLE_PIXBUF_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_PIXBUF_LOADER, EntanglePixbufLoaderClass))


typedef struct _EntanglePixbufLoader EntanglePixbufLoader;
typedef struct _EntanglePixbufLoaderPrivate EntanglePixbufLoaderPrivate;
typedef struct _EntanglePixbufLoaderClass EntanglePixbufLoaderClass;

struct _EntanglePixbufLoader
{
    GObject parent;

    EntanglePixbufLoaderPrivate *priv;
};

struct _EntanglePixbufLoaderClass
{
    GObjectClass parent_class;

    void (*pixbuf_loaded)(EntanglePixbufLoader *loader, EntangleImage *image);
    void (*metadata_loaded)(EntanglePixbufLoader *loader, EntangleImage *image);
    void (*pixbuf_unloaded)(EntanglePixbufLoader *loader, EntangleImage *image);
    void (*metadata_unloaded)(EntanglePixbufLoader *loader, EntangleImage *image);

    GdkPixbuf *(*pixbuf_load)(EntanglePixbufLoader *loader, EntangleImage *image,
                              GExiv2Metadata **metadata);
};


GType entangle_pixbuf_loader_get_type(void) G_GNUC_CONST;

gboolean entangle_pixbuf_loader_is_ready(EntanglePixbufLoader *loader,
                                         EntangleImage *image);
GdkPixbuf *entangle_pixbuf_loader_get_pixbuf(EntanglePixbufLoader *loader,
                                             EntangleImage *image);
GExiv2Metadata *entangle_pixbuf_loader_get_metadata(EntanglePixbufLoader *loader,
                                                    EntangleImage *image);

gboolean entangle_pixbuf_loader_load(EntanglePixbufLoader *loader,
                                     EntangleImage *image);

gboolean entangle_pixbuf_loader_unload(EntanglePixbufLoader *loader,
                                       EntangleImage *image);

void entangle_pixbuf_loader_trigger_reload(EntanglePixbufLoader *loader);

void entangle_pixbuf_loader_set_colour_transform(EntanglePixbufLoader *loader,
                                                 EntangleColourProfileTransform *transform);

EntangleColourProfileTransform *entangle_pixbuf_loader_get_colour_transform(EntanglePixbufLoader *loader);

void entangle_pixbuf_loader_set_workers(EntanglePixbufLoader *loader,
                                        int count);

int entangle_pixbuf_loader_get_workers(EntanglePixbufLoader *loader);

G_END_DECLS

#endif /* __ENTANGLE_PIXBUF_LOADER_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
