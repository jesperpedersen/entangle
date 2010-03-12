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

#include <config.h>

#include "capa-debug.h"
#include "capa-image-loader.h"

G_DEFINE_TYPE(CapaImageLoader, capa_image_loader, CAPA_TYPE_PIXBUF_LOADER);


static GdkPixbuf *capa_image_loader_pixbuf_load(CapaPixbufLoader *loader G_GNUC_UNUSED,
                                                const char *filename)
{
    GdkPixbuf *master = gdk_pixbuf_new_from_file(filename, NULL);
    GdkPixbuf *result;

    if (!master)
        return NULL;

    result = gdk_pixbuf_apply_embedded_orientation(master);

    g_object_unref(master);

    return result;
}


static void capa_image_loader_class_init(CapaImageLoaderClass *klass)
{
    CapaPixbufLoaderClass *loader_class = CAPA_PIXBUF_LOADER_CLASS(klass);

    loader_class->pixbuf_load = capa_image_loader_pixbuf_load;
}


CapaImageLoader *capa_image_loader_new(void)
{
    return CAPA_IMAGE_LOADER(g_object_new(CAPA_TYPE_IMAGE_LOADER,
                                          NULL));
}


static void capa_image_loader_init(CapaImageLoader *loader G_GNUC_UNUSED)
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
