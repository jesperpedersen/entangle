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

#define ENTANGLE_IMAGE_LOADER_GET_PRIVATE(obj)                          \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_IMAGE_LOADER, EntangleImageLoaderPrivate))

struct _EntangleImageLoaderPrivate {
    gboolean embeddedPreview;
};

G_DEFINE_TYPE(EntangleImageLoader, entangle_image_loader, ENTANGLE_TYPE_PIXBUF_LOADER);


enum {
    PROP_0,
    PROP_EMBEDDED_PREVIEW,
};


static void entangle_image_loader_get_property(GObject *object,
                                               guint prop_id,
                                               GValue *value,
                                               GParamSpec *pspec)
{
    EntangleImageLoader *loader = ENTANGLE_IMAGE_LOADER(object);
    EntangleImageLoaderPrivate *priv = loader->priv;

    switch (prop_id)
        {
        case PROP_EMBEDDED_PREVIEW:
            g_value_set_boolean(value, priv->embeddedPreview);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_image_loader_set_property(GObject *object,
                                               guint prop_id,
                                               const GValue *value,
                                               GParamSpec *pspec)
{
    EntangleImageLoader *loader = ENTANGLE_IMAGE_LOADER(object);

    switch (prop_id)
        {
        case PROP_EMBEDDED_PREVIEW:
            entangle_image_loader_set_embedded_preview(loader, g_value_get_boolean(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static GdkPixbuf *entangle_image_loader_pixbuf_load(EntanglePixbufLoader *loader G_GNUC_UNUSED,
                                                    EntangleImage *image,
                                                    GExiv2Metadata **metadata)
{
    EntangleImageLoaderPrivate *priv = (ENTANGLE_IMAGE_LOADER(loader))->priv;
    g_printerr("Preview %d\n", priv->embeddedPreview);
    if (priv->embeddedPreview)
        return entangle_pixbuf_open_image(image,
                                          ENTANGLE_PIXBUF_IMAGE_SLOT_PREVIEW,
                                          TRUE,
                                          metadata);
    else
        return entangle_pixbuf_open_image(image,
                                          ENTANGLE_PIXBUF_IMAGE_SLOT_MASTER,
                                          TRUE,
                                          metadata);
}


static void entangle_image_loader_class_init(EntangleImageLoaderClass *klass)
{
    EntanglePixbufLoaderClass *loader_class = ENTANGLE_PIXBUF_LOADER_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = entangle_image_loader_get_property;
    object_class->set_property = entangle_image_loader_set_property;

    g_object_class_install_property(object_class,
                                    PROP_EMBEDDED_PREVIEW,
                                    g_param_spec_boolean("embedded-preview",
                                                         "Embedded preview",
                                                         "Use embedded preview",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    loader_class->pixbuf_load = entangle_image_loader_pixbuf_load;

    g_type_class_add_private(klass, sizeof(EntangleImageLoaderPrivate));
}


EntangleImageLoader *entangle_image_loader_new(void)
{
    return ENTANGLE_IMAGE_LOADER(g_object_new(ENTANGLE_TYPE_IMAGE_LOADER,
                                              "with-metadata", TRUE,
                                              NULL));
}


static void entangle_image_loader_init(EntangleImageLoader *loader)
{
    EntangleImageLoaderPrivate *priv;

    priv = loader->priv = ENTANGLE_IMAGE_LOADER_GET_PRIVATE(loader);

    priv->embeddedPreview = TRUE;
}


gboolean entangle_image_loader_get_embedded_preview(EntangleImageLoader *loader)
{
    EntangleImageLoaderPrivate *priv = loader->priv;
    return priv->embeddedPreview;
}

void entangle_image_loader_set_embedded_preview(EntangleImageLoader *loader, gboolean enable)
{
    EntangleImageLoaderPrivate *priv = loader->priv;
    g_printerr("Set preview %d\n", enable);
    priv->embeddedPreview = enable;
    entangle_pixbuf_loader_trigger_reload(ENTANGLE_PIXBUF_LOADER(loader));
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
