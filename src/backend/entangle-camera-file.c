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

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <unistd.h>

#include "entangle-debug.h"
#include "entangle-camera-file.h"

#define ENTANGLE_CAMERA_FILE_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_FILE, EntangleCameraFilePrivate))

struct _EntangleCameraFilePrivate {
    char *folder;
    char *name;
    char *mimetype;

    GByteArray *data;
};

G_DEFINE_TYPE(EntangleCameraFile, entangle_camera_file, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_FOLDER,
    PROP_NAME,
    PROP_MIMETYPE,
    PROP_DATA,
};


static void entangle_camera_file_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    EntangleCameraFile *file = ENTANGLE_CAMERA_FILE(object);
    EntangleCameraFilePrivate *priv = file->priv;

    switch (prop_id)
        {
        case PROP_FOLDER:
            g_value_set_string(value, priv->folder);
            break;

        case PROP_NAME:
            g_value_set_string(value, priv->name);
            break;

        case PROP_MIMETYPE:
           g_value_set_string(value, priv->mimetype);
            break;

        case PROP_DATA:
            g_value_set_boxed(value, priv->data);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_camera_file_set_property(GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
    EntangleCameraFile *file = ENTANGLE_CAMERA_FILE(object);
    EntangleCameraFilePrivate *priv = file->priv;

    switch (prop_id)
        {
        case PROP_FOLDER:
            g_free(priv->folder);
            priv->folder = g_value_dup_string(value);
            break;

        case PROP_NAME:
            g_free(priv->name);
            priv->name = g_value_dup_string(value);
            break;

        case PROP_MIMETYPE:
            g_free(priv->mimetype);
            priv->mimetype = g_value_dup_string(value);
            break;

        case PROP_DATA:
            if (priv->data)
                g_byte_array_unref(priv->data);
            priv->data = g_value_dup_boxed(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_file_finalize(GObject *object)
{
    EntangleCameraFile *file = ENTANGLE_CAMERA_FILE(object);
    EntangleCameraFilePrivate *priv = file->priv;

    ENTANGLE_DEBUG("Finalize camera %p", object);

    g_free(priv->folder);
    g_free(priv->name);
    g_free(priv->mimetype);

    if (priv->data)
        g_byte_array_unref(priv->data);

    G_OBJECT_CLASS(entangle_camera_file_parent_class)->finalize(object);
}


static void entangle_camera_file_class_init(EntangleCameraFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_camera_file_finalize;
    object_class->get_property = entangle_camera_file_get_property;
    object_class->set_property = entangle_camera_file_set_property;


    g_object_class_install_property(object_class,
                                    PROP_FOLDER,
                                    g_param_spec_string("folder",
                                                        "Camera file folder",
                                                        "Folder name on the camera",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Camera file name",
                                                        "File name on the camera",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_MIMETYPE,
                                    g_param_spec_string("mimetype",
                                                        "Camera file mimetype",
                                                        "File mimetype on the camera",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_DATA,
                                    g_param_spec_boxed("data",
                                                       "Profile data",
                                                       "Raw data for the file",
                                                       G_TYPE_BYTE_ARRAY,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    ENTANGLE_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(EntangleCameraFilePrivate));
}


EntangleCameraFile *entangle_camera_file_new(const char *folder,
                                     const char *name)
{
    return ENTANGLE_CAMERA_FILE(g_object_new(ENTANGLE_TYPE_CAMERA_FILE,
                                         "folder", folder,
                                         "name", name,
                                         NULL));
}


static void entangle_camera_file_init(EntangleCameraFile *file)
{
    file->priv = ENTANGLE_CAMERA_FILE_GET_PRIVATE(file);
}


const char *entangle_camera_file_get_folder(EntangleCameraFile *file)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_FILE(file), NULL);

    EntangleCameraFilePrivate *priv = file->priv;
    return priv->folder;
}


const char *entangle_camera_file_get_name(EntangleCameraFile *file)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_FILE(file), NULL);

    EntangleCameraFilePrivate *priv = file->priv;
    return priv->name;
}


gboolean entangle_camera_file_save_path(EntangleCameraFile *file,
                                        const char *localpath,
                                        GError **err)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_FILE(file), FALSE);

    EntangleCameraFilePrivate *priv = file->priv;
    GFile *gf;
    GFileOutputStream *gos;
    gsize written;
    int ret = FALSE;

    ENTANGLE_DEBUG("Saving path [%s]", localpath);
    if (!priv->data) {
        ENTANGLE_DEBUG("Failed no data");
        return FALSE;
    }

    gf = g_file_new_for_path(localpath);

    gos = g_file_create(gf, G_FILE_CREATE_NONE, NULL, err);
    if (!gos) {
        ENTANGLE_DEBUG("Failed to create file");
        goto cleanup;
    }

    if (!g_output_stream_write_all(G_OUTPUT_STREAM(gos),
                                   priv->data->data,
                                   priv->data->len,
                                   &written,
                                   NULL,
                                   err)) {
        ENTANGLE_DEBUG("Failed write data %p %d", priv->data->data, priv->data->len);
        goto cleanup;
    }

    if (!g_output_stream_close(G_OUTPUT_STREAM(gos),
                               NULL,
                               err)) {
        ENTANGLE_DEBUG("Failed close stream");
        goto cleanup;
    }

    ret = TRUE;
    ENTANGLE_DEBUG("Wrote %d of %p %d\n", (int)written, priv->data, priv->data->len);

 cleanup:
    if (gos) {
        if (!ret)
            g_file_delete(gf, NULL, NULL);
        g_object_unref(gos);
    }
    g_object_unref(gf);
    return ret;
}

gboolean entangle_camera_file_save_uri(EntangleCameraFile *file,
                                       const char *uri,
                                       GError **err)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_FILE(file), FALSE);

    EntangleCameraFilePrivate *priv = file->priv;
    GFile *gf;
    GFileOutputStream *gos;
    gsize written;
    int ret = FALSE;

    ENTANGLE_DEBUG("Saving uri [%s]", uri);
    if (!priv->data) {
        ENTANGLE_DEBUG("Failed no data");
        return FALSE;
    }

    gf = g_file_new_for_uri(uri);

    gos = g_file_create(gf, G_FILE_CREATE_NONE, NULL, err);
    if (!gos) {
        ENTANGLE_DEBUG("Failed to create file");
        goto cleanup;
    }

    if (!g_output_stream_write_all(G_OUTPUT_STREAM(gos),
                                   priv->data->data,
                                   priv->data->len,
                                   &written,
                                   NULL,
                                   err)) {
        ENTANGLE_DEBUG("Failed write data %p %d", priv->data->data, priv->data->len);
        goto cleanup;
    }

    if (!g_output_stream_close(G_OUTPUT_STREAM(gos),
                               NULL,
                               err)) {
        ENTANGLE_DEBUG("Failed close stream");
        goto cleanup;
    }

    ret = TRUE;
    ENTANGLE_DEBUG("Wrote %d of %p %d\n", (int)written, priv->data, priv->data->len);
 cleanup:
    if (gos) {
        if (!ret)
            g_file_delete(gf, NULL, NULL);
        g_object_unref(gos);
    }
    g_object_unref(gf);
    return ret;
}


GByteArray *entangle_camera_file_get_data(EntangleCameraFile *file)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_FILE(file), NULL);

    EntangleCameraFilePrivate *priv = file->priv;
    return priv->data;
}


void entangle_camera_file_set_data(EntangleCameraFile *file, GByteArray *data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_FILE(file));

    EntangleCameraFilePrivate *priv = file->priv;
    if (priv->data)
        g_byte_array_unref(priv->data);
    priv->data = data;
    if (priv->data)
        g_byte_array_ref(priv->data);
}


const gchar *entangle_camera_file_get_mimetype(EntangleCameraFile *file)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_FILE(file), NULL);

    EntangleCameraFilePrivate *priv = file->priv;

    return priv->mimetype;
}


void entangle_camera_file_set_mimetype(EntangleCameraFile *file, const gchar *mimetype)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_FILE(file));

    EntangleCameraFilePrivate *priv = file->priv;
    g_free(priv->mimetype);
    priv->mimetype = NULL;
    if (mimetype)
        priv->mimetype = g_strdup(mimetype);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
