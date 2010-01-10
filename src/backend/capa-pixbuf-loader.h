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

#ifndef __CAPA_PIXBUF_LOADER_H__
#define __CAPA_PIXBUF_LOADER_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define CAPA_TYPE_PIXBUF_LOADER            (capa_pixbuf_loader_get_type ())
#define CAPA_PIXBUF_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PIXBUF_LOADER, CapaPixbufLoader))
#define CAPA_PIXBUF_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_PIXBUF_LOADER, CapaPixbufLoaderClass))
#define CAPA_IS_PIXBUF_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PIXBUF_LOADER))
#define CAPA_IS_PIXBUF_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_PIXBUF_LOADER))
#define CAPA_PIXBUF_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_PIXBUF_LOADER, CapaPixbufLoaderClass))


typedef struct _CapaPixbufLoader CapaPixbufLoader;
typedef struct _CapaPixbufLoaderPrivate CapaPixbufLoaderPrivate;
typedef struct _CapaPixbufLoaderClass CapaPixbufLoaderClass;

struct _CapaPixbufLoader
{
    GObject parent;

    CapaPixbufLoaderPrivate *priv;
};

struct _CapaPixbufLoaderClass
{
    GObjectClass parent_class;

    void (*pixbuf_loaded)(CapaPixbufLoader *loader, const char *filename);

    GdkPixbuf *(*pixbuf_load)(CapaPixbufLoader *loader, const char *filename);
};


GType capa_pixbuf_loader_get_type(void) G_GNUC_CONST;

CapaPixbufLoader *capa_pixbuf_loader_new(void);


gboolean capa_pixbuf_loader_is_ready(CapaPixbufLoader *loader,
                                     const char *filename);

GdkPixbuf *capa_pixbuf_loader_get_pixbuf(CapaPixbufLoader *loader,
                                         const char *filename);

gboolean capa_pixbuf_loader_load(CapaPixbufLoader *loader,
                                 const char *filename);

gboolean capa_pixbuf_loader_unload(CapaPixbufLoader *loader,
                                   const char *filename);


G_END_DECLS

#endif /* __CAPA_PIXBUF_LOADER_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
