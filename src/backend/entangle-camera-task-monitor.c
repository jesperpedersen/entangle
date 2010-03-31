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
#include "entangle-camera-task-monitor.h"
#include "entangle-progress.h"

#define ENTANGLE_CAMERA_TASK_MONITOR_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_TASK_MONITOR, EntangleCameraTaskMonitorPrivate))

struct _EntangleCameraTaskMonitorPrivate {
    int dummy;
};

G_DEFINE_TYPE(EntangleCameraTaskMonitor, entangle_camera_task_monitor, ENTANGLE_TYPE_CAMERA_TASK);


static void entangle_camera_task_monitor_finalize(GObject *object)
{
    ENTANGLE_DEBUG("Finalize camera %p", object);

    G_OBJECT_CLASS (entangle_camera_task_monitor_parent_class)->finalize (object);
}

static void do_camera_file_added(EntangleCamera *camera,
                                 EntangleCameraFile *file,
                                 EntangleCameraTask *task G_GNUC_UNUSED)
{
    entangle_camera_download_file(camera, file);

    if (!entangle_camera_delete_file(camera, file)) {
        ENTANGLE_DEBUG("Failed delete file");
    }
}

static gboolean entangle_camera_task_monitor_execute(EntangleCameraTask *task,
                                                 EntangleCamera *camera)
{
    gulong sig;
    EntangleProgress *progress;

    g_object_get(camera, "progress", &progress, NULL);

    sig = g_signal_connect(camera, "camera-file-added",
                           G_CALLBACK(do_camera_file_added), task);

    while (!entangle_progress_cancelled(progress)) {
        ENTANGLE_DEBUG("Wait for event");
        if (!entangle_camera_event_wait(camera, 500))
            break;
    }

    g_signal_handler_disconnect(camera, sig);

    return TRUE;
}


static void entangle_camera_task_monitor_class_init(EntangleCameraTaskMonitorClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    EntangleCameraTaskClass *task_class = ENTANGLE_CAMERA_TASK_CLASS (klass);

    object_class->finalize = entangle_camera_task_monitor_finalize;
    task_class->execute = entangle_camera_task_monitor_execute;

    ENTANGLE_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(EntangleCameraTaskMonitorPrivate));
}


EntangleCameraTaskMonitor *entangle_camera_task_monitor_new(void)
{
    return ENTANGLE_CAMERA_TASK_MONITOR(g_object_new(ENTANGLE_TYPE_CAMERA_TASK_MONITOR,
                                                 "name", "monitor",
                                                 "label", "Monitor for new images",
                                                 NULL));
}


static void entangle_camera_task_monitor_init(EntangleCameraTaskMonitor *task)
{
    EntangleCameraTaskMonitorPrivate *priv;

    priv = task->priv = ENTANGLE_CAMERA_TASK_MONITOR_GET_PRIVATE(task);
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
