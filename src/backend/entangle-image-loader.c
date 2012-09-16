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

#include <config.h>

#include "entangle-debug.h"
#include "entangle-pixbuf.h"
#include "entangle-image-loader.h"

G_DEFINE_TYPE(EntangleImageLoader, entangle_image_loader, ENTANGLE_TYPE_PIXBUF_LOADER);


static GdkPixbuf *entangle_image_loader_pixbuf_load(EntanglePixbufLoader *loader G_GNUC_UNUSED,
                                                    EntangleImage *image)
{
    return entangle_pixbuf_open_image(image,
                                      ENTANGLE_PIXBUF_IMAGE_SLOT_PREVIEW);
}

static GExiv2Metadata *entangle_image_loader_metadata_load(EntanglePixbufLoader *loader G_GNUC_UNUSED,
                                                           EntangleImage *image)
{
    GExiv2Metadata *metadata = gexiv2_metadata_new();

    if (!gexiv2_metadata_open_path(metadata, entangle_image_get_filename(image), NULL)) {
        g_object_unref(metadata);
        metadata = NULL;
    }

    return metadata;
}


static void entangle_image_loader_class_init(EntangleImageLoaderClass *klass)
{
    EntanglePixbufLoaderClass *loader_class = ENTANGLE_PIXBUF_LOADER_CLASS(klass);

    loader_class->pixbuf_load = entangle_image_loader_pixbuf_load;
    loader_class->metadata_load = entangle_image_loader_metadata_load;
}


EntangleImageLoader *entangle_image_loader_new(void)
{
    return ENTANGLE_IMAGE_LOADER(g_object_new(ENTANGLE_TYPE_IMAGE_LOADER,
                                              "with-metadata", TRUE,
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
