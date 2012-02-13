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

#ifndef __ENTANGLE_THUMBNAIL_LOADER_H__
#define __ENTANGLE_THUMBNAIL_LOADER_H__

#include <glib-object.h>
#include "entangle-pixbuf-loader.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_THUMBNAIL_LOADER            (entangle_thumbnail_loader_get_type ())
#define ENTANGLE_THUMBNAIL_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_THUMBNAIL_LOADER, EntangleThumbnailLoader))
#define ENTANGLE_THUMBNAIL_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_THUMBNAIL_LOADER, EntangleThumbnailLoaderClass))
#define ENTANGLE_IS_THUMBNAIL_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_THUMBNAIL_LOADER))
#define ENTANGLE_IS_THUMBNAIL_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_THUMBNAIL_LOADER))
#define ENTANGLE_THUMBNAIL_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_THUMBNAIL_LOADER, EntangleThumbnailLoaderClass))


typedef struct _EntangleThumbnailLoader EntangleThumbnailLoader;
typedef struct _EntangleThumbnailLoaderPrivate EntangleThumbnailLoaderPrivate;
typedef struct _EntangleThumbnailLoaderClass EntangleThumbnailLoaderClass;

struct _EntangleThumbnailLoader
{
    EntanglePixbufLoader parent;

    EntangleThumbnailLoaderPrivate *priv;
};

struct _EntangleThumbnailLoaderClass
{
    EntanglePixbufLoaderClass parent_class;
};


GType entangle_thumbnail_loader_get_type(void) G_GNUC_CONST;

EntangleThumbnailLoader *entangle_thumbnail_loader_new(int width,
                                               int height);

G_END_DECLS

#endif /* __ENTANGLE_THUMBNAIL_LOADER_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
