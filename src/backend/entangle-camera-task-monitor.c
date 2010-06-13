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
#include "entangle-cancellable.h"

#define ENTANGLE_CAMERA_TASK_MONITOR_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_TASK_MONITOR, EntangleCameraTaskMonitorPrivate))

static void entangle_camera_task_monitor_init_cancellable(gpointer g_iface,
                                                          gpointer iface_data G_GNUC_UNUSED);

struct _EntangleCameraTaskMonitorPrivate {
    GMutex *lock;
    gboolean cancelled;
};

G_DEFINE_TYPE_EXTENDED(EntangleCameraTaskMonitor, entangle_camera_task_monitor, ENTANGLE_TYPE_CAMERA_TASK, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_CANCELLABLE, entangle_camera_task_monitor_init_cancellable));



static void entangle_camera_task_monitor_finalize(GObject *object)
{
    EntangleCameraTaskMonitor *task = ENTANGLE_CAMERA_TASK_MONITOR(object);
    EntangleCameraTaskMonitorPrivate *priv = task->priv;
    ENTANGLE_DEBUG("Finalize camera %p", object);

    g_mutex_free(priv->lock);

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
    EntangleCameraTaskMonitorPrivate *priv = ENTANGLE_CAMERA_TASK_MONITOR(task)->priv;
    gulong sig;

    sig = g_signal_connect(camera, "camera-file-added",
                           G_CALLBACK(do_camera_file_added), task);

    g_mutex_lock(priv->lock);
    while (!priv->cancelled) {
        g_mutex_unlock(priv->lock);
        ENTANGLE_DEBUG("Wait for event");
        if (!entangle_camera_event_wait(camera, 500)) {
            g_mutex_lock(priv->lock);
            break;
        }
        g_mutex_lock(priv->lock);
    }
    g_mutex_unlock(priv->lock);

    g_signal_handler_disconnect(camera, sig);

    return TRUE;
}


static void entangle_camera_task_monitor_cancel(EntangleCancellable *con)
{
    EntangleCameraTaskMonitor *task = ENTANGLE_CAMERA_TASK_MONITOR(con);
    EntangleCameraTaskMonitorPrivate *priv = task->priv;

    g_mutex_lock(priv->lock);
    priv->cancelled = TRUE;
    g_mutex_unlock(priv->lock);
}


static void entangle_camera_task_monitor_cancel_reset(EntangleCancellable *con)
{
    EntangleCameraTaskMonitor *task = ENTANGLE_CAMERA_TASK_MONITOR(con);
    EntangleCameraTaskMonitorPrivate *priv = task->priv;

    g_mutex_lock(priv->lock);
    priv->cancelled = FALSE;
    g_mutex_unlock(priv->lock);
}


static gboolean entangle_camera_task_monitor_is_cancelled(EntangleCancellable *con)
{
    EntangleCameraTaskMonitor *task = ENTANGLE_CAMERA_TASK_MONITOR(con);
    EntangleCameraTaskMonitorPrivate *priv = task->priv;
    gboolean ret;

    g_mutex_lock(priv->lock);
    ret = priv->cancelled;
    g_mutex_unlock(priv->lock);

    return ret;
}


static void entangle_camera_task_monitor_init_cancellable(gpointer g_iface,
                                                          gpointer iface_data G_GNUC_UNUSED)
{
    EntangleCancellableInterface *iface = g_iface;
    iface->cancel = entangle_camera_task_monitor_cancel;
    iface->reset = entangle_camera_task_monitor_cancel_reset;
    iface->is_cancelled = entangle_camera_task_monitor_is_cancelled;
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
