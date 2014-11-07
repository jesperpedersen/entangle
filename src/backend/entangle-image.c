/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2015 Daniel P. Berrange
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

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "entangle-debug.h"
#include "entangle-image.h"

#define ENTANGLE_IMAGE_GET_PRIVATE(obj)                                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_IMAGE, EntangleImagePrivate))

struct _EntangleImagePrivate {
    char *filename;
    GdkPixbuf *pixbuf;
    GExiv2Metadata *metadata;

    gboolean dirty;
    struct stat st;
};

G_DEFINE_TYPE(EntangleImage, entangle_image, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_FILENAME,
    PROP_PIXBUF,
    PROP_METADATA,
};

static void entangle_image_get_property(GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
    EntangleImage *picker = ENTANGLE_IMAGE(object);
    EntangleImagePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_FILENAME:
            g_value_set_string(value, priv->filename);
            break;

        case PROP_PIXBUF:
            g_value_set_object(value, priv->pixbuf);
            break;

        case PROP_METADATA:
            g_value_set_object(value, priv->metadata);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_image_set_property(GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
    EntangleImage *image = ENTANGLE_IMAGE(object);
    EntangleImagePrivate *priv = image->priv;

    switch (prop_id)
        {
        case PROP_FILENAME:
            g_free(priv->filename);
            priv->filename = g_value_dup_string(value);
            priv->dirty = TRUE;
            break;

        case PROP_PIXBUF:
            if (priv->pixbuf)
                g_object_unref(priv->pixbuf);
            priv->pixbuf = g_value_get_object(value);
            if (priv->pixbuf)
                g_object_ref(priv->pixbuf);
            break;

        case PROP_METADATA:
            if (priv->metadata)
                g_object_unref(priv->metadata);
            priv->metadata = g_value_get_object(value);
            if (priv->metadata)
                g_object_ref(priv->metadata);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_image_finalize(GObject *object)
{
    EntangleImage *image = ENTANGLE_IMAGE(object);
    EntangleImagePrivate *priv = image->priv;

    ENTANGLE_DEBUG("Finalize image %p", object);

    if (priv->pixbuf)
        g_object_unref(priv->pixbuf);
    if (priv->metadata)
        g_object_unref(priv->metadata);

    g_free(priv->filename);

    G_OBJECT_CLASS(entangle_image_parent_class)->finalize(object);
}


static void entangle_image_class_init(EntangleImageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_image_finalize;
    object_class->get_property = entangle_image_get_property;
    object_class->set_property = entangle_image_set_property;

    g_object_class_install_property(object_class,
                                    PROP_FILENAME,
                                    g_param_spec_string("filename",
                                                        "Image filename",
                                                        "Full path to image file",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PIXBUF,
                                    g_param_spec_object("pixbuf",
                                                        "Image pixbuf",
                                                        "Image pixbuf",
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_METADATA,
                                    g_param_spec_object("metadata",
                                                        "Image metadata",
                                                        "Image metadata",
                                                        GEXIV2_TYPE_METADATA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleImagePrivate));
}


/**
 * entangle_image_new_file:
 * @filename: (transfer none): the filename of the image
 *
 * Create a new image instance for the image stored in
 * @filename.
 *
 * Returns: (transfer full): the new image
 */
EntangleImage *entangle_image_new_file(const char *filename)
{
    return ENTANGLE_IMAGE(g_object_new(ENTANGLE_TYPE_IMAGE,
                                       "filename", filename,
                                       NULL));
}


/**
 * entangle_image_new_pixbuf:
 * @pixbuf: (transfer none): the pixbuf instance
 *
 * Create a new image instance for the image data stored
 * in @pixbuf
 *
 * Returns: (transfer full): the new image
 */
EntangleImage *entangle_image_new_pixbuf(GdkPixbuf *pixbuf)
{
    return ENTANGLE_IMAGE(g_object_new(ENTANGLE_TYPE_IMAGE,
                                       "pixbuf", pixbuf,
                                       NULL));
}


static void entangle_image_init(EntangleImage *image)
{
    EntangleImagePrivate *priv;

    priv = image->priv = ENTANGLE_IMAGE_GET_PRIVATE(image);

    priv->dirty = TRUE;
}


/**
 * entangle_image_get_filename:
 * @image: (transfer none): the image instance
 *
 * Get the filename associated with the image, if it has one.
 *
 * Returns: (transfer none): the filename or NULL
 */
const char *entangle_image_get_filename(EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), NULL);

    EntangleImagePrivate *priv = image->priv;
    return priv->filename;
}


static gboolean entangle_image_load(EntangleImage *image)
{
    EntangleImagePrivate *priv = image->priv;

    if (!priv->dirty)
        return TRUE;

    if (stat(priv->filename, &priv->st) < 0) {
        memset(&priv->st, 0, sizeof priv->st);
        return FALSE;
    }

    priv->dirty = FALSE;
    return TRUE;
}


/**
 * entangle_image_get_last_modified:
 * @image: (transfer none): the image instance
 *
 * Get the time at which the image was last modified, if
 * it is backed by a file on disk
 *
 * Returns: the last modification time in seconds since epoch, or 0
 */
time_t entangle_image_get_last_modified(EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), 0);

    EntangleImagePrivate *priv = image->priv;

    if (!entangle_image_load(image))
        return 0;

    return priv->st.st_mtime;
}


/**
 * entangle_image_get_file_size:
 * @image: (transfer none): the image instance
 *
 * Get the size of the image on disk, if it is backed by
 * a file on disk
 *
 * Returns: the size in bytes, or 0
 */
off_t entangle_image_get_file_size(EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), 0);

    EntangleImagePrivate *priv = image->priv;

    if (!entangle_image_load(image))
        return 0;

    return priv->st.st_size;
}


/**
 * entange_image_delete:
 * @image: (transfer none): the image instance
 *
 * Delete the file on disk.
 *
 * Returns: TRUE if the file was deleted
 */
gboolean entangle_image_delete(EntangleImage *image, GError **error)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), FALSE);

    EntangleImagePrivate *priv = image->priv;
    GFile *file = g_file_new_for_path(priv->filename);

    return g_file_delete(file, NULL, error);
}


/**
 * entangle_image_get_pixbuf:
 * @image: (transfer none): the image instance
 *
 * Get the pixbuf associated with the image, if it is available
 *
 * Returns: (transfer none): the image pixbuf or NULL
 */
GdkPixbuf *entangle_image_get_pixbuf(EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), NULL);

    EntangleImagePrivate *priv = image->priv;
    return priv->pixbuf;
}


/**
 * entangle_image_set_pixbuf:
 * @image: (transfer none): the image instance
 * @pixbuf: (transfer none): the new pixbuf
 *
 * Set the pixbuf associated with the image
 */
void entangle_image_set_pixbuf(EntangleImage *image,
                               GdkPixbuf *pixbuf)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE(image));

    g_object_set(image, "pixbuf", pixbuf, NULL);
}


/**
 * entangle_image_get_metadata:
 * @image: (transfer none): the image instance
 *
 * Get the metadata associated with the image, if it is available
 *
 * Returns: (transfer none): the image metadata or NULL
 */
GExiv2Metadata *entangle_image_get_metadata(EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), NULL);

    EntangleImagePrivate *priv = image->priv;
    return priv->metadata;
}


/**
 * entangle_image_set_metadata:
 * @image: (transfer none): the image instance
 * @metadata: (transfer none): the new metadata
 *
 * Set the metadata associated with the image
 */
void entangle_image_set_metadata(EntangleImage *image,
                                 GExiv2Metadata *metadata)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE(image));

    g_object_set(image, "metadata", metadata, NULL);
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
