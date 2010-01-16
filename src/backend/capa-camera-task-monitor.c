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
#include "capa-camera-task-monitor.h"
#include "capa-progress.h"

#define CAPA_CAMERA_TASK_MONITOR_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_TASK_MONITOR, CapaCameraTaskMonitorPrivate))

struct _CapaCameraTaskMonitorPrivate {
    int dummy;
};

G_DEFINE_TYPE(CapaCameraTaskMonitor, capa_camera_task_monitor, CAPA_TYPE_CAMERA_TASK);


static void capa_camera_task_monitor_finalize(GObject *object)
{
    CAPA_DEBUG("Finalize camera %p", object);

    G_OBJECT_CLASS (capa_camera_task_monitor_parent_class)->finalize (object);
}

static void do_camera_file_added(CapaCamera *camera,
                                 CapaCameraFile *file,
                                 CapaCameraTask *task G_GNUC_UNUSED)
{
    capa_camera_download_file(camera, file);

    if (!capa_camera_delete_file(camera, file)) {
        CAPA_DEBUG("Failed delete file");
    }
}

static gboolean capa_camera_task_monitor_execute(CapaCameraTask *task,
                                                 CapaCamera *camera)
{
    gulong sig;
    CapaProgress *progress;

    g_object_get(camera, "progress", &progress, NULL);

    sig = g_signal_connect(camera, "camera-file-added",
                           G_CALLBACK(do_camera_file_added), task);

    while (!capa_progress_cancelled(progress)) {
        CAPA_DEBUG("Wait for event");
        capa_camera_event_wait(camera, 500);
    }

    g_signal_handler_disconnect(camera, sig);

    return TRUE;
}


static void capa_camera_task_monitor_class_init(CapaCameraTaskMonitorClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    CapaCameraTaskClass *task_class = CAPA_CAMERA_TASK_CLASS (klass);

    object_class->finalize = capa_camera_task_monitor_finalize;
    task_class->execute = capa_camera_task_monitor_execute;

    CAPA_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(CapaCameraTaskMonitorPrivate));
}


CapaCameraTaskMonitor *capa_camera_task_monitor_new(void)
{
    return CAPA_CAMERA_TASK_MONITOR(g_object_new(CAPA_TYPE_CAMERA_TASK_MONITOR,
                                                 "name", "monitor",
                                                 "label", "Monitor for new images",
                                                 NULL));
}


static void capa_camera_task_monitor_init(CapaCameraTaskMonitor *task)
{
    CapaCameraTaskMonitorPrivate *priv;

    priv = task->priv = CAPA_CAMERA_TASK_MONITOR_GET_PRIVATE(task);
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
