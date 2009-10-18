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

#include <stdio.h>

#include "internal.h"
#include "control-range.h"

#define CAPA_CONTROL_RANGE_GET_PRIVATE(obj)                             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_RANGE, CapaControlRangePrivate))

struct _CapaControlRangePrivate {
    float value;
    float min;
    float max;
    float step;
};

G_DEFINE_TYPE(CapaControlRange, capa_control_range, CAPA_TYPE_CONTROL);

enum {
    PROP_0,
    PROP_VALUE,
    PROP_RANGE_MIN,
    PROP_RANGE_MAX,
    PROP_RANGE_STEP
};

static void capa_control_range_get_property(GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
    CapaControlRange *picker = CAPA_CONTROL_RANGE(object);
    CapaControlRangePrivate *priv = picker->priv;

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

static void capa_control_range_set_property(GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
    CapaControlRange *picker = CAPA_CONTROL_RANGE(object);
    CapaControlRangePrivate *priv = picker->priv;

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


static void capa_control_range_finalize (GObject *object)
{
    G_OBJECT_CLASS (capa_control_range_parent_class)->finalize (object);
}

static void capa_control_range_class_init(CapaControlRangeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_control_range_finalize;
    object_class->get_property = capa_control_range_get_property;
    object_class->set_property = capa_control_range_set_property;

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


    g_type_class_add_private(klass, sizeof(CapaControlRangePrivate));
}


CapaControlRange *capa_control_range_new(const char *path,
                                         int id,
                                         const char *label,
                                         const char *info,
                                         float min,
                                         float max,
                                         float step)
{
    return CAPA_CONTROL_RANGE(g_object_new(CAPA_TYPE_CONTROL_RANGE,
                                           "path", path,
                                           "id", id,
                                           "label", label,
                                           "info", info,
                                           "range-min", min,
                                           "range-max", max,
                                           "range-step", step,
                                           NULL));
}


static void capa_control_range_init(CapaControlRange *picker)
{
    CapaControlRangePrivate *priv;

    priv = picker->priv = CAPA_CONTROL_RANGE_GET_PRIVATE(picker);
}


float capa_control_range_get_min(CapaControlRange *range)
{
    CapaControlRangePrivate *priv = range->priv;

    return priv->min;
}

float capa_control_range_get_max(CapaControlRange *range)
{
    CapaControlRangePrivate *priv = range->priv;

    return priv->max;
}

float capa_control_range_get_step(CapaControlRange *range)
{
    CapaControlRangePrivate *priv = range->priv;

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
