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

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <unistd.h>

#include "capa-debug.h"
#include "capa-camera-task-preview.h"

#define CAPA_CAMERA_TASK_PREVIEW_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_TASK_PREVIEW, CapaCameraTaskPreviewPrivate))

struct _CapaCameraTaskPreviewPrivate {
    int dummy;
};

G_DEFINE_TYPE(CapaCameraTaskPreview, capa_camera_task_preview, CAPA_TYPE_CAMERA_TASK);


static void capa_camera_task_preview_finalize(GObject *object)
{
    CAPA_DEBUG("Finalize camera %p", object);

    G_OBJECT_CLASS (capa_camera_task_preview_parent_class)->finalize (object);
}

static gboolean capa_camera_task_preview_execute(CapaCameraTask *task G_GNUC_UNUSED,
                                                 CapaCamera *camera)
{
    CapaCameraFile *file;
    int ntimes = 20;

    while (ntimes-- > 0) {
        CAPA_DEBUG("Starting preview");
        if (!(file = capa_camera_preview_image(camera))) {
            CAPA_DEBUG("Failed preview");
            goto error;
        }

        g_object_unref(file);

        g_usleep(100*1000);
    }

    CAPA_DEBUG("Starting capture");
    if (!(file = capa_camera_capture_image(camera))) {
        CAPA_DEBUG("Failed preview");
        goto error;
    }

    if (!capa_camera_download_file(camera, file)) {
        CAPA_DEBUG("Failed download");
        goto error_delete;
    }

    if (!capa_camera_delete_file(camera, file)) {
        CAPA_DEBUG("Failed delete file");
        goto error;
    }

    g_object_unref(file);

    return TRUE;

 error_delete:
    if (!capa_camera_delete_file(camera, file)) {
        goto error;
    }

 error:
    if (file)
        g_object_unref(file);
    return FALSE;
}


static void capa_camera_task_preview_class_init(CapaCameraTaskPreviewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    CapaCameraTaskClass *task_class = CAPA_CAMERA_TASK_CLASS (klass);

    object_class->finalize = capa_camera_task_preview_finalize;
    task_class->execute = capa_camera_task_preview_execute;

    CAPA_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(CapaCameraTaskPreviewPrivate));
}


CapaCameraTaskPreview *capa_camera_task_preview_new(void)
{
    return CAPA_CAMERA_TASK_PREVIEW(g_object_new(CAPA_TYPE_CAMERA_TASK_PREVIEW,
                                                 "name", "preview",
                                                 "label", "Preview an image",
                                                 NULL));
}


static void capa_camera_task_preview_init(CapaCameraTaskPreview *task)
{
    CapaCameraTaskPreviewPrivate *priv;

    priv = task->priv = CAPA_CAMERA_TASK_PREVIEW_GET_PRIVATE(task);
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
