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
#include <string.h>

#include "entangle-debug.h"
#include "entangle-control-text.h"

#define ENTANGLE_CONTROL_TEXT_GET_PRIVATE(obj)                              \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_TEXT, EntangleControlTextPrivate))

struct _EntangleControlTextPrivate {
    char *value;
};

G_DEFINE_TYPE(EntangleControlText, entangle_control_text, ENTANGLE_TYPE_CONTROL);

enum {
    PROP_0,
    PROP_VALUE,
};

static void entangle_control_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    EntangleControlText *picker = ENTANGLE_CONTROL_TEXT(object);
    EntangleControlTextPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            g_value_set_string(value, priv->value);
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
    EntangleControlText *picker = ENTANGLE_CONTROL_TEXT(object);
    EntangleControlTextPrivate *priv = picker->priv;
    gchar *newvalue;

    switch (prop_id)
        {
        case PROP_VALUE:
            newvalue = g_value_dup_string(value);
            /* Hack, at least one Nikon D5100 appends 25 zeros
             * on the end of the serial number, so we strip them
             * here.
             */
            if (g_str_equal(entangle_control_get_path(ENTANGLE_CONTROL(object)),
                            "/main/status/serialnumber")) {
                size_t len = strlen(newvalue);
                gboolean match = TRUE;
                for (size_t i = 0; i < 25; i++) {
                    if (newvalue[len-(1+i)] != '0')
                        match = FALSE;
                }
                if (match)
                    newvalue[len-25] = '\0';
            }
            if ((newvalue && !priv->value) ||
                (!newvalue && priv->value) ||
                (newvalue && priv->value &&
                 !g_str_equal(newvalue, priv->value))) {
                g_free(priv->value);
                priv->value = newvalue;
                entangle_control_set_dirty(ENTANGLE_CONTROL(object), TRUE);
            } else {
                g_free(newvalue);
            }
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_control_text_finalize(GObject *object)
{
    EntangleControlText *picker = ENTANGLE_CONTROL_TEXT(object);
    EntangleControlTextPrivate *priv = picker->priv;

    g_free(priv->value);

    G_OBJECT_CLASS(entangle_control_text_parent_class)->finalize(object);
}

static void entangle_control_text_class_init(EntangleControlTextClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_control_text_finalize;
    object_class->get_property = entangle_control_get_property;
    object_class->set_property = entangle_control_set_property;

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


    g_type_class_add_private(klass, sizeof(EntangleControlTextPrivate));
}


EntangleControlText *entangle_control_text_new(const char *path,
                                               int id,
                                               const char *label,
                                               const char *info,
                                               gboolean readonly)
{
    g_return_val_if_fail(path != NULL, NULL);
    g_return_val_if_fail(label != NULL, NULL);

    return ENTANGLE_CONTROL_TEXT(g_object_new(ENTANGLE_TYPE_CONTROL_TEXT,
                                              "path", path,
                                              "id", id,
                                              "label", label,
                                              "info", info,
                                              "readonly", readonly,
                                              NULL));
}


static void entangle_control_text_init(EntangleControlText *picker)
{
    picker->priv = ENTANGLE_CONTROL_TEXT_GET_PRIVATE(picker);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
