/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2014 Daniel P. Berrange
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
#include <string.h>

#include "entangle-debug.h"
#include "entangle-camera-automata.h"

#define ENTANGLE_CAMERA_AUTOMATA_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_AUTOMATA, EntangleCameraAutomataPrivate))

struct _EntangleCameraAutomataPrivate {
    EntangleSession *session;
    EntangleCamera *camera;

    gboolean deleteFile;

    char *deleteImageDup;

    gulong sigFileAdd;
    gulong sigFileDownload;
};

G_DEFINE_TYPE(EntangleCameraAutomata, entangle_camera_automata, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_SESSION,
    PROP_CAMERA,
    PROP_DELETE_FILE,
};


static void entangle_camera_automata_get_property(GObject *object,
                                                  guint prop_id,
                                                  GValue *value,
                                                  GParamSpec *pspec)
{
    EntangleCameraAutomata *automata = ENTANGLE_CAMERA_AUTOMATA(object);
    EntangleCameraAutomataPrivate *priv = automata->priv;

    switch (prop_id) {
    case PROP_SESSION:
        g_value_set_object(value, priv->session);
        break;

    case PROP_CAMERA:
        g_value_set_object(value, priv->camera);
        break;

    case PROP_DELETE_FILE:
        g_value_set_boolean(value, priv->deleteFile);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void entangle_camera_automata_set_property(GObject *object,
                                                  guint prop_id,
                                                  const GValue *value,
                                                  GParamSpec *pspec)
{
    EntangleCameraAutomata *automata = ENTANGLE_CAMERA_AUTOMATA(object);
    EntangleCameraAutomataPrivate *priv = automata->priv;

    switch (prop_id) {
    case PROP_SESSION:
        entangle_camera_automata_set_session(automata,
                                             g_value_get_object(value));
        break;

    case PROP_CAMERA:
        entangle_camera_automata_set_camera(automata,
                                            g_value_get_object(value));
        break;

    case PROP_DELETE_FILE:
        priv->deleteFile = g_value_get_boolean(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void entangle_camera_automata_finalize(GObject *object)
{
    EntangleCameraAutomata *automata = ENTANGLE_CAMERA_AUTOMATA(object);
    EntangleCameraAutomataPrivate *priv = automata->priv;

    ENTANGLE_DEBUG("Finalize camera automata %p", object);

    if (priv->camera)
        g_object_unref(priv->camera);
    if (priv->session)
        g_object_unref(priv->session);

    g_free(priv->deleteImageDup);

    G_OBJECT_CLASS(entangle_camera_automata_parent_class)->finalize(object);
}


static void entangle_camera_automata_class_init(EntangleCameraAutomataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_camera_automata_finalize;
    object_class->get_property = entangle_camera_automata_get_property;
    object_class->set_property = entangle_camera_automata_set_property;

    g_object_class_install_property(object_class,
                                    PROP_SESSION,
                                    g_param_spec_object("session",
                                                        "Session",
                                                        "Session",
                                                        ENTANGLE_TYPE_SESSION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera",
                                                        ENTANGLE_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DELETE_FILE,
                                    g_param_spec_boolean("delete-file",
                                                         "Delete file",
                                                         "Delete file",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_signal_new("camera-capture-begin",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraAutomataClass, camera_capture_begin),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);
    g_signal_new("camera-capture-end",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraAutomataClass, camera_capture_end),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_type_class_add_private(klass, sizeof(EntangleCameraAutomataPrivate));
}


EntangleCameraAutomata *entangle_camera_automata_new(void)
{
    return ENTANGLE_CAMERA_AUTOMATA(g_object_new(ENTANGLE_TYPE_CAMERA_AUTOMATA,
                                                 NULL));
}


static void entangle_camera_automata_init(EntangleCameraAutomata *automata)
{
    automata->priv = ENTANGLE_CAMERA_AUTOMATA_GET_PRIVATE(automata);
}


typedef struct {
    EntangleCameraAutomata *automata;
    GSimpleAsyncResult *result;
    GCancellable *cancel;
    GCancellable *confirm;
    EntangleCameraFile *file;
} EntangleCameraAutomataData;


static EntangleCameraAutomataData *
entangle_camera_automata_data_new(EntangleCameraAutomata *automata,
                                  GCancellable *cancel,
                                  GCancellable *confirm,
                                  GSimpleAsyncResult *result)
{
    EntangleCameraAutomataData *data = g_new0(EntangleCameraAutomataData, 1);
    data->automata = g_object_ref(automata);
    data->result = g_object_ref(result);
    if (cancel)
        data->cancel = g_object_ref(cancel);
    if (confirm)
        data->confirm = g_object_ref(confirm);
    return data;
}


static void entangle_camera_automata_data_free(EntangleCameraAutomataData *data)
{
    g_object_unref(data->automata);
    if (data->file)
        g_object_unref(data->file);
    g_object_unref(data->result);
    if (data->cancel)
        g_object_unref(data->cancel);
    g_free(data);
}


static void do_entangle_camera_file_download(EntangleCamera *cam G_GNUC_UNUSED,
                                             EntangleCameraFile *file,
                                             void *opaque)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(opaque));

    EntangleCameraAutomata *automata = opaque;
    EntangleCameraAutomataPrivate *priv = automata->priv;
    EntangleImage *image;
    gchar *localpath;

    ENTANGLE_DEBUG("File download %p %p %p", cam, file, automata);

    localpath = entangle_session_next_filename(priv->session, file);

    if (!entangle_camera_file_save_path(file, localpath, NULL)) {
        ENTANGLE_DEBUG("Failed save path");
        goto cleanup;
    }
    ENTANGLE_DEBUG("Saved to %s", localpath);
    image = entangle_image_new_file(localpath);

    entangle_session_add(priv->session, image);

    g_object_unref(image);

 cleanup:
    g_free(localpath);
}


static void do_entangle_camera_delete_finish(GObject *src,
                                             GAsyncResult *res,
                                             gpointer opaque)
{
    EntangleCameraAutomataData *data = opaque;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    if (!entangle_camera_delete_file_finish(camera, res, &error)) {
        g_simple_async_result_set_from_error(data->result,
                                             error);
        g_error_free(error);
        /* Fallthrough to unref */
    }

    g_simple_async_result_complete(data->result);
    entangle_camera_automata_data_free(data);
}


static void do_entangle_camera_download_finish(GObject *src,
                                               GAsyncResult *res,
                                               gpointer opaque)
{
    EntangleCameraAutomataData *data = opaque;
    EntangleCameraAutomataPrivate *priv = data->automata->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    if (!entangle_camera_download_file_finish(camera, res, &error)) {
        g_simple_async_result_set_from_error(data->result,
                                             error);
        g_error_free(error);
        /* Fallthrough to delete anyway */
    }

    if (priv->deleteFile) {
        entangle_camera_delete_file_async(camera,
                                          data->file,
                                          NULL,
                                          do_entangle_camera_delete_finish,
                                          data);
    } else {
        g_simple_async_result_complete(data->result);
        entangle_camera_automata_data_free(data);
    }
}


static void do_entangle_camera_file_add_finish(GObject *src G_GNUC_UNUSED,
                                               GAsyncResult *res G_GNUC_UNUSED,
                                               gpointer opaque G_GNUC_UNUSED)
{
    /* XXX report error ? */
}


static void do_entangle_camera_file_add(EntangleCamera *camera,
                                        EntangleCameraFile *file,
                                        void *opaque)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA(camera));
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(opaque));
    g_return_if_fail(ENTANGLE_IS_CAMERA_FILE(file));

    EntangleCameraAutomata *automata = ENTANGLE_CAMERA_AUTOMATA(opaque);
    EntangleCameraAutomataPrivate *priv = automata->priv;
    EntangleCameraAutomataData *data;
    GSimpleAsyncResult *result;

    result = g_simple_async_result_new(G_OBJECT(automata),
                                       do_entangle_camera_file_add_finish,
                                       automata,
                                       do_entangle_camera_file_add);
    data = entangle_camera_automata_data_new(automata,
                                             NULL,
                                             NULL,
                                             result);
    g_object_unref(result);
    ENTANGLE_DEBUG("File add %p %p %p", camera, file, data);

    data->file = g_object_ref(file);

    if (priv->deleteImageDup) {
        gsize len = strlen(priv->deleteImageDup);
        if (strncmp(entangle_camera_file_get_name(file),
                    priv->deleteImageDup,
                    len) == 0) {
            g_free(priv->deleteImageDup);
            priv->deleteImageDup = NULL;
            entangle_camera_delete_file_async(camera,
                                              data->file,
                                              NULL,
                                              do_entangle_camera_delete_finish,
                                              data);
            return;
        }
    }

    entangle_camera_download_file_async(camera,
                                        data->file,
                                        NULL,
                                        do_entangle_camera_download_finish,
                                        data);
}


static void do_entangle_camera_capture_finish(GObject *src,
                                              GAsyncResult *res,
                                              gpointer opaque)
{
    EntangleCameraAutomataData *data = opaque;
    EntangleCameraAutomataPrivate *priv = data->automata->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    g_signal_emit_by_name(data->automata, "camera-capture-end");

    if (!(data->file = entangle_camera_capture_image_finish(camera, res, &error))) {
        g_simple_async_result_set_from_error(data->result,
                                             error);
        g_error_free(error);
        g_simple_async_result_complete(data->result);
        entangle_camera_automata_data_free(data);
        return;
    }

    if (g_cancellable_is_cancelled(data->cancel)) {
        if (priv->deleteFile) {
            entangle_camera_delete_file_async(camera,
                                              data->file,
                                              NULL,
                                              do_entangle_camera_delete_finish,
                                              data);
        } else {
            g_simple_async_result_complete(data->result);
            entangle_camera_automata_data_free(data);
        }
    } else {
        entangle_camera_download_file_async(camera,
                                            data->file,
                                            NULL,
                                            do_entangle_camera_download_finish,
                                            data);
    }
}


void entangle_camera_automata_capture_async(EntangleCameraAutomata *automata,
                                            GCancellable *cancel,
                                            GAsyncReadyCallback callback,
                                            gpointer user_data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata));

    EntangleCameraAutomataPrivate *priv = automata->priv;
    EntangleCameraAutomataData *data;
    GSimpleAsyncResult *result;

    result = g_simple_async_result_new(G_OBJECT(automata),
                                       callback,
                                       user_data,
                                       entangle_camera_automata_capture_async);
    data = entangle_camera_automata_data_new(automata,
                                             cancel,
                                             NULL,
                                             result);

    g_signal_emit_by_name(automata, "camera-capture-begin");
    entangle_camera_capture_image_async(priv->camera,
                                        cancel,
                                        do_entangle_camera_capture_finish,
                                        data);

    g_object_unref(result);
}


gboolean entangle_camera_automata_capture_finish(EntangleCameraAutomata *automata,
                                                 GAsyncResult *res,
                                                 GError **error)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata), FALSE);

    if (g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(res),
                                              error))
        return FALSE;

    return TRUE;
}


static void do_entangle_camera_discard_finish(GObject *src,
                                              GAsyncResult *res,
                                              gpointer opaque)
{
    EntangleCameraAutomataData *data = opaque;
    EntangleCameraAutomataPrivate *priv = data->automata->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;
    char *filename;
    char *tmp;

    if (!(data->file = entangle_camera_capture_image_finish(camera, res, &error))) {
        g_simple_async_result_set_from_error(data->result,
                                             error);
        g_error_free(error);
        g_simple_async_result_complete(data->result);
        entangle_camera_automata_data_free(data);
        return;
    }

    filename = g_strdup(entangle_camera_file_get_name(data->file));
    if ((tmp = strrchr(filename, '.')))
        *tmp = '\0';
    priv->deleteImageDup = filename;

    entangle_camera_delete_file_async(camera,
                                      data->file,
                                      NULL,
                                      do_entangle_camera_delete_finish,
                                      data);
}


static void do_entangle_camera_viewfinder_finish(GObject *src,
                                                 GAsyncResult *res,
                                                 gpointer opaque)
{
    EntangleCameraAutomataData *data = opaque;
    EntangleCameraAutomataPrivate *priv = data->automata->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    if (!entangle_camera_set_viewfinder_finish(camera, res, &error)) {
        g_simple_async_result_set_from_error(data->result,
                                             error);
        g_error_free(error);
    }

    if (g_cancellable_is_cancelled(data->cancel)) {
        g_simple_async_result_complete(data->result);
        entangle_camera_automata_data_free(data);
    } else {
        g_cancellable_reset(data->confirm);
        entangle_camera_capture_image_async(priv->camera,
                                            data->cancel,
                                            do_entangle_camera_capture_finish,
                                            data);
    }
}


static void do_entangle_camera_preview_finish(GObject *src,
                                              GAsyncResult *res,
                                              gpointer opaque)
{
    EntangleCameraAutomataData *data = opaque;
    EntangleCameraAutomataPrivate *priv = data->automata->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    EntangleCameraFile *file;
    GError *error = NULL;

    if (!(file = entangle_camera_preview_image_finish(camera, res, &error))) {
        if (g_cancellable_is_cancelled(data->cancel) && priv->camera) {
            if (entangle_camera_get_has_viewfinder(priv->camera))
                entangle_camera_set_viewfinder_async(priv->camera,
                                                     FALSE,
                                                     NULL,
                                                     do_entangle_camera_viewfinder_finish,
                                                     data);
            else
                entangle_camera_capture_image_async(priv->camera,
                                                    NULL,
                                                    do_entangle_camera_discard_finish,
                                                    data);
        } else {
            g_simple_async_result_set_from_error(data->result,
                                                 error);
            g_simple_async_result_complete(data->result);
            entangle_camera_automata_data_free(data);
        }
        g_error_free(error);
        return;
    }

    g_object_unref(file);

    if (g_cancellable_is_cancelled(data->cancel)) {
        if (entangle_camera_get_has_viewfinder(priv->camera))
            entangle_camera_set_viewfinder_async(priv->camera,
                                                 FALSE,
                                                 NULL,
                                                 do_entangle_camera_viewfinder_finish,
                                                 data);
        else
            entangle_camera_capture_image_async(priv->camera,
                                                NULL,
                                                do_entangle_camera_discard_finish,
                                                data);
    } else if (g_cancellable_is_cancelled(data->confirm)) {
        g_signal_emit_by_name(data->automata, "camera-capture-begin");
        if (entangle_camera_get_has_viewfinder(priv->camera)) {
            entangle_camera_set_viewfinder_async(priv->camera,
                                                 FALSE,
                                                 NULL,
                                                 do_entangle_camera_viewfinder_finish,
                                                 data);
        } else {
            g_cancellable_reset(data->confirm);
            entangle_camera_capture_image_async(priv->camera,
                                                data->cancel,
                                                do_entangle_camera_capture_finish,
                                                data);
        }
    } else {
        entangle_camera_preview_image_async(priv->camera,
                                            data->cancel,
                                            do_entangle_camera_preview_finish,
                                            data);
    }
}


void entangle_camera_automata_preview_async(EntangleCameraAutomata *automata,
                                            GCancellable *cancel,
                                            GCancellable *confirm,
                                            GAsyncReadyCallback callback,
                                            gpointer user_data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata));

    EntangleCameraAutomataPrivate *priv = automata->priv;
    EntangleCameraAutomataData *data;
    GSimpleAsyncResult *result;

    result = g_simple_async_result_new(G_OBJECT(automata),
                                       callback,
                                       user_data,
                                       entangle_camera_automata_capture_async);
    data = entangle_camera_automata_data_new(automata,
                                             cancel,
                                             confirm,
                                             result);

    entangle_camera_preview_image_async(priv->camera,
                                        cancel,
                                        do_entangle_camera_preview_finish,
                                        data);

    g_object_unref(result);
}


gboolean entangle_camera_automata_preview_finish(EntangleCameraAutomata *automata,
                                                 GAsyncResult *res,
                                                 GError **error)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata), FALSE);

    if (g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(res),
                                              error))
        return FALSE;

    return TRUE;
}


void entangle_camera_automata_set_camera(EntangleCameraAutomata *automata,
                                         EntangleCamera *camera)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata));
    g_return_if_fail(!camera || ENTANGLE_IS_CAMERA(camera));

    EntangleCameraAutomataPrivate *priv = automata->priv;

    if (priv->camera) {
        g_signal_handler_disconnect(priv->camera, priv->sigFileDownload);
        g_signal_handler_disconnect(priv->camera, priv->sigFileAdd);

        g_object_unref(priv->camera);
        priv->camera = NULL;
    }

    if (camera) {
        priv->camera = g_object_ref(camera);

        priv->sigFileDownload = g_signal_connect(priv->camera,
                                                 "camera-file-downloaded",
                                                 G_CALLBACK(do_entangle_camera_file_download),
                                                 automata);
        priv->sigFileAdd = g_signal_connect(priv->camera,
                                            "camera-file-added",
                                            G_CALLBACK(do_entangle_camera_file_add),
                                            automata);
    }
}


/**
 * entangle_camera_automata_get_camera:
 * @automata: (transfer none): the automata object
 *
 * Get the camera associated with the automata
 *
 * Returns: (transfer none): the camera or NULL
 */
EntangleCamera *entangle_camera_automata_get_camera(EntangleCameraAutomata *automata)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata), NULL);

    EntangleCameraAutomataPrivate *priv = automata->priv;

    return priv->camera;
}


void entangle_camera_automata_set_session(EntangleCameraAutomata *automata,
                                          EntangleSession *session)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata));
    g_return_if_fail(!session || ENTANGLE_IS_SESSION(session));

    EntangleCameraAutomataPrivate *priv = automata->priv;

    if (priv->session) {
        g_object_unref(priv->session);
        priv->session = NULL;
    }

    if (session) {
        priv->session = g_object_ref(session);
    }
}


/**
 * entangle_camera_automata_get_session:
 * @automata: (transfer none): the automata object
 *
 * Get the session associated with the automata
 *
 * Returns: (transfer none): the session or NULL
 */
EntangleSession *entangle_camera_automata_get_session(EntangleCameraAutomata *automata)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata), NULL);

    EntangleCameraAutomataPrivate *priv = automata->priv;

    return priv->session;
}


void entangle_camera_automata_set_delete_file(EntangleCameraAutomata *automata,
                                              gboolean value)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata));

    EntangleCameraAutomataPrivate *priv = automata->priv;

    priv->deleteFile = value;
}


gboolean entangle_camera_automata_get_delete_file(EntangleCameraAutomata *automata)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata), TRUE);

    EntangleCameraAutomataPrivate *priv = automata->priv;

    return priv->deleteFile;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
