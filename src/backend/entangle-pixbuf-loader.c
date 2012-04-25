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

#include <stdio.h>
#include <string.h>

#include "entangle-debug.h"
#include "entangle-pixbuf-loader.h"

#define ENTANGLE_PIXBUF_LOADER_GET_PRIVATE(obj)                                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PIXBUF_LOADER, EntanglePixbufLoaderPrivate))

typedef struct _EntanglePixbufLoaderEntry {
    int refs;
    EntangleImage *image;
    gboolean pending;
    gboolean processing;
    gboolean ready;
    GdkPixbuf *pixbuf;
    GExiv2Metadata *metadata;
} EntanglePixbufLoaderEntry;

typedef struct _EntanglePixbufrLoaderResult {
    EntanglePixbufLoader *loader;
    EntangleImage *image;
    GdkPixbuf *pixbuf;
    GExiv2Metadata *metadata;
} EntanglePixbufLoaderResult;

struct _EntanglePixbufLoaderPrivate {
    GThreadPool *workers;
    EntangleColourProfileTransform *colourTransform;

    GMutex *lock;
    GHashTable *pixbufs;

    gboolean withMetadata;
};

G_DEFINE_ABSTRACT_TYPE(EntanglePixbufLoader, entangle_pixbuf_loader, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_WORKERS,
    PROP_COLOUR_TRANSFORM,
    PROP_WITH_METADATA,
};


static void entangle_pixbuf_loader_get_property(GObject *object,
                                                guint prop_id,
                                                GValue *value,
                                                GParamSpec *pspec)
{
    EntanglePixbufLoader *loader = ENTANGLE_PIXBUF_LOADER(object);
    EntanglePixbufLoaderPrivate *priv = loader->priv;

    switch (prop_id)
        {
        case PROP_WORKERS:
            g_value_set_int(value, entangle_pixbuf_loader_get_workers(loader));
            break;

        case PROP_COLOUR_TRANSFORM:
            g_value_set_object(value, priv->colourTransform);
            break;

        case PROP_WITH_METADATA:
            g_value_set_boolean(value, priv->withMetadata);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_pixbuf_loader_set_property(GObject *object,
                                                guint prop_id,
                                                const GValue *value,
                                                GParamSpec *pspec)
{
    EntanglePixbufLoader *loader = ENTANGLE_PIXBUF_LOADER(object);
    EntanglePixbufLoaderPrivate *priv = loader->priv;

    switch (prop_id)
        {
        case PROP_WORKERS:
            entangle_pixbuf_loader_set_workers(loader, g_value_get_int(value));
            break;

        case PROP_COLOUR_TRANSFORM:
            entangle_pixbuf_loader_set_colour_transform(loader, g_value_get_object(value));
            break;

        case PROP_WITH_METADATA:
            priv->withMetadata = g_value_get_boolean(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_pixbuf_loader_entry_free(gpointer opaque)
{
    EntanglePixbufLoaderEntry *entry = opaque;

    ENTANGLE_DEBUG("free entry %p %p", entry, entry->image);
    if (entry->image)
        g_object_unref(entry->image);
    if (entry->pixbuf)
        g_object_unref(entry->pixbuf);
    if (entry->metadata)
        g_object_unref(entry->metadata);
    g_free(entry);
}


static EntanglePixbufLoaderEntry *entangle_pixbuf_loader_entry_new(EntangleImage *image)
{
    EntanglePixbufLoaderEntry *entry;

    entry = g_new0(EntanglePixbufLoaderEntry, 1);
    entry->image = image;
    g_object_ref(image);
    entry->refs = 1;
    entry->pending = TRUE;

    ENTANGLE_DEBUG("new entry %p %p", entry, image);

    return entry;
}


static void entangle_pixbuf_loader_trigger_reload(EntanglePixbufLoader *loader)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    GHashTableIter iter;
    gpointer key, value;

    ENTANGLE_DEBUG("Triggering mass reload");

    g_mutex_lock(priv->lock);
    g_hash_table_iter_init(&iter, priv->pixbufs);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        EntanglePixbufLoaderEntry *entry = value;
        if (entry->refs &&
            !entry->processing)
            g_thread_pool_push(priv->workers, entry->image, NULL);
    }
    g_mutex_unlock(priv->lock);
}


static gboolean entangle_pixbuf_loader_result(gpointer data)
{
    EntanglePixbufLoaderResult *result = data;
    EntanglePixbufLoader *loader = result->loader;
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    EntanglePixbufLoaderEntry *entry;

    ENTANGLE_DEBUG("result %p %p %p", loader, result->image, result->pixbuf);

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, entangle_image_get_filename(result->image));
    if (!entry) {
        g_mutex_unlock(priv->lock);
        if (result->pixbuf)
            g_object_unref(result->pixbuf);
        g_object_unref(result->image);
        if (result->metadata)
            g_object_unref(result->metadata);
        g_object_unref(result->loader);
        g_free(result);
        return FALSE;
    }

    if (entry->pixbuf)
        g_object_unref(entry->pixbuf);
    entry->pixbuf = result->pixbuf;
    entry->metadata = result->metadata;
    entry->ready = TRUE;
    entry->processing = FALSE;

    if (entry->refs) {
        g_mutex_unlock(priv->lock);
        ENTANGLE_DEBUG("Emit loaded %p %p %p", result->image, result->pixbuf, result->metadata);
        g_signal_emit_by_name(loader, "pixbuf-loaded", result->image);
        if (result->metadata)
            g_signal_emit_by_name(loader, "metadata-loaded", result->image);
        g_mutex_lock(priv->lock);
    } else if (!entry->pending) {
        g_hash_table_remove(priv->pixbufs, entangle_image_get_filename(result->image));
    }

    g_object_unref(result->loader);
    g_object_unref(result->image);
    g_free(result);
    g_mutex_unlock(priv->lock);

    return FALSE;
}


static GdkPixbuf *entangle_pixbuf_load(EntanglePixbufLoader *loader, EntangleImage *image)
{
    return ENTANGLE_PIXBUF_LOADER_GET_CLASS(loader)->pixbuf_load(loader, image);
}

static GExiv2Metadata *entangle_metadata_load(EntanglePixbufLoader *loader, EntangleImage *image)
{
    return ENTANGLE_PIXBUF_LOADER_GET_CLASS(loader)->metadata_load(loader, image);
}

static void entangle_pixbuf_loader_worker(gpointer data,
                                          gpointer opaque)
{
    EntanglePixbufLoader *loader = opaque;
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    EntangleImage *image = data;
    EntanglePixbufLoaderResult *result = g_new0(EntanglePixbufLoaderResult, 1);
    EntangleColourProfileTransform *transform;
    EntanglePixbufLoaderEntry *entry;
    GdkPixbuf *pixbuf;

    ENTANGLE_DEBUG("worker process job %p %p", loader, image);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, entangle_image_get_filename(image));
    if (!entry)
        goto cleanup;
    if (entry->refs == 0) {
        ENTANGLE_DEBUG("pixbuf already removed");
        g_hash_table_remove(priv->pixbufs, entangle_image_get_filename(image));
        goto cleanup;
    }
    entry->pending = FALSE;
    entry->processing = TRUE;

    transform = priv->colourTransform;
    if (transform)
        g_object_ref(transform);
    g_mutex_unlock(priv->lock);

    pixbuf = entangle_pixbuf_load(loader, image);
    if (pixbuf) {
        if (transform) {
            result->pixbuf = entangle_colour_profile_transform_apply(transform,
                                                                     pixbuf);
            g_object_unref(pixbuf);
        } else {
            result->pixbuf = pixbuf;
        }
    }
    if (priv->withMetadata) {
        result->metadata = entangle_metadata_load(loader, image);
    }

    result->loader = loader;
    result->image = image;
    g_object_ref(image);
    g_object_ref(loader);

    g_idle_add(entangle_pixbuf_loader_result, result);

    g_mutex_lock(priv->lock);
    if (transform)
        g_object_unref(transform);

 cleanup:
    g_mutex_unlock(priv->lock);
}


static void entangle_pixbuf_loader_finalize(GObject *object)
{
    EntanglePixbufLoader *loader = ENTANGLE_PIXBUF_LOADER(object);
    EntanglePixbufLoaderPrivate *priv = loader->priv;

    ENTANGLE_DEBUG("Finalize pixbuf loader %p", object);
    g_thread_pool_free(priv->workers, TRUE, TRUE);

    if (priv->colourTransform)
        g_object_unref(priv->colourTransform);

    g_hash_table_unref(priv->pixbufs);
    g_mutex_free(priv->lock);

    G_OBJECT_CLASS (entangle_pixbuf_loader_parent_class)->finalize (object);
}


static void entangle_pixbuf_loader_class_init(EntanglePixbufLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_pixbuf_loader_finalize;
    object_class->get_property = entangle_pixbuf_loader_get_property;
    object_class->set_property = entangle_pixbuf_loader_set_property;

    g_object_class_install_property(object_class,
                                    PROP_WORKERS,
                                    g_param_spec_int("workers",
                                                     "Workers",
                                                     "Number of worker threads to load pixbufs",
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
                                                        ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_WITH_METADATA,
                                    g_param_spec_boolean("with-metadata",
                                                         "With metadata",
                                                         "Load image metadata",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_signal_new("pixbuf-loaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntanglePixbufLoaderClass, pixbuf_loaded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_IMAGE);
    g_signal_new("metadata-loaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntanglePixbufLoaderClass, metadata_loaded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_IMAGE);

    g_type_class_add_private(klass, sizeof(EntanglePixbufLoaderPrivate));
}


static void entangle_pixbuf_loader_init(EntanglePixbufLoader *loader)
{
    EntanglePixbufLoaderPrivate *priv;

    priv = loader->priv = ENTANGLE_PIXBUF_LOADER_GET_PRIVATE(loader);
    memset(priv, 0, sizeof(*priv));

    priv->lock = g_mutex_new();
    priv->pixbufs = g_hash_table_new_full(g_str_hash,
                                          g_str_equal,
                                          g_free,
                                          entangle_pixbuf_loader_entry_free);
    priv->workers = g_thread_pool_new(entangle_pixbuf_loader_worker,
                                      loader,
                                      1,
                                      TRUE,
                                      NULL);
}


gboolean entangle_pixbuf_loader_is_ready(EntanglePixbufLoader *loader,
                                         EntangleImage *image)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    EntanglePixbufLoaderEntry *entry;
    gboolean ready = FALSE;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, entangle_image_get_filename(image));
    if (!entry)
        goto cleanup;
    ready = entry->ready;

 cleanup:
    g_mutex_unlock(priv->lock);
    return ready;
}

GdkPixbuf *entangle_pixbuf_loader_get_pixbuf(EntanglePixbufLoader *loader,
                                             EntangleImage *image)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    EntanglePixbufLoaderEntry *entry;
    GdkPixbuf *pixbuf = NULL;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, entangle_image_get_filename(image));
    if (!entry)
        goto cleanup;
    pixbuf = entry->pixbuf;

 cleanup:
    g_mutex_unlock(priv->lock);
    return pixbuf;
}


GExiv2Metadata *entangle_pixbuf_loader_get_metadata(EntanglePixbufLoader *loader,
                                                    EntangleImage *image)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    EntanglePixbufLoaderEntry *entry;
    GExiv2Metadata *metadata = NULL;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, entangle_image_get_filename(image));
    if (!entry)
        goto cleanup;
    metadata = entry->metadata;

 cleanup:
    g_mutex_unlock(priv->lock);
    return metadata;
}


gboolean entangle_pixbuf_loader_load(EntanglePixbufLoader *loader,
                                     EntangleImage *image)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    EntanglePixbufLoaderEntry *entry;

    ENTANGLE_DEBUG("Queue load %p %p", loader, image);

    if (!entangle_image_get_filename(image))
        return FALSE;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, entangle_image_get_filename(image));
    if (entry) {
        gboolean hasPixbuf = entry->pixbuf != NULL;
        gboolean hasMetadata = entry->metadata != NULL;
        entry->refs++;
        g_mutex_unlock(priv->lock);
        if (hasPixbuf)
            g_signal_emit_by_name(loader, "pixbuf-loaded", image);
        if (hasMetadata)
            g_signal_emit_by_name(loader, "metadata-loaded", image);
        return TRUE;
    }
    entry = entangle_pixbuf_loader_entry_new(image);
    g_hash_table_insert(priv->pixbufs, g_strdup(entangle_image_get_filename(image)), entry);
    g_thread_pool_push(priv->workers, image, NULL);

    g_mutex_unlock(priv->lock);
    return TRUE;
}

gboolean entangle_pixbuf_loader_unload(EntanglePixbufLoader *loader,
                                       EntangleImage *image)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;
    EntanglePixbufLoaderEntry *entry;

    if (!entangle_image_get_filename(image))
        return FALSE;

    ENTANGLE_DEBUG("Unqueue load %p %p", loader, image);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, entangle_image_get_filename(image));
    if (!entry)
        goto cleanup;
    entry->refs--;
    ENTANGLE_DEBUG("Entry %d %d", entry->refs, entry->ready);
    if (entry->refs == 0 &&
        !entry->processing &&
        !entry->pending)
        g_hash_table_remove(priv->pixbufs, entangle_image_get_filename(image));

 cleanup:
    g_mutex_unlock(priv->lock);
    return TRUE;
}


void entangle_pixbuf_loader_set_colour_transform(EntanglePixbufLoader *loader,
                                                 EntangleColourProfileTransform *transform)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;

    g_mutex_lock(priv->lock);
    if (priv->colourTransform)
        g_object_unref(priv->colourTransform);
    priv->colourTransform = transform;
    if (priv->colourTransform)
        g_object_ref(priv->colourTransform);
    g_mutex_unlock(priv->lock);

    entangle_pixbuf_loader_trigger_reload(loader);
}


EntangleColourProfileTransform *entangle_pixbuf_loader_get_colour_transform(EntanglePixbufLoader *loader)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;

    return priv->colourTransform;
}


void entangle_pixbuf_loader_set_workers(EntanglePixbufLoader *loader,
                                    int count)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;

    g_thread_pool_set_max_threads(priv->workers, count, NULL);
}


int entangle_pixbuf_loader_get_workers(EntanglePixbufLoader *loader)
{
    EntanglePixbufLoaderPrivate *priv = loader->priv;

    return g_thread_pool_get_max_threads(priv->workers);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
