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
#include "capa-camera-task.h"

#define CAPA_CAMERA_TASK_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_TASK, CapaCameraTaskPrivate))

struct _CapaCameraTaskPrivate {
    char *name;
    char *label;
};

G_DEFINE_ABSTRACT_TYPE(CapaCameraTask, capa_camera_task, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_NAME,
    PROP_LABEL,
};


static void capa_camera_task_get_property(GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
    CapaCameraTask *task = CAPA_CAMERA_TASK(object);
    CapaCameraTaskPrivate *priv = task->priv;

    switch (prop_id)
        {
        case PROP_NAME:
            g_value_set_string(value, priv->name);
            break;

        case PROP_LABEL:
            g_value_set_string(value, priv->label);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_camera_task_set_property(GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
    CapaCameraTask *task = CAPA_CAMERA_TASK(object);
    CapaCameraTaskPrivate *priv = task->priv;

    switch (prop_id)
        {
        case PROP_NAME:
            g_free(priv->name);
            priv->name = g_value_dup_string(value);
            break;

        case PROP_LABEL:
            g_free(priv->label);
            priv->label = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void capa_camera_task_finalize(GObject *object)
{
    CapaCameraTask *task = CAPA_CAMERA_TASK(object);
    CapaCameraTaskPrivate *priv = task->priv;

    CAPA_DEBUG("Finalize camera %p", object);

    g_free(priv->label);
    g_free(priv->name);

    G_OBJECT_CLASS (capa_camera_task_parent_class)->finalize (object);
}


static void capa_camera_task_class_init(CapaCameraTaskClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_camera_task_finalize;
    object_class->get_property = capa_camera_task_get_property;
    object_class->set_property = capa_camera_task_set_property;


    g_object_class_install_property(object_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Camera task name",
                                                        "Task name on the camera",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_LABEL,
                                    g_param_spec_string("label",
                                                        "Camera task label",
                                                        "Label name on the camera",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    CAPA_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(CapaCameraTaskPrivate));
}


CapaCameraTask *capa_camera_task_new(const char *name,
                                     const char *label)
{
    return CAPA_CAMERA_TASK(g_object_new(CAPA_TYPE_CAMERA_TASK,
                                         "name", name,
                                         "label", label,
                                         NULL));
}


static void capa_camera_task_init(CapaCameraTask *task)
{
    CapaCameraTaskPrivate *priv;

    priv = task->priv = CAPA_CAMERA_TASK_GET_PRIVATE(task);
}


const char *capa_camera_task_get_label(CapaCameraTask *task)
{
    CapaCameraTaskPrivate *priv = task->priv;
    return priv->label;
}

const char *capa_camera_task_get_name(CapaCameraTask *task)
{
    CapaCameraTaskPrivate *priv = task->priv;
    return priv->name;
}

gboolean capa_camera_task_execute(CapaCameraTask *task,
                                  CapaCamera *camera)
{
    return (CAPA_CAMERA_TASK_GET_CLASS(task)->execute)(task, camera);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
