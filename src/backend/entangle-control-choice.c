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
#include "entangle-control-choice.h"

#define ENTANGLE_CONTROL_CHOICE_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_CHOICE, EntangleControlChoicePrivate))

struct _EntangleControlChoicePrivate {
    char *value;
    size_t nentries;
    char **entries;
};

G_DEFINE_TYPE(EntangleControlChoice, entangle_control_choice, ENTANGLE_TYPE_CONTROL);

enum {
    PROP_0,
    PROP_VALUE,
};

static void entangle_control_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    EntangleControlChoice *picker = ENTANGLE_CONTROL_CHOICE(object);
    EntangleControlChoicePrivate *priv = picker->priv;

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
    EntangleControlChoice *picker = ENTANGLE_CONTROL_CHOICE(object);
    EntangleControlChoicePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_VALUE:
            g_free(priv->value);
            priv->value = g_value_dup_string(value);
            entangle_control_set_dirty(ENTANGLE_CONTROL(object), TRUE);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_control_choice_finalize (GObject *object)
{
    EntangleControlChoice *picker = ENTANGLE_CONTROL_CHOICE(object);
    EntangleControlChoicePrivate *priv = picker->priv;

    for (int i = 0 ; i < priv->nentries ; i++)
        g_free(priv->entries[i]);
    g_free(priv->entries);

    G_OBJECT_CLASS (entangle_control_choice_parent_class)->finalize (object);
}

static void entangle_control_choice_class_init(EntangleControlChoiceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_control_choice_finalize;
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

    g_type_class_add_private(klass, sizeof(EntangleControlChoicePrivate));
}


EntangleControlChoice *entangle_control_choice_new(const char *path,
                                                   int id,
                                                   const char *label,
                                                   const char *info,
                                                   gboolean readonly)
{
    return ENTANGLE_CONTROL_CHOICE(g_object_new(ENTANGLE_TYPE_CONTROL_CHOICE,
                                                "path", path,
                                                "id", id,
                                                "label", label,
                                                "info", info,
                                                "readonly", readonly,
                                                NULL));
}


static void entangle_control_choice_init(EntangleControlChoice *picker)
{
    picker->priv = ENTANGLE_CONTROL_CHOICE_GET_PRIVATE(picker);
}

void entangle_control_choice_add_entry(EntangleControlChoice *choice,
                                       const char *entry)
{
    EntangleControlChoicePrivate *priv = choice->priv;

    priv->entries = g_renew(char *, priv->entries, priv->nentries+1);
    priv->entries[priv->nentries++] = g_strdup(entry);
}

int entangle_control_choice_entry_count(EntangleControlChoice *choice)
{
    EntangleControlChoicePrivate *priv = choice->priv;

    return priv->nentries;
}

const char *entangle_control_choice_entry_get(EntangleControlChoice *choice,
                                              int idx)
{
    EntangleControlChoicePrivate *priv = choice->priv;

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
