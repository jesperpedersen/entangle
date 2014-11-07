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

#include <stdio.h>
#include <string.h>

#include "entangle-debug.h"
#include "entangle-pixbuf-loader.h"

#define ENTANGLE_PIXBUF_LOADER_GET_PRIVATE(obj)                         \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PIXBUF_LOADER, EntanglePixbufLoaderPrivate))

#if GLIB_CHECK_VERSION(2, 31, 0)
#define g_mutex_new() g_new0(GMutex, 1)
#define g_mutex_free(m) g_free(m)
#endif

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


struct idle_emit_data {
    EntangleImage *image;
    EntanglePixbufLoader *loader;
    const char *name;
};

static gboolean do_idle_emit_func(gpointer opaque)
{
    struct idle_emit_data *data = opaque;

    g_signal_emit_by_name(data->loader, data->name, data->image);
    g_object_unref(data->image);
    g_object_unref(data->loader);
    g_free(data);

    return FALSE;
}

static void do_idle_emit(EntanglePixbufLoader *loader,
                         const char *name,
                         EntangleImage *image)
{
    struct idle_emit_data *data = g_new0(struct idle_emit_data, 1);
    data->loader = g_object_ref(loader);
    data->image = g_object_ref(image);
    data->name = name;
    g_idle_add(do_idle_emit_func, data);
}


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


/**
 * entangle_pixbuf_loader_trigger_reload:
 * @loader: the image loader
 *
 * Request a mass reload of the data associated with all
 * images
 */
void entangle_pixbuf_loader_trigger_reload(EntanglePixbufLoader *loader)
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
        do_idle_emit(loader, "pixbuf-loaded", result->image);
        if (result->metadata)
            do_idle_emit(loader, "metadata-loaded", result->image);
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


static GdkPixbuf *entangle_pixbuf_load(EntanglePixbufLoader *loader, EntangleImage *image,
                                       GExiv2Metadata **metadata)
{
    return ENTANGLE_PIXBUF_LOADER_GET_CLASS(loader)->pixbuf_load(loader, image, metadata);
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

    pixbuf = entangle_pixbuf_load(loader, image,
                                  priv->withMetadata ?
                                  &result->metadata : NULL);
    if (pixbuf) {
        if (transform) {
            result->pixbuf = entangle_colour_profile_transform_apply(transform,
                                                                     pixbuf);
            g_object_unref(pixbuf);
        } else {
            result->pixbuf = pixbuf;
        }
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

    G_OBJECT_CLASS(entangle_pixbuf_loader_parent_class)->finalize(object);
}


static void entangle_pixbuf_loader_class_init(EntanglePixbufLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

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
    g_signal_new("pixbuf-unloaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntanglePixbufLoaderClass, pixbuf_unloaded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_IMAGE);
    g_signal_new("metadata-unloaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntanglePixbufLoaderClass, metadata_unloaded),
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


/**
 * entangle_pixbuf_loader_is_ready:
 * @loader: the pixbuf loader
 * @image: the camera image
 *
 * Determine if the image has completed loading. Normally it is
 * better to wait for the 'pixbuf-loaded' or 'metadata-loaded'
 * signals than to use this method. The return value of this
 * method may be out of date if another thread concurrently
 * requests unload of the image
 *
 * Returns: a true value if the image is currently loaded
 */
gboolean entangle_pixbuf_loader_is_ready(EntanglePixbufLoader *loader,
                                         EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader), FALSE);
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), FALSE);

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


/**
 * entangle_pixbuf_loader_get_pixbuf:
 * @loader: (transfer none): the pixbuf loader
 * @image: (transfer none): the camera image
 *
 * Get the loaded pixbuf for @image, if any. If this is
 * called before the 'pixbuf-loaded' signal is emitted
 * then it will likely return NULL.
 *
 * Returns: (allow-none)(transfer none): the pixbuf
 */
GdkPixbuf *entangle_pixbuf_loader_get_pixbuf(EntanglePixbufLoader *loader,
                                             EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader), NULL);
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), NULL);

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


/**
 * entangle_pixbuf_loader_get_metadata:
 * @loader: (transfer none): the pixbuf loader
 * @image: (transfer none): the camera image
 *
 * Get the loaded metadata for @image, if any. If this is
 * called before the 'metadata-loaded' signal is emitted
 * then it will likely return NULL.
 *
 * Returns: (allow-none)(transfer none): the metadata
 */
GExiv2Metadata *entangle_pixbuf_loader_get_metadata(EntanglePixbufLoader *loader,
                                                    EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader), NULL);
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), NULL);

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


/**
 * entangle_pixbuf_loader_load:
 * @loader: (transfer none): the pixbuf loader
 * @image: (transfer none): the camera image
 *
 * Request that @loader have its pixbuf and metadata loaded.
 * The loading of the data may take place asynchronously
 * and the 'pixbuf-loaded' and 'metadata-loaded' signals
 * will be emitted when completed.
 *
 * Returns: a true value if the image was queued for loading
 */
gboolean entangle_pixbuf_loader_load(EntanglePixbufLoader *loader,
                                     EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader), FALSE);
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), FALSE);

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
        if (hasPixbuf && 0)
            do_idle_emit(loader, "pixbuf-loaded", image);
        if (hasMetadata && 0)
            do_idle_emit(loader, "metadata-loaded", image);
        return TRUE;
    }
    entry = entangle_pixbuf_loader_entry_new(image);
    g_hash_table_insert(priv->pixbufs, g_strdup(entangle_image_get_filename(image)), entry);
    g_thread_pool_push(priv->workers, image, NULL);

    g_mutex_unlock(priv->lock);
    return TRUE;
}


/**
 * entangle_pixbuf_loader_unload:
 * @loader: (transfer none): the pixbuf loader
 * @image: (transfer none): the camera image
 *
 * Indicate that @image is no longer required and can have its
 * pixbuf / metadata unloaded. The unloading of the data may
 * take place asynchronously and the 'pixbuf-unloaded' and
 * 'metadata-unloaded' signals will be emitted when completed.
 *
 * Returns: a true value if the image was unloaded
 */
gboolean entangle_pixbuf_loader_unload(EntanglePixbufLoader *loader,
                                       EntangleImage *image)
{
    g_return_val_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader), FALSE);
    g_return_val_if_fail(ENTANGLE_IS_IMAGE(image), FALSE);

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
        !entry->pending) {
        gboolean hasPixbuf = entry->pixbuf != NULL;
        gboolean hasMetadata = entry->metadata != NULL;
        if (hasPixbuf)
            do_idle_emit(loader, "pixbuf-unloaded", image);
        if (hasMetadata)
            do_idle_emit(loader, "metadata-unloaded", image);
        g_hash_table_remove(priv->pixbufs, entangle_image_get_filename(image));
    }

 cleanup:
    g_mutex_unlock(priv->lock);
    return TRUE;
}


/**
 * entangle_pixbuf_loader_set_colour_transform:
 * @loader: the pixbuf loader
 * @transform: the new colour profile transform
 *
 * Set the colour profile transform that will be applied when
 * loading images. This will trigger a mass-reload of all
 * existing images to update their colour profile
 */
void entangle_pixbuf_loader_set_colour_transform(EntanglePixbufLoader *loader,
                                                 EntangleColourProfileTransform *transform)
{
    g_return_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader));

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


/**
 * entangle_pixbuf_loader_get_colour_transform:
 * @loader: the pixbuf loader
 *
 * Get the colour transform that will be applied when loading
 * images
 *
 * Returns: (allow-none)(transfer none): the colour profile transform
 */
EntangleColourProfileTransform *entangle_pixbuf_loader_get_colour_transform(EntanglePixbufLoader *loader)
{
    g_return_val_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader), NULL);

    EntanglePixbufLoaderPrivate *priv = loader->priv;

    return priv->colourTransform;
}


/**
 * entangle_pixbuf_loader_set_workers:
 * @loader: the pixbuf loader
 * @count: the new limit on workers
 *
 * Set the maximum number of worker threads for the pixbuf loader
 */
void entangle_pixbuf_loader_set_workers(EntanglePixbufLoader *loader,
                                        int count)
{
    g_return_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader));

    EntanglePixbufLoaderPrivate *priv = loader->priv;

    g_thread_pool_set_max_threads(priv->workers, count, NULL);
}


/**
 * entangle_pixbuf_loader_get_workers:
 * @loader: the pixbuf loader
 *
 * Get the number of worker threads associated with the loader
 *
 * Returns: the maximum number of worker threads
 */
int entangle_pixbuf_loader_get_workers(EntanglePixbufLoader *loader)
{
    g_return_val_if_fail(ENTANGLE_IS_PIXBUF_LOADER(loader), 0);

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
