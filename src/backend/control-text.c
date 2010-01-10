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

#include "debug.h"
#include "control-text.h"

#define CAPA_CONTROL_TEXT_GET_PRIVATE(obj)                              \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_TEXT, CapaControlTextPrivate))

struct _CapaControlTextPrivate {
    char *value;
};

G_DEFINE_TYPE(CapaControlText, capa_control_text, CAPA_TYPE_CONTROL);

enum {
    PROP_0,
    PROP_VALUE,
};

static void capa_control_get_property(GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
    CapaControlText *picker = CAPA_CONTROL_TEXT(object);
    CapaControlTextPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            g_value_set_string(value, priv->value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_control_set_property(GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
    CapaControlText *picker = CAPA_CONTROL_TEXT(object);
    CapaControlTextPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            g_free(priv->value);
            priv->value = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void capa_control_text_finalize (GObject *object)
{
    CapaControlText *picker = CAPA_CONTROL_TEXT(object);
    CapaControlTextPrivate *priv = picker->priv;

    g_free(priv->value);

    G_OBJECT_CLASS (capa_control_text_parent_class)->finalize (object);
}

static void capa_control_text_class_init(CapaControlTextClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_control_text_finalize;
    object_class->get_property = capa_control_get_property;
    object_class->set_property = capa_control_set_property;

    g_object_class_install_property(object_class,
                                    PROP_VALUE,
                                    g_param_spec_string("value",
                                                        "Control value",
                                                        "Current value of the control",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(CapaControlTextPrivate));
}


CapaControlText *capa_control_text_new(const char *path,
                                       int id,
                                       const char *label,
                                       const char *info)
{
    return CAPA_CONTROL_TEXT(g_object_new(CAPA_TYPE_CONTROL_TEXT,
                                          "path", path,
                                          "id", id,
                                          "label", label,
                                          "info", info,
                                          NULL));
}


static void capa_control_text_init(CapaControlText *picker)
{
    CapaControlTextPrivate *priv;

    priv = picker->priv = CAPA_CONTROL_TEXT_GET_PRIVATE(picker);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
