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
#include "entangle-camera-task-capture.h"

#define ENTANGLE_CAMERA_TASK_CAPTURE_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_TASK_CAPTURE, EntangleCameraTaskCapturePrivate))

struct _EntangleCameraTaskCapturePrivate {
    int dummy;
};

G_DEFINE_TYPE(EntangleCameraTaskCapture, entangle_camera_task_capture, ENTANGLE_TYPE_CAMERA_TASK);


static void entangle_camera_task_capture_finalize(GObject *object)
{
    ENTANGLE_DEBUG("Finalize camera capture task %p", object);

    G_OBJECT_CLASS (entangle_camera_task_capture_parent_class)->finalize (object);
}

static gboolean entangle_camera_task_capture_execute(EntangleCameraTask *task G_GNUC_UNUSED,
                                                     EntangleCamera *camera,
                                                     GError **error)
{
    EntangleCameraFile *file;

    ENTANGLE_DEBUG("Starting capture");
    if (!(file = entangle_camera_capture_image(camera, error))) {
        ENTANGLE_DEBUG("Failed capture");
        goto error;
    }

    if (!entangle_camera_download_file(camera, file, error)) {
        ENTANGLE_DEBUG("Failed download");
        goto error_delete;
    }

    if (!entangle_camera_delete_file(camera, file, error)) {
        ENTANGLE_DEBUG("Failed delete file");
        goto error;
    }

    g_object_unref(file);

    return TRUE;

 error_delete:
    if (!entangle_camera_delete_file(camera, file, NULL)) {
        goto error;
    }

 error:
    if (file)
        g_object_unref(file);
    return FALSE;
}


static void entangle_camera_task_capture_class_init(EntangleCameraTaskCaptureClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    EntangleCameraTaskClass *task_class = ENTANGLE_CAMERA_TASK_CLASS (klass);

    object_class->finalize = entangle_camera_task_capture_finalize;
    task_class->execute = entangle_camera_task_capture_execute;

    ENTANGLE_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(EntangleCameraTaskCapturePrivate));
}


EntangleCameraTaskCapture *entangle_camera_task_capture_new(void)
{
    return ENTANGLE_CAMERA_TASK_CAPTURE(g_object_new(ENTANGLE_TYPE_CAMERA_TASK_CAPTURE,
                                                 "name", "capture",
                                                 "label", "Capture an image",
                                                 NULL));
}


static void entangle_camera_task_capture_init(EntangleCameraTaskCapture *task)
{
    task->priv = ENTANGLE_CAMERA_TASK_CAPTURE_GET_PRIVATE(task);
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
