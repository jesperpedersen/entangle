/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#include <config.h>

#include "entangle-debug.h"
#include "entangle-image-loader.h"

G_DEFINE_TYPE(EntangleImageLoader, entangle_image_loader, ENTANGLE_TYPE_PIXBUF_LOADER);


static GdkPixbuf *entangle_image_loader_pixbuf_load(EntanglePixbufLoader *loader G_GNUC_UNUSED,
                                                    EntangleImage *image)
{
    GdkPixbuf *master = gdk_pixbuf_new_from_file(entangle_image_get_filename(image), NULL);
    GdkPixbuf *result;

    if (!master)
        return NULL;

    result = gdk_pixbuf_apply_embedded_orientation(master);

    g_object_unref(master);

    return result;
}


static void entangle_image_loader_class_init(EntangleImageLoaderClass *klass)
{
    EntanglePixbufLoaderClass *loader_class = ENTANGLE_PIXBUF_LOADER_CLASS(klass);

    loader_class->pixbuf_load = entangle_image_loader_pixbuf_load;
}


EntangleImageLoader *entangle_image_loader_new(void)
{
    return ENTANGLE_IMAGE_LOADER(g_object_new(ENTANGLE_TYPE_IMAGE_LOADER,
                                          NULL));
}


static void entangle_image_loader_init(EntangleImageLoader *loader G_GNUC_UNUSED)
{
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
