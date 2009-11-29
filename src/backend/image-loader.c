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

#include <stdio.h>
#include <string.h>

#include "internal.h"
#include "image-loader.h"
#include "colour-profile.h"

#define CAPA_IMAGE_LOADER_GET_PRIVATE(obj)                                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_IMAGE_LOADER, CapaImageLoaderPrivate))

typedef struct _CapaImageLoaderEntry {
    int refs;
    char *filename;
    gboolean pending;
    gboolean processing;
    gboolean ready;
    GdkPixbuf *pixbuf;
} CapaImageLoaderEntry;

typedef struct _CapaImagerLoaderResult {
    CapaImageLoader *loader;
    const char *filename;
    GdkPixbuf *pixbuf;
} CapaImageLoaderResult;

struct _CapaImageLoaderPrivate {
    GThreadPool *workers;
    CapaColourProfileTransform *colourTransform;

    GMutex *lock;
    GHashTable *images;
};

G_DEFINE_TYPE(CapaImageLoader, capa_image_loader, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_NTHREADS,
    PROP_COLOUR_TRANSFORM,
};


static void capa_image_loader_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
    CapaImageLoader *loader = CAPA_IMAGE_LOADER(object);
    CapaImageLoaderPrivate *priv = loader->priv;

    switch (prop_id)
        {
        case PROP_NTHREADS:
            g_value_set_int(value, g_thread_pool_get_max_threads(priv->workers));
            break;

        case PROP_COLOUR_TRANSFORM:
            g_value_set_object(value, priv->colourTransform);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_image_loader_trigger_reload(CapaImageLoader *loader)
{
    CapaImageLoaderPrivate *priv = loader->priv;
    GHashTableIter iter;
    gpointer key, value;

    CAPA_DEBUG("Triggering mass reload");

    g_mutex_lock(priv->lock);
    g_hash_table_iter_init(&iter, priv->images);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        CapaImageLoaderEntry *entry = value;
        if (entry->refs &&
            !entry->processing)
            g_thread_pool_push(priv->workers, entry->filename, NULL);
    }
    g_mutex_unlock(priv->lock);
}

static void capa_image_loader_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
    CapaImageLoader *loader = CAPA_IMAGE_LOADER(object);
    CapaImageLoaderPrivate *priv = loader->priv;

    switch (prop_id)
        {
        case PROP_NTHREADS:
            g_thread_pool_set_max_threads(priv->workers, g_value_get_int(value), NULL);
            break;

        case PROP_COLOUR_TRANSFORM:
            g_mutex_lock(priv->lock);
            if (priv->colourTransform)
                g_object_unref(G_OBJECT(priv->colourTransform));
            priv->colourTransform = g_value_get_object(value);
            if (priv->colourTransform)
                g_object_ref(G_OBJECT(priv->colourTransform));
            g_mutex_unlock(priv->lock);
            capa_image_loader_trigger_reload(loader);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_image_loader_entry_free(gpointer opaque)
{
    CapaImageLoaderEntry *entry = opaque;

    CAPA_DEBUG("free entry %p %s", entry, entry->filename);
    g_free(entry->filename);
    if (entry->pixbuf)
        g_object_unref(G_OBJECT(entry->pixbuf));
    g_free(entry);
}


static CapaImageLoaderEntry *capa_image_loader_entry_new(const char *filename)
{
    CapaImageLoaderEntry *entry;

    entry = g_new0(CapaImageLoaderEntry, 1);
    entry->filename = g_strdup(filename);
    entry->refs = 1;

    CAPA_DEBUG("new entry %p %s", entry, filename);

    return entry;
}


static gboolean capa_image_loader_result(gpointer data)
{
    CapaImageLoaderResult *result = data;
    CapaImageLoader *loader = result->loader;
    CapaImageLoaderPrivate *priv = loader->priv;
    CapaImageLoaderEntry *entry;
    char *filename;

    CAPA_DEBUG("result %p %s %p", loader, result->filename, result->pixbuf);

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->images, result->filename);
    if (!entry) {
        g_mutex_unlock(priv->lock);
        if (result->pixbuf)
            g_object_unref(G_OBJECT(result->pixbuf));
        g_free(result);
        return FALSE;
    }
    filename = g_strdup(result->filename);
    if (entry->pixbuf)
        g_object_unref(entry->pixbuf);
    entry->pixbuf = result->pixbuf;
    entry->ready = TRUE;
    entry->processing = FALSE;

    if (entry->refs) {
        g_mutex_unlock(priv->lock);
        g_signal_emit_by_name(G_OBJECT(loader), "image-loaded", filename);
    } else if (!entry->pending) {
        g_hash_table_remove(priv->images, entry->filename);
        g_mutex_unlock(priv->lock);
    }

    g_free(filename);
    g_free(result);

    return FALSE;
}

static void capa_image_loader_worker(gpointer data,
                                     gpointer opaque)
{
    CapaImageLoader *loader = opaque;
    CapaImageLoaderPrivate *priv = loader->priv;
    const char *filename = data;
    CapaImageLoaderResult *result = g_new0(CapaImageLoaderResult, 1);
    CapaColourProfileTransform *transform;
    CapaImageLoaderEntry *entry;
    GdkPixbuf *pixbuf;

    CAPA_DEBUG("worker %p %s", loader, filename);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->images, filename);
    if (!entry)
        goto cleanup;
    if (entry->refs == 0) {
        CAPA_DEBUG("image already removed");
        g_hash_table_remove(priv->images, filename);
        goto cleanup;
    }
    entry->pending = FALSE;
    entry->processing = TRUE;

    transform = priv->colourTransform;
    g_object_ref(G_OBJECT(transform));
    g_mutex_unlock(priv->lock);

    pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    if (pixbuf) {
        if (transform) {
            result->pixbuf = capa_colour_profile_transform_apply(transform,
                                                                 pixbuf);
            g_object_unref(G_OBJECT(pixbuf));
        } else {
            result->pixbuf = pixbuf;
        }
    }

    result->loader = loader;
    result->filename = filename;

    g_idle_add(capa_image_loader_result, result);

    g_mutex_lock(priv->lock);
    g_object_unref(G_OBJECT(transform));

 cleanup:
    g_mutex_unlock(priv->lock);
}


static void capa_image_loader_finalize(GObject *object)
{
    CapaImageLoader *loader = CAPA_IMAGE_LOADER(object);
    CapaImageLoaderPrivate *priv = loader->priv;

    CAPA_DEBUG("Finalize image loader %p", object);
    g_thread_pool_free(priv->workers, TRUE, TRUE);

    if (priv->colourTransform)
        g_object_unref(G_OBJECT(priv->colourTransform));

    g_hash_table_unref(priv->images);
    g_mutex_free(priv->lock);

    G_OBJECT_CLASS (capa_image_loader_parent_class)->finalize (object);
}


static void capa_image_loader_class_init(CapaImageLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_image_loader_finalize;
    object_class->get_property = capa_image_loader_get_property;
    object_class->set_property = capa_image_loader_set_property;

    g_object_class_install_property(object_class,
                                    PROP_NTHREADS,
                                    g_param_spec_int("nthreads",
                                                     "Number of threads",
                                                     "Number of threads to load images",
                                                     1, 64, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_COLOUR_TRANSFORM,
                                    g_param_spec_object("colour-transform",
                                                        "Colour transform",
                                                        "Colour profile transformation",
                                                        CAPA_TYPE_COLOUR_PROFILE_TRANSFORM,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_signal_new("image-loaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaImageLoaderClass, image_loaded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__STRING,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_STRING);

    g_type_class_add_private(klass, sizeof(CapaImageLoaderPrivate));
}


CapaImageLoader *capa_image_loader_new(void)
{
    return CAPA_IMAGE_LOADER(g_object_new(CAPA_TYPE_IMAGE_LOADER,
                                          NULL));
}


static void capa_image_loader_init(CapaImageLoader *loader)
{
    CapaImageLoaderPrivate *priv;

    priv = loader->priv = CAPA_IMAGE_LOADER_GET_PRIVATE(loader);
    memset(priv, 0, sizeof(*priv));

    priv->lock = g_mutex_new();
    priv->images = g_hash_table_new_full(g_str_hash,
                                         g_str_equal,
                                         NULL,
                                         capa_image_loader_entry_free);
    priv->workers = g_thread_pool_new(capa_image_loader_worker,
                                      loader,
                                      1,
                                      TRUE,
                                      NULL);
}


gboolean capa_image_loader_is_ready(CapaImageLoader *loader,
                                    const char *filename)
{
    CapaImageLoaderPrivate *priv = loader->priv;
    CapaImageLoaderEntry *entry;
    gboolean ready = FALSE;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->images, filename);
    if (!entry)
        goto cleanup;
    ready = entry->ready;

 cleanup:
    g_mutex_unlock(priv->lock);
    return ready;
}

GdkPixbuf *capa_image_loader_get_pixbuf(CapaImageLoader *loader,
                                        const char *filename)
{
    CapaImageLoaderPrivate *priv = loader->priv;
    CapaImageLoaderEntry *entry;
    GdkPixbuf *pixbuf = NULL;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->images, filename);
    if (!entry)
        goto cleanup;
    pixbuf = entry->pixbuf;

 cleanup:
    g_mutex_unlock(priv->lock);
    return pixbuf;
}

gboolean capa_image_loader_load(CapaImageLoader *loader,
                                const char *filename)
{
    CapaImageLoaderPrivate *priv = loader->priv;
    CapaImageLoaderEntry *entry;

    CAPA_DEBUG("load %p %s", loader, filename);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->images, filename);
    if (entry) {
        entry->refs++;
        g_mutex_unlock(priv->lock);
        g_signal_emit_by_name(G_OBJECT(loader), "image-loaded", entry->filename);
        return TRUE;
    }
    entry = capa_image_loader_entry_new(filename);
    g_hash_table_insert(priv->images, entry->filename, entry);
    g_thread_pool_push(priv->workers, entry->filename, NULL);

    g_mutex_unlock(priv->lock);
    return TRUE;
}

gboolean capa_image_loader_unload(CapaImageLoader *loader,
                                  const char *filename)
{
    CapaImageLoaderPrivate *priv = loader->priv;
    CapaImageLoaderEntry *entry;

    CAPA_DEBUG("unload %p %s", loader, filename);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->images, filename);
    if (!entry)
        goto cleanup;
    entry->refs--;
    CAPA_DEBUG("Entry %d %d", entry->refs, entry->ready);
    if (entry->refs == 0 &&
        !entry->processing &&
        !entry->pending)
        g_hash_table_remove(priv->images, entry->filename);

 cleanup:
    g_mutex_unlock(priv->lock);
    return TRUE;
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
