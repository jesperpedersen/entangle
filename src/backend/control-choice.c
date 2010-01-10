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
#include <string.h>

#include "debug.h"
#include "control-choice.h"

#define CAPA_CONTROL_CHOICE_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_CHOICE, CapaControlChoicePrivate))

struct _CapaControlChoicePrivate {
    char *value;
    size_t nentries;
    char **entries;
};

G_DEFINE_TYPE(CapaControlChoice, capa_control_choice, CAPA_TYPE_CONTROL);

enum {
    PROP_0,
    PROP_VALUE,
};

static void capa_control_get_property(GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
    CapaControlChoice *picker = CAPA_CONTROL_CHOICE(object);
    CapaControlChoicePrivate *priv = picker->priv;

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
    CapaControlChoice *picker = CAPA_CONTROL_CHOICE(object);
    CapaControlChoicePrivate *priv = picker->priv;

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


static void capa_control_choice_finalize (GObject *object)
{
    CapaControlChoice *picker = CAPA_CONTROL_CHOICE(object);
    CapaControlChoicePrivate *priv = picker->priv;

    for (int i = 0 ; i < priv->nentries ; i++)
        g_free(priv->entries[i]);
    g_free(priv->entries);

    G_OBJECT_CLASS (capa_control_choice_parent_class)->finalize (object);
}

static void capa_control_choice_class_init(CapaControlChoiceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_control_choice_finalize;
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

    g_type_class_add_private(klass, sizeof(CapaControlChoicePrivate));
}


CapaControlChoice *capa_control_choice_new(const char *path,
                                           int id,
                                           const char *label,
                                           const char *info)
{
    return CAPA_CONTROL_CHOICE(g_object_new(CAPA_TYPE_CONTROL_CHOICE,
                                            "path", path,
                                            "id", id,
                                            "label", label,
                                            "info", info,
                                            NULL));
}


static void capa_control_choice_init(CapaControlChoice *picker)
{
    CapaControlChoicePrivate *priv;

    priv = picker->priv = CAPA_CONTROL_CHOICE_GET_PRIVATE(picker);
    memset(priv, 0, sizeof *priv);
}

void capa_control_choice_add_entry(CapaControlChoice *choice,
                                   const char *entry)
{
    CapaControlChoicePrivate *priv = choice->priv;

    priv->entries = g_renew(char *, priv->entries, priv->nentries+1);
    priv->entries[priv->nentries++] = g_strdup(entry);
}

int capa_control_choice_entry_count(CapaControlChoice *choice)
{
    CapaControlChoicePrivate *priv = choice->priv;

    return priv->nentries;
}

const char *capa_control_choice_entry_get(CapaControlChoice *choice,
                                          int idx)
{
    CapaControlChoicePrivate *priv = choice->priv;

    if (idx < 0 || idx >= priv->nentries)
        return NULL;

    return priv->entries[idx];
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
