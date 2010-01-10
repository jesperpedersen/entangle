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

#include "capa-debug.h"
#include "capa-pixbuf-loader.h"
#include "capa-colour-profile.h"

#define CAPA_PIXBUF_LOADER_GET_PRIVATE(obj)                                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_PIXBUF_LOADER, CapaPixbufLoaderPrivate))

typedef struct _CapaPixbufLoaderEntry {
    int refs;
    char *filename;
    gboolean pending;
    gboolean processing;
    gboolean ready;
    GdkPixbuf *pixbuf;
} CapaPixbufLoaderEntry;

typedef struct _CapaPixbufrLoaderResult {
    CapaPixbufLoader *loader;
    const char *filename;
    GdkPixbuf *pixbuf;
} CapaPixbufLoaderResult;

struct _CapaPixbufLoaderPrivate {
    GThreadPool *workers;
    CapaColourProfileTransform *colourTransform;

    GMutex *lock;
    GHashTable *pixbufs;
};

G_DEFINE_ABSTRACT_TYPE(CapaPixbufLoader, capa_pixbuf_loader, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_NTHREADS,
    PROP_COLOUR_TRANSFORM,
};


static void capa_pixbuf_loader_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
    CapaPixbufLoader *loader = CAPA_PIXBUF_LOADER(object);
    CapaPixbufLoaderPrivate *priv = loader->priv;

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

static void capa_pixbuf_loader_trigger_reload(CapaPixbufLoader *loader)
{
    CapaPixbufLoaderPrivate *priv = loader->priv;
    GHashTableIter iter;
    gpointer key, value;

    CAPA_DEBUG("Triggering mass reload");

    g_mutex_lock(priv->lock);
    g_hash_table_iter_init(&iter, priv->pixbufs);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        CapaPixbufLoaderEntry *entry = value;
        if (entry->refs &&
            !entry->processing)
            g_thread_pool_push(priv->workers, entry->filename, NULL);
    }
    g_mutex_unlock(priv->lock);
}

static void capa_pixbuf_loader_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
    CapaPixbufLoader *loader = CAPA_PIXBUF_LOADER(object);
    CapaPixbufLoaderPrivate *priv = loader->priv;

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
            capa_pixbuf_loader_trigger_reload(loader);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_pixbuf_loader_entry_free(gpointer opaque)
{
    CapaPixbufLoaderEntry *entry = opaque;

    CAPA_DEBUG("free entry %p %s", entry, entry->filename);
    g_free(entry->filename);
    if (entry->pixbuf)
        g_object_unref(G_OBJECT(entry->pixbuf));
    g_free(entry);
}


static CapaPixbufLoaderEntry *capa_pixbuf_loader_entry_new(const char *filename)
{
    CapaPixbufLoaderEntry *entry;

    entry = g_new0(CapaPixbufLoaderEntry, 1);
    entry->filename = g_strdup(filename);
    entry->refs = 1;

    CAPA_DEBUG("new entry %p %s", entry, filename);

    return entry;
}


static gboolean capa_pixbuf_loader_result(gpointer data)
{
    CapaPixbufLoaderResult *result = data;
    CapaPixbufLoader *loader = result->loader;
    CapaPixbufLoaderPrivate *priv = loader->priv;
    CapaPixbufLoaderEntry *entry;
    char *filename;

    CAPA_DEBUG("result %p %s %p", loader, result->filename, result->pixbuf);

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, result->filename);
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
        g_signal_emit_by_name(G_OBJECT(loader), "pixbuf-loaded", filename);
    } else if (!entry->pending) {
        g_hash_table_remove(priv->pixbufs, entry->filename);
        g_mutex_unlock(priv->lock);
    }

    g_free(filename);
    g_free(result);

    return FALSE;
}


static GdkPixbuf *capa_pixbuf_load(CapaPixbufLoader *loader, const char *filename)
{
    return (CAPA_PIXBUF_LOADER_GET_CLASS(loader)->pixbuf_load)(loader, filename);
}

static void capa_pixbuf_loader_worker(gpointer data,
                                      gpointer opaque)
{
    CapaPixbufLoader *loader = opaque;
    CapaPixbufLoaderPrivate *priv = loader->priv;
    const char *filename = data;
    CapaPixbufLoaderResult *result = g_new0(CapaPixbufLoaderResult, 1);
    CapaColourProfileTransform *transform;
    CapaPixbufLoaderEntry *entry;
    GdkPixbuf *pixbuf;

    CAPA_DEBUG("worker %p %s", loader, filename);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, filename);
    if (!entry)
        goto cleanup;
    if (entry->refs == 0) {
        CAPA_DEBUG("pixbuf already removed");
        g_hash_table_remove(priv->pixbufs, filename);
        goto cleanup;
    }
    entry->pending = FALSE;
    entry->processing = TRUE;

    transform = priv->colourTransform;
    if (transform)
        g_object_ref(G_OBJECT(transform));
    g_mutex_unlock(priv->lock);

    pixbuf = capa_pixbuf_load(loader, filename);
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

    g_idle_add(capa_pixbuf_loader_result, result);

    g_mutex_lock(priv->lock);
    if (transform)
        g_object_unref(G_OBJECT(transform));

 cleanup:
    g_mutex_unlock(priv->lock);
}


static void capa_pixbuf_loader_finalize(GObject *object)
{
    CapaPixbufLoader *loader = CAPA_PIXBUF_LOADER(object);
    CapaPixbufLoaderPrivate *priv = loader->priv;

    CAPA_DEBUG("Finalize pixbuf loader %p", object);
    g_thread_pool_free(priv->workers, TRUE, TRUE);

    if (priv->colourTransform)
        g_object_unref(G_OBJECT(priv->colourTransform));

    g_hash_table_unref(priv->pixbufs);
    g_mutex_free(priv->lock);

    G_OBJECT_CLASS (capa_pixbuf_loader_parent_class)->finalize (object);
}


static void capa_pixbuf_loader_class_init(CapaPixbufLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_pixbuf_loader_finalize;
    object_class->get_property = capa_pixbuf_loader_get_property;
    object_class->set_property = capa_pixbuf_loader_set_property;

    g_object_class_install_property(object_class,
                                    PROP_NTHREADS,
                                    g_param_spec_int("nthreads",
                                                     "Number of threads",
                                                     "Number of threads to load pixbufs",
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

    g_signal_new("pixbuf-loaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaPixbufLoaderClass, pixbuf_loaded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__STRING,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_STRING);

    g_type_class_add_private(klass, sizeof(CapaPixbufLoaderPrivate));
}


CapaPixbufLoader *capa_pixbuf_loader_new(void)
{
    return CAPA_PIXBUF_LOADER(g_object_new(CAPA_TYPE_PIXBUF_LOADER,
                                          NULL));
}


static void capa_pixbuf_loader_init(CapaPixbufLoader *loader)
{
    CapaPixbufLoaderPrivate *priv;

    priv = loader->priv = CAPA_PIXBUF_LOADER_GET_PRIVATE(loader);
    memset(priv, 0, sizeof(*priv));

    priv->lock = g_mutex_new();
    priv->pixbufs = g_hash_table_new_full(g_str_hash,
                                         g_str_equal,
                                         NULL,
                                         capa_pixbuf_loader_entry_free);
    priv->workers = g_thread_pool_new(capa_pixbuf_loader_worker,
                                      loader,
                                      1,
                                      TRUE,
                                      NULL);
}


gboolean capa_pixbuf_loader_is_ready(CapaPixbufLoader *loader,
                                    const char *filename)
{
    CapaPixbufLoaderPrivate *priv = loader->priv;
    CapaPixbufLoaderEntry *entry;
    gboolean ready = FALSE;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, filename);
    if (!entry)
        goto cleanup;
    ready = entry->ready;

 cleanup:
    g_mutex_unlock(priv->lock);
    return ready;
}

GdkPixbuf *capa_pixbuf_loader_get_pixbuf(CapaPixbufLoader *loader,
                                        const char *filename)
{
    CapaPixbufLoaderPrivate *priv = loader->priv;
    CapaPixbufLoaderEntry *entry;
    GdkPixbuf *pixbuf = NULL;

    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, filename);
    if (!entry)
        goto cleanup;
    pixbuf = entry->pixbuf;

 cleanup:
    g_mutex_unlock(priv->lock);
    return pixbuf;
}

gboolean capa_pixbuf_loader_load(CapaPixbufLoader *loader,
                                const char *filename)
{
    CapaPixbufLoaderPrivate *priv = loader->priv;
    CapaPixbufLoaderEntry *entry;

    CAPA_DEBUG("Queue load %p %s", loader, filename);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, filename);
    if (entry) {
        entry->refs++;
        g_mutex_unlock(priv->lock);
        g_signal_emit_by_name(G_OBJECT(loader), "pixbuf-loaded", entry->filename);
        return TRUE;
    }
    entry = capa_pixbuf_loader_entry_new(filename);
    g_hash_table_insert(priv->pixbufs, entry->filename, entry);
    g_thread_pool_push(priv->workers, entry->filename, NULL);

    g_mutex_unlock(priv->lock);
    return TRUE;
}

gboolean capa_pixbuf_loader_unload(CapaPixbufLoader *loader,
                                  const char *filename)
{
    CapaPixbufLoaderPrivate *priv = loader->priv;
    CapaPixbufLoaderEntry *entry;

    CAPA_DEBUG("Unqueue load %p %s", loader, filename);
    g_mutex_lock(priv->lock);
    entry = g_hash_table_lookup(priv->pixbufs, filename);
    if (!entry)
        goto cleanup;
    entry->refs--;
    CAPA_DEBUG("Entry %d %d", entry->refs, entry->ready);
    if (entry->refs == 0 &&
        !entry->processing &&
        !entry->pending)
        g_hash_table_remove(priv->pixbufs, entry->filename);

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
