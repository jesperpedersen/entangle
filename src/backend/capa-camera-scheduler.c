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
#include "capa-camera-scheduler.h"

#define CAPA_CAMERA_SCHEDULER_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_SCHEDULER, CapaCameraSchedulerPrivate))

struct _CapaCameraSchedulerPrivate {
    CapaCamera *camera;

    GThread *worker;
    gboolean cancelled;

    GAsyncQueue *tasks;
};

G_DEFINE_TYPE(CapaCameraScheduler, capa_camera_scheduler, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_CAMERA,
};


static void capa_camera_scheduler_get_property(GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
    CapaCameraScheduler *scheduler = CAPA_CAMERA_SCHEDULER(object);
    CapaCameraSchedulerPrivate *priv = scheduler->priv;

    switch (prop_id)
        {
        case PROP_CAMERA:
            g_value_set_object(value, priv->camera);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_camera_scheduler_set_property(GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
    CapaCameraScheduler *scheduler = CAPA_CAMERA_SCHEDULER(object);
    CapaCameraSchedulerPrivate *priv = scheduler->priv;

    switch (prop_id)
        {
        case PROP_CAMERA:
            if (priv->camera)
                g_object_unref(priv->camera);
            priv->camera = g_value_get_object(value);
            if (priv->camera)
                g_object_ref(priv->camera);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void capa_camera_scheduler_finalize(GObject *object)
{
    CapaCameraScheduler *scheduler = CAPA_CAMERA_SCHEDULER(object);
    CapaCameraSchedulerPrivate *priv = scheduler->priv;

    CAPA_DEBUG("Finalize camera %p", object);

    while (g_async_queue_length(priv->tasks) > 0) {
        CapaCameraTask *task = g_async_queue_pop(priv->tasks);
        g_object_unref(task);
    }
    g_async_queue_unref(priv->tasks);

    g_object_unref(priv->camera);

    G_OBJECT_CLASS (capa_camera_scheduler_parent_class)->finalize (object);
}


static void capa_camera_scheduler_class_init(CapaCameraSchedulerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_camera_scheduler_finalize;
    object_class->get_property = capa_camera_scheduler_get_property;
    object_class->set_property = capa_camera_scheduler_set_property;

    g_signal_new("camera-scheduler-task-begin",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraSchedulerClass, camera_scheduler_task_begin),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 CAPA_TYPE_CAMERA_TASK);


    g_signal_new("camera-scheduler-task-end",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraSchedulerClass, camera_scheduler_task_end),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 CAPA_TYPE_CAMERA_TASK);



    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to manage",
                                                        CAPA_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(CapaCameraSchedulerPrivate));
}


CapaCameraScheduler *capa_camera_scheduler_new(CapaCamera *camera)
{
    return CAPA_CAMERA_SCHEDULER(g_object_new(CAPA_TYPE_CAMERA_SCHEDULER,
                                              "camera", camera,
                                              NULL));
}


static void capa_camera_scheduler_init(CapaCameraScheduler *scheduler)
{
    CapaCameraSchedulerPrivate *priv;

    priv = scheduler->priv = CAPA_CAMERA_SCHEDULER_GET_PRIVATE(scheduler);

    priv->tasks = g_async_queue_new();
}


static gpointer capa_camera_scheduler_worker(gpointer data)
{
    CapaCameraScheduler *scheduler = data;
    CapaCameraSchedulerPrivate *priv = scheduler->priv;

    CAPA_DEBUG("Camera scheduler worker active");

    while (!priv->cancelled) {
        while (g_async_queue_length(priv->tasks) > 0) {
            CapaCameraTask *task = g_async_queue_pop(priv->tasks);
            CAPA_DEBUG("Running task %p on camera %p", task, priv->camera);
            g_signal_emit_by_name(scheduler, "camera-scheduler-task-begin", task);
            capa_camera_task_execute(task, priv->camera);
            g_signal_emit_by_name(scheduler, "camera-scheduler-task-end", task);
            CAPA_DEBUG("Finished task %p on camera %p", task, priv->camera);
            capa_camera_event_flush(priv->camera);
            CAPA_DEBUG("Flush events", priv->camera);
            g_object_unref(task);
        }

        if (!capa_camera_event_wait(priv->camera, 500)) {
            CAPA_DEBUG("Failed when waiting for events");
            break;
        }
    }

    CAPA_DEBUG("Camera scheduler worker quit");
    while (g_async_queue_length(priv->tasks) > 0) {
        CapaCameraTask *task = g_async_queue_pop(priv->tasks);
        g_object_unref(task);
    }
    g_object_unref(scheduler);
    return NULL;
}


gboolean capa_camera_scheduler_start(CapaCameraScheduler *scheduler)
{
    CapaCameraSchedulerPrivate *priv = scheduler->priv;

    if (priv->worker)
        return FALSE;

    /* Keep a extra ref while the BG thread is active */
    g_object_ref(scheduler);

    priv->cancelled = FALSE;
    priv->worker = g_thread_create(capa_camera_scheduler_worker,
                                   scheduler,
                                   FALSE,
                                   NULL);
    if (!priv->worker)
        return FALSE;

    return TRUE;
}


gboolean capa_camera_scheduler_end(CapaCameraScheduler *scheduler)
{
    CapaCameraSchedulerPrivate *priv = scheduler->priv;

    if (!priv->worker)
        return FALSE;

    priv->cancelled = TRUE;

    return TRUE;
}


gboolean capa_camera_scheduler_queue(CapaCameraScheduler *scheduler,
                                     CapaCameraTask *task)
{
    CapaCameraSchedulerPrivate *priv = scheduler->priv;

    if (!priv->worker)
        return FALSE;

    g_object_ref(task);
    g_async_queue_push(priv->tasks, task);

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
