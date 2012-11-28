/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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

#include <stdio.h>

#include "entangle-debug.h"
#include "entangle-control.h"

#define ENTANGLE_CONTROL_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL, EntangleControlPrivate))

struct _EntangleControlPrivate {
    char *path;
    int id;
    char *label;
    char *info;
    gboolean readonly;
    gboolean dirty;
};

G_DEFINE_ABSTRACT_TYPE(EntangleControl, entangle_control, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_PATH,
    PROP_ID,
    PROP_LABEL,
    PROP_INFO,
    PROP_READONLY,
    PROP_DIRTY
};

static void entangle_control_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    EntangleControl *picker = ENTANGLE_CONTROL(object);
    EntangleControlPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_PATH:
            g_value_set_string(value, priv->path);
            break;

        case PROP_ID:
            g_value_set_int(value, priv->id);
            break;

        case PROP_LABEL:
            g_value_set_string(value, priv->label);
            break;

        case PROP_INFO:
            g_value_set_string(value, priv->info);
            break;

        case PROP_READONLY:
            g_value_set_boolean(value, priv->readonly);
            break;

        case PROP_DIRTY:
            g_value_set_boolean(value, priv->dirty);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_control_set_property(GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
    EntangleControl *picker = ENTANGLE_CONTROL(object);
    EntangleControlPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_PATH:
            g_free(priv->path);
            priv->path = g_value_dup_string(value);
            break;

        case PROP_ID:
            priv->id = g_value_get_int(value);
            break;

        case PROP_LABEL:
            g_free(priv->label);
            priv->label = g_value_dup_string(value);
            break;

        case PROP_INFO:
            g_free(priv->info);
            priv->info = g_value_dup_string(value);
            break;

        case PROP_READONLY:
            priv->readonly = g_value_get_boolean(value);
            break;

        case PROP_DIRTY:
            priv->dirty = g_value_get_boolean(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_control_finalize (GObject *object)
{
    EntangleControl *picker = ENTANGLE_CONTROL(object);
    EntangleControlPrivate *priv = picker->priv;

    g_free(priv->path);
    g_free(priv->label);
    g_free(priv->info);

    G_OBJECT_CLASS (entangle_control_parent_class)->finalize (object);
}

static void entangle_control_class_init(EntangleControlClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_control_finalize;
    object_class->get_property = entangle_control_get_property;
    object_class->set_property = entangle_control_set_property;

    g_object_class_install_property(object_class,
                                    PROP_PATH,
                                    g_param_spec_string("path",
                                                        "Control path",
                                                        "Path of the control",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_ID,
                                    g_param_spec_int("id",
                                                     "Control id",
                                                     "Id of the control",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_LABEL,
                                    g_param_spec_string("label",
                                                        "Control label",
                                                        "Label of the control",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_INFO,
                                    g_param_spec_string("info",
                                                        "Control info",
                                                        "Info of the control",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_READONLY,
                                    g_param_spec_boolean("readonly",
                                                         "Control state",
                                                         "State of the control",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DIRTY,
                                    g_param_spec_boolean("dirty",
                                                         "Dirty",
                                                         "Dirty if value has been changed",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleControlPrivate));
}


EntangleControl *entangle_control_new(const gchar *path,
                                      gint id,
                                      const gchar *label,
                                      const gchar *info,
                                      gboolean readonly)
{
    g_return_val_if_fail(path != NULL, NULL);
    g_return_val_if_fail(label != NULL, NULL);

    return ENTANGLE_CONTROL(g_object_new(ENTANGLE_TYPE_CONTROL,
                                         "path", path,
                                         "id", id,
                                         "label", label,
                                         "info", info,
                                         "readonly", readonly,
                                         NULL));
}


static void entangle_control_init(EntangleControl *control)
{
    control->priv = ENTANGLE_CONTROL_GET_PRIVATE(control);
}


gint entangle_control_get_id(EntangleControl *control)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL(control), 0);

    EntangleControlPrivate *priv = control->priv;

    return priv->id;
}

const gchar *entangle_control_get_path(EntangleControl *control)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL(control), NULL);

    EntangleControlPrivate *priv = control->priv;

    return priv->path;
}

const gchar *entangle_control_get_label(EntangleControl *control)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL(control), NULL);

    EntangleControlPrivate *priv = control->priv;

    return priv->label;
}

const gchar *entangle_control_get_info(EntangleControl *control)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL(control), NULL);

    EntangleControlPrivate *priv = control->priv;

    return priv->info;
}

gboolean entangle_control_get_dirty(EntangleControl *control)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL(control), FALSE);

    EntangleControlPrivate *priv = control->priv;

    return priv->dirty;
}

void entangle_control_set_dirty(EntangleControl *control,
                                gboolean dirty)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL(control));

    EntangleControlPrivate *priv = control->priv;
    gboolean changed = priv->dirty != dirty;

    priv->dirty = dirty;
    if (changed)
        g_object_notify(G_OBJECT(control), "dirty");
}

gboolean entangle_control_get_readonly(EntangleControl *control)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL(control), TRUE);

    EntangleControlPrivate *priv = control->priv;

    return priv->readonly;
}

void entangle_control_set_readonly(EntangleControl *control,
                                   gboolean ro)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL(control));

    EntangleControlPrivate *priv = control->priv;
    gboolean changed = priv->readonly != ro;

    priv->readonly = ro;
    if (changed)
        g_object_notify(G_OBJECT(control), "readonly");
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
