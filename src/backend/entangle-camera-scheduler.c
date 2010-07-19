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
#include "entangle-camera-scheduler.h"

#define ENTANGLE_CAMERA_SCHEDULER_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_SCHEDULER, EntangleCameraSchedulerPrivate))

struct _EntangleCameraSchedulerPrivate {
    EntangleCamera *camera;

    GThread *worker;
    GMutex *lock;
    gboolean cancelled;

    gboolean pause;
    gboolean resume;
    GCond *pauseSignal;
    GCond *resumeSignal;

    GAsyncQueue *tasks;
};

G_DEFINE_TYPE(EntangleCameraScheduler, entangle_camera_scheduler, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_CAMERA,
};


static void entangle_camera_scheduler_get_property(GObject *object,
                                                   guint prop_id,
                                                   GValue *value,
                                                   GParamSpec *pspec)
{
    EntangleCameraScheduler *scheduler = ENTANGLE_CAMERA_SCHEDULER(object);
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

    switch (prop_id)
        {
        case PROP_CAMERA:
            g_value_set_object(value, priv->camera);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_camera_scheduler_set_property(GObject *object,
                                                   guint prop_id,
                                                   const GValue *value,
                                                   GParamSpec *pspec)
{
    EntangleCameraScheduler *scheduler = ENTANGLE_CAMERA_SCHEDULER(object);
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

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


static void entangle_camera_scheduler_finalize(GObject *object)
{
    EntangleCameraScheduler *scheduler = ENTANGLE_CAMERA_SCHEDULER(object);
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

    ENTANGLE_DEBUG("Finalize camera scheduler %p", object);

    while (g_async_queue_length(priv->tasks) > 0) {
        EntangleCameraTask *task = g_async_queue_pop(priv->tasks);
        g_object_unref(task);
    }
    g_async_queue_unref(priv->tasks);

    g_mutex_free(priv->lock);
    g_cond_free(priv->pauseSignal);
    g_cond_free(priv->resumeSignal);

    g_object_unref(priv->camera);

    G_OBJECT_CLASS (entangle_camera_scheduler_parent_class)->finalize (object);
}


static void entangle_camera_scheduler_class_init(EntangleCameraSchedulerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_camera_scheduler_finalize;
    object_class->get_property = entangle_camera_scheduler_get_property;
    object_class->set_property = entangle_camera_scheduler_set_property;

    g_signal_new("camera-scheduler-task-begin",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraSchedulerClass, camera_scheduler_task_begin),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA_TASK);


    g_signal_new("camera-scheduler-task-end",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraSchedulerClass, camera_scheduler_task_end),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA_TASK);



    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to manage",
                                                        ENTANGLE_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleCameraSchedulerPrivate));
}


EntangleCameraScheduler *entangle_camera_scheduler_new(EntangleCamera *camera)
{
    return ENTANGLE_CAMERA_SCHEDULER(g_object_new(ENTANGLE_TYPE_CAMERA_SCHEDULER,
                                              "camera", camera,
                                              NULL));
}


static void entangle_camera_scheduler_init(EntangleCameraScheduler *scheduler)
{
    EntangleCameraSchedulerPrivate *priv;

    priv = scheduler->priv = ENTANGLE_CAMERA_SCHEDULER_GET_PRIVATE(scheduler);

    priv->tasks = g_async_queue_new();
    priv->lock = g_mutex_new();
    priv->pauseSignal = g_cond_new();
    priv->resumeSignal = g_cond_new();
}


static gpointer entangle_camera_scheduler_worker(gpointer data)
{
    EntangleCameraScheduler *scheduler = data;
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

    ENTANGLE_DEBUG("Camera scheduler worker active");

    g_mutex_lock(priv->lock);

    while (!priv->cancelled &&
           entangle_camera_get_connected(priv->camera)) {

        g_mutex_unlock(priv->lock);

        while (g_async_queue_length(priv->tasks) > 0) {
            EntangleCameraTask *task = g_async_queue_pop(priv->tasks);
            GError *error = NULL;
            ENTANGLE_DEBUG("Running task %p on camera %p", task, priv->camera);

            g_signal_emit_by_name(scheduler, "camera-scheduler-task-begin", task);
            if (!entangle_camera_task_execute(task, priv->camera, &error)) {
                ENTANGLE_DEBUG("Failed task %p on camera %p: %s",
                               task, priv->camera, error->message);
                g_error_free(error);
            }
            g_signal_emit_by_name(scheduler, "camera-scheduler-task-end", task);

            ENTANGLE_DEBUG("Finished task %p on camera %p", task, priv->camera);

            if (!entangle_camera_get_connected(priv->camera)) {
                g_object_unref(task);
                g_mutex_lock(priv->lock);
                break;
            }

            ENTANGLE_DEBUG("Flush events %p", priv->camera);
            entangle_camera_event_flush(priv->camera, NULL);

            g_object_unref(task);
        }

        if (!entangle_camera_get_connected(priv->camera)) {
            g_mutex_lock(priv->lock);
            break;
        }

        if (!entangle_camera_event_wait(priv->camera, 500, NULL)) {
            ENTANGLE_DEBUG("Failed when waiting for events");
            g_mutex_lock(priv->lock);
            break;
        }

        g_mutex_lock(priv->lock);

        if (priv->pause) {
            ENTANGLE_DEBUG("Signalling that we are not paused");
            priv->pause = FALSE;
            priv->resume = FALSE;
            g_cond_signal(priv->pauseSignal);

            ENTANGLE_DEBUG("Waiting for resume");
            while (!priv->resume)
                g_cond_wait(priv->resumeSignal, priv->lock);
            ENTANGLE_DEBUG("We are now resumed");
        }
    }

    priv->worker = NULL;
    g_mutex_unlock(priv->lock);

    ENTANGLE_DEBUG("Camera scheduler worker done, purging tasks");
    while (g_async_queue_length(priv->tasks) > 0) {
        EntangleCameraTask *task = g_async_queue_pop(priv->tasks);
        g_object_unref(task);
    }
    g_object_unref(scheduler);

    ENTANGLE_DEBUG("Camera scheduler worker quit %p", scheduler);
    return NULL;
}


gboolean entangle_camera_scheduler_start(EntangleCameraScheduler *scheduler)
{
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

    if (priv->worker)
        return FALSE;

    /* Keep a extra ref while the BG thread is active */
    g_object_ref(scheduler);

    priv->cancelled = FALSE;
    priv->worker = g_thread_create(entangle_camera_scheduler_worker,
                                   scheduler,
                                   FALSE,
                                   NULL);
    if (!priv->worker)
        return FALSE;

    return TRUE;
}


gboolean entangle_camera_scheduler_end(EntangleCameraScheduler *scheduler)
{
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

    if (!priv->worker)
        return FALSE;

    g_mutex_lock(priv->lock);
    priv->cancelled = TRUE;
    g_mutex_unlock(priv->lock);

    return TRUE;
}


gboolean entangle_camera_scheduler_pause(EntangleCameraScheduler *scheduler)
{
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

    if (!priv->worker)
        return FALSE;

    g_mutex_lock(priv->lock);
    priv->pause = TRUE;
    ENTANGLE_DEBUG("Waiting for thread to pause");
    while (priv->pause)
        g_cond_wait(priv->pauseSignal, priv->lock);
    ENTANGLE_DEBUG("Thread is now paused");
    g_mutex_unlock(priv->lock);
    
    return TRUE;
}


gboolean entangle_camera_scheduler_resume(EntangleCameraScheduler *scheduler)
{
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

    if (!priv->worker)
        return FALSE;

    g_mutex_lock(priv->lock);
    ENTANGLE_DEBUG("Signalling thread to resume");
    priv->resume = TRUE;
    g_cond_signal(priv->resumeSignal);
    g_mutex_unlock(priv->lock);

    return TRUE;
}


gboolean entangle_camera_scheduler_queue(EntangleCameraScheduler *scheduler,
                                     EntangleCameraTask *task)
{
    EntangleCameraSchedulerPrivate *priv = scheduler->priv;

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
