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

#ifndef __CAPA_THUMBNAIL_LOADER_H__
#define __CAPA_THUMBNAIL_LOADER_H__

#include <glib-object.h>
#include "capa-pixbuf-loader.h"

G_BEGIN_DECLS

#define CAPA_TYPE_THUMBNAIL_LOADER            (capa_thumbnail_loader_get_type ())
#define CAPA_THUMBNAIL_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_THUMBNAIL_LOADER, CapaThumbnailLoader))
#define CAPA_THUMBNAIL_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_THUMBNAIL_LOADER, CapaThumbnailLoaderClass))
#define CAPA_IS_THUMBNAIL_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_THUMBNAIL_LOADER))
#define CAPA_IS_THUMBNAIL_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_THUMBNAIL_LOADER))
#define CAPA_THUMBNAIL_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_THUMBNAIL_LOADER, CapaThumbnailLoaderClass))


typedef struct _CapaThumbnailLoader CapaThumbnailLoader;
typedef struct _CapaThumbnailLoaderPrivate CapaThumbnailLoaderPrivate;
typedef struct _CapaThumbnailLoaderClass CapaThumbnailLoaderClass;

struct _CapaThumbnailLoader
{
    CapaPixbufLoader parent;

    CapaThumbnailLoaderPrivate *priv;
};

struct _CapaThumbnailLoaderClass
{
    CapaPixbufLoaderClass parent_class;
};


GType capa_thumbnail_loader_get_type(void) G_GNUC_CONST;

CapaThumbnailLoader *capa_thumbnail_loader_new(int width,
                                               int height);

G_END_DECLS

#endif /* __CAPA_THUMBNAIL_LOADER_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
