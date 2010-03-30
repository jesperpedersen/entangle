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
#include "entangle-control-range.h"

#define ENTANGLE_CONTROL_RANGE_GET_PRIVATE(obj)                             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_RANGE, EntangleControlRangePrivate))

struct _EntangleControlRangePrivate {
    float value;
    float min;
    float max;
    float step;
};

G_DEFINE_TYPE(EntangleControlRange, entangle_control_range, ENTANGLE_TYPE_CONTROL);

enum {
    PROP_0,
    PROP_VALUE,
    PROP_RANGE_MIN,
    PROP_RANGE_MAX,
    PROP_RANGE_STEP
};

static void entangle_control_range_get_property(GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
    EntangleControlRange *picker = ENTANGLE_CONTROL_RANGE(object);
    EntangleControlRangePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            g_value_set_float(value, priv->value);
            break;

        case PROP_RANGE_MIN:
            g_value_set_float(value, priv->min);
            break;

        case PROP_RANGE_MAX:
            g_value_set_float(value, priv->max);
            break;

        case PROP_RANGE_STEP:
            g_value_set_float(value, priv->step);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_control_range_set_property(GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
    EntangleControlRange *picker = ENTANGLE_CONTROL_RANGE(object);
    EntangleControlRangePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            priv->value = g_value_get_float(value);
            break;

        case PROP_RANGE_MIN:
            priv->min = g_value_get_float(value);
            break;

        case PROP_RANGE_MAX:
            priv->max = g_value_get_float(value);
            break;

        case PROP_RANGE_STEP:
            priv->step = g_value_get_float(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_control_range_finalize (GObject *object)
{
    G_OBJECT_CLASS (entangle_control_range_parent_class)->finalize (object);
}

static void entangle_control_range_class_init(EntangleControlRangeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_control_range_finalize;
    object_class->get_property = entangle_control_range_get_property;
    object_class->set_property = entangle_control_range_set_property;

    g_object_class_install_property(object_class,
                                    PROP_VALUE,
                                    g_param_spec_float("value",
                                                       "Control value",
                                                       "Current control value",
                                                       -10000000.0,
                                                       10000000.0,
                                                       0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_RANGE_MIN,
                                    g_param_spec_float("range-min",
                                                       "Range minimum",
                                                       "Minimum range value",
                                                       -10000000.0,
                                                       10000000.0,
                                                       0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_RANGE_MAX,
                                    g_param_spec_float("range-max",
                                                       "Range maximum",
                                                       "Maximum range value",
                                                       -10000000.0,
                                                       10000000.0,
                                                       0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_RANGE_STEP,
                                    g_param_spec_float("range-step",
                                                       "Range step",
                                                       "Increment for range steps",
                                                       -10000000.0,
                                                       10000000.0,
                                                       0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(EntangleControlRangePrivate));
}


EntangleControlRange *entangle_control_range_new(const char *path,
                                         int id,
                                         const char *label,
                                         const char *info,
                                         float min,
                                         float max,
                                         float step)
{
    return ENTANGLE_CONTROL_RANGE(g_object_new(ENTANGLE_TYPE_CONTROL_RANGE,
                                           "path", path,
                                           "id", id,
                                           "label", label,
                                           "info", info,
                                           "range-min", min,
                                           "range-max", max,
                                           "range-step", step,
                                           NULL));
}


static void entangle_control_range_init(EntangleControlRange *picker)
{
    EntangleControlRangePrivate *priv;

    priv = picker->priv = ENTANGLE_CONTROL_RANGE_GET_PRIVATE(picker);
}


float entangle_control_range_get_min(EntangleControlRange *range)
{
    EntangleControlRangePrivate *priv = range->priv;

    return priv->min;
}

float entangle_control_range_get_max(EntangleControlRange *range)
{
    EntangleControlRangePrivate *priv = range->priv;

    return priv->max;
}

float entangle_control_range_get_step(EntangleControlRange *range)
{
    EntangleControlRangePrivate *priv = range->priv;

    return priv->step;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
