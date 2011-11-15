/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <unistd.h>

#include "entangle-debug.h"
#include "entangle-camera-task-preview.h"
#include "entangle-confirmable.h"
#include "entangle-cancellable.h"

static void entangle_camera_task_preview_init_confirmable(gpointer g_iface,
                                                          gpointer iface_data G_GNUC_UNUSED);
static void entangle_camera_task_preview_init_cancellable(gpointer g_iface,
                                                          gpointer iface_data G_GNUC_UNUSED);

#define ENTANGLE_CAMERA_TASK_PREVIEW_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_TASK_PREVIEW, EntangleCameraTaskPreviewPrivate))

struct _EntangleCameraTaskPreviewPrivate {
    GMutex *lock;
    gboolean confirmed;
    gboolean cancelled;
};

G_DEFINE_TYPE_EXTENDED(EntangleCameraTaskPreview, entangle_camera_task_preview, ENTANGLE_TYPE_CAMERA_TASK, 0,
                       {
                           G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_CANCELLABLE, entangle_camera_task_preview_init_cancellable)
                           G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_CONFIRMABLE, entangle_camera_task_preview_init_confirmable)
                       });



static void entangle_camera_task_preview_finalize(GObject *object)
{
    EntangleCameraTaskPreview *task = ENTANGLE_CAMERA_TASK_PREVIEW(object);
    EntangleCameraTaskPreviewPrivate *priv = task->priv;
    ENTANGLE_DEBUG("Finalize camera preview task %p", object);

    g_mutex_free(priv->lock);

    G_OBJECT_CLASS (entangle_camera_task_preview_parent_class)->finalize (object);
}

#define FRAMES_PER_SECOND 15


static void do_camera_file_deleted(GObject *src,
                                   GAsyncResult *res,
                                   gpointer user_data)
{
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    EntangleCameraFile *file = ENTANGLE_CAMERA_FILE(user_data);
    GError *error = NULL;

    if (!entangle_camera_delete_file_finish(camera, res, &error)) {
        ENTANGLE_DEBUG("Failed delete file %s", error ? error->message : NULL);
        g_error_free(error);
        /* Fallthrough to unref */
    }
    g_object_unref(file);
}


static void do_camera_file_downloaded(GObject *src,
                                      GAsyncResult *res,
                                      gpointer user_data)
{
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    EntangleCameraFile *file = ENTANGLE_CAMERA_FILE(user_data);
    GError *error = NULL;

    if (!entangle_camera_download_file_finish(camera, res, &error)) {
        ENTANGLE_DEBUG("Failed to download file %s", error ? error->message : NULL);
        g_error_free(error);
        /* Fallthrough to delete anyway */
    }

    entangle_camera_delete_file_async(camera,
                                      file,
                                      NULL,
                                      do_camera_file_deleted,
                                      file);
}


static gboolean entangle_camera_task_preview_execute(EntangleCameraTask *task,
                                                     EntangleCamera *camera,
                                                     GError **error)
{
    EntangleCameraTaskPreviewPrivate *priv = ENTANGLE_CAMERA_TASK_PREVIEW(task)->priv;
    EntangleCameraFile *file = NULL;
    EntangleProgress *progress;

    g_object_get(camera, "progress", &progress, NULL);

    g_mutex_lock(priv->lock);

    while (!priv->confirmed) {
        if (priv->cancelled) {
            g_mutex_unlock(priv->lock);
            /* To cancel live view mode we need to capture
             * an image and discard it */
            if ((file = entangle_camera_capture_image(camera, error)) &&
                (!entangle_camera_delete_file(camera, file, error))) {
                ENTANGLE_DEBUG("Failed delete preview capture");
                goto error;
            }
            goto done;
        }
        g_mutex_unlock(priv->lock);

        ENTANGLE_DEBUG("Starting preview");
        if (!(file = entangle_camera_preview_image(camera, error))) {
            ENTANGLE_DEBUG("Failed preview");
            goto error;
        }

        g_object_unref(file);

        g_mutex_lock(priv->lock);
        if (priv->cancelled) {
            g_mutex_unlock(priv->lock);
            goto error;
        }
        g_mutex_unlock(priv->lock);

        g_usleep(1000*1000/FRAMES_PER_SECOND);

        g_mutex_lock(priv->lock);
    }

    g_mutex_unlock(priv->lock);

    ENTANGLE_DEBUG("Starting capture");
    if (!(file = entangle_camera_capture_image(camera, error))) {
        ENTANGLE_DEBUG("Failed preview");
        goto error;
    }

    entangle_camera_download_file_async(camera,
                                        file,
                                        NULL,
                                        do_camera_file_downloaded,
                                        file);

 done:
    return TRUE;

 error:
    if (file)
        g_object_unref(file);
    return FALSE;
}

static void entangle_camera_task_preview_confirm(EntangleConfirmable *con)
{
    EntangleCameraTaskPreview *task = ENTANGLE_CAMERA_TASK_PREVIEW(con);
    EntangleCameraTaskPreviewPrivate *priv = task->priv;

    g_mutex_lock(priv->lock);
    priv->confirmed = TRUE;
    g_mutex_unlock(priv->lock);
}


static void entangle_camera_task_preview_confirm_reset(EntangleConfirmable *con)
{
    EntangleCameraTaskPreview *task = ENTANGLE_CAMERA_TASK_PREVIEW(con);
    EntangleCameraTaskPreviewPrivate *priv = task->priv;

    g_mutex_lock(priv->lock);
    priv->confirmed = FALSE;
    g_mutex_unlock(priv->lock);
}


static gboolean entangle_camera_task_preview_is_confirmed(EntangleConfirmable *con)
{
    EntangleCameraTaskPreview *task = ENTANGLE_CAMERA_TASK_PREVIEW(con);
    EntangleCameraTaskPreviewPrivate *priv = task->priv;
    gboolean ret;

    g_mutex_lock(priv->lock);
    ret = priv->confirmed;
    g_mutex_unlock(priv->lock);

    return ret;
}


static void entangle_camera_task_preview_cancel(EntangleCancellable *con)
{
    EntangleCameraTaskPreview *task = ENTANGLE_CAMERA_TASK_PREVIEW(con);
    EntangleCameraTaskPreviewPrivate *priv = task->priv;

    g_mutex_lock(priv->lock);
    priv->cancelled = TRUE;
    g_mutex_unlock(priv->lock);
}


static void entangle_camera_task_preview_cancel_reset(EntangleCancellable *con)
{
    EntangleCameraTaskPreview *task = ENTANGLE_CAMERA_TASK_PREVIEW(con);
    EntangleCameraTaskPreviewPrivate *priv = task->priv;

    g_mutex_lock(priv->lock);
    priv->cancelled = FALSE;
    g_mutex_unlock(priv->lock);
}


static gboolean entangle_camera_task_preview_is_cancelled(EntangleCancellable *con)
{
    EntangleCameraTaskPreview *task = ENTANGLE_CAMERA_TASK_PREVIEW(con);
    EntangleCameraTaskPreviewPrivate *priv = task->priv;
    gboolean ret;

    g_mutex_lock(priv->lock);
    ret = priv->cancelled;
    g_mutex_unlock(priv->lock);

    return ret;
}


static void entangle_camera_task_preview_init_confirmable(gpointer g_iface,
                                                          gpointer iface_data G_GNUC_UNUSED)
{
    EntangleConfirmableInterface *iface = g_iface;
    iface->confirm = entangle_camera_task_preview_confirm;
    iface->reset = entangle_camera_task_preview_confirm_reset;
    iface->is_confirmed = entangle_camera_task_preview_is_confirmed;
}


static void entangle_camera_task_preview_init_cancellable(gpointer g_iface,
                                                          gpointer iface_data G_GNUC_UNUSED)
{
    EntangleCancellableInterface *iface = g_iface;
    iface->cancel = entangle_camera_task_preview_cancel;
    iface->reset = entangle_camera_task_preview_cancel_reset;
    iface->is_cancelled = entangle_camera_task_preview_is_cancelled;
}


static void entangle_camera_task_preview_class_init(EntangleCameraTaskPreviewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    EntangleCameraTaskClass *task_class = ENTANGLE_CAMERA_TASK_CLASS (klass);

    object_class->finalize = entangle_camera_task_preview_finalize;
    task_class->execute = entangle_camera_task_preview_execute;

    ENTANGLE_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(EntangleCameraTaskPreviewPrivate));
}


EntangleCameraTaskPreview *entangle_camera_task_preview_new(void)
{
    return ENTANGLE_CAMERA_TASK_PREVIEW(g_object_new(ENTANGLE_TYPE_CAMERA_TASK_PREVIEW,
                                                     "name", "preview",
                                                     "label", "Preview an image",
                                                     NULL));
}


static void entangle_camera_task_preview_init(EntangleCameraTaskPreview *task)
{
    EntangleCameraTaskPreviewPrivate *priv;

    priv = task->priv = ENTANGLE_CAMERA_TASK_PREVIEW_GET_PRIVATE(task);

    priv->lock = g_mutex_new();
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
