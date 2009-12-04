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

#ifndef __CAPA_IMAGE_LOADER_H__
#define __CAPA_IMAGE_LOADER_H__

#include "pixbuf-loader.h"

G_BEGIN_DECLS

#define CAPA_TYPE_IMAGE_LOADER            (capa_image_loader_get_type ())
#define CAPA_IMAGE_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_IMAGE_LOADER, CapaImageLoader))
#define CAPA_IMAGE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_IMAGE_LOADER, CapaImageLoaderClass))
#define CAPA_IS_IMAGE_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_IMAGE_LOADER))
#define CAPA_IS_IMAGE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_IMAGE_LOADER))
#define CAPA_IMAGE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_IMAGE_LOADER, CapaImageLoaderClass))


typedef struct _CapaImageLoader CapaImageLoader;
typedef struct _CapaImageLoaderClass CapaImageLoaderClass;

struct _CapaImageLoader
{
    CapaPixbufLoader parent;
};

struct _CapaImageLoaderClass
{
    CapaPixbufLoaderClass parent_class;
};


GType capa_image_loader_get_type(void) G_GNUC_CONST;

CapaImageLoader *capa_image_loader_new(void);


G_END_DECLS

#endif /* __CAPA_IMAGE_LOADER_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
