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

#include <stdio.h>

#include "entangle-debug.h"
#include "entangle-control-date.h"

#define ENTANGLE_CONTROL_DATE_GET_PRIVATE(obj)                              \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_DATE, EntangleControlDatePrivate))

struct _EntangleControlDatePrivate {
    int value;
};

G_DEFINE_TYPE(EntangleControlDate, entangle_control_date, ENTANGLE_TYPE_CONTROL);

enum {
    PROP_0,
    PROP_VALUE,
};

static void entangle_control_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    EntangleControlDate *picker = ENTANGLE_CONTROL_DATE(object);
    EntangleControlDatePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            g_value_set_int(value, priv->value);
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
    EntangleControlDate *picker = ENTANGLE_CONTROL_DATE(object);
    EntangleControlDatePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            priv->value = g_value_get_int(value);
            entangle_control_set_dirty(ENTANGLE_CONTROL(object), TRUE);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_control_date_finalize (GObject *object)
{
    G_OBJECT_CLASS (entangle_control_date_parent_class)->finalize (object);
}

static void entangle_control_date_class_init(EntangleControlDateClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_control_date_finalize;
    object_class->get_property = entangle_control_get_property;
    object_class->set_property = entangle_control_set_property;

    g_object_class_install_property(object_class,
                                    PROP_VALUE,
                                    g_param_spec_int("value",
                                                     "Control value",
                                                     "Current value of the control",
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleControlDatePrivate));
}


EntangleControlDate *entangle_control_date_new(const char *path,
                                               int id,
                                               const char *label,
                                               const char *info,
                                               gboolean readonly)
{
    return ENTANGLE_CONTROL_DATE(g_object_new(ENTANGLE_TYPE_CONTROL_DATE,
                                              "path", path,
                                              "id", id,
                                              "label", label,
                                              "info", info,
                                              "readonly", readonly,
                                              NULL));
}


static void entangle_control_date_init(EntangleControlDate *picker)
{
    picker->priv = ENTANGLE_CONTROL_DATE_GET_PRIVATE(picker);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
