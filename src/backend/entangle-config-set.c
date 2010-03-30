/*
 *  Entangle: Entangle Assists Photograph Aquisition
 *
 *  Copyright (C) 2009-2010 Daniel P. Berrange
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

#include <string.h>

#include "entangle-debug.h"
#include "entangle-config-set.h"

#define ENTANGLE_CONFIG_SET_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONFIG_SET, EntangleConfigSetPrivate))

struct _EntangleConfigSetPrivate {
    char *name;
    char *description;

    GList *entries;
};


G_DEFINE_TYPE(EntangleConfigSet, entangle_config_set, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_NAME,
    PROP_DESCRIPTION,
};


static void entangle_config_set_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    EntangleConfigSet *set = ENTANGLE_CONFIG_SET(object);
    EntangleConfigSetPrivate *priv = set->priv;

    switch (prop_id)
        {
        case PROP_NAME:
            g_value_set_string(value, priv->name);
            break;

        case PROP_DESCRIPTION:
            g_value_set_string(value, priv->description);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_config_set_property(GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
    EntangleConfigSet *set = ENTANGLE_CONFIG_SET(object);
    EntangleConfigSetPrivate *priv = set->priv;

    switch (prop_id)
        {
        case PROP_NAME:
            g_free(priv->name);
            priv->name = g_value_dup_string(value);
            break;

        case PROP_DESCRIPTION:
            g_free(priv->description);
            priv->description = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_config_set_finalize(GObject *object)
{
    EntangleConfigSet *config = ENTANGLE_CONFIG_SET(object);
    EntangleConfigSetPrivate *priv = config->priv;
    GList *tmp;

    ENTANGLE_DEBUG("Finalize config %p", object);

    g_free(priv->name);
    g_free(priv->description);

    tmp = priv->entries;
    while (tmp) {
        g_object_unref(tmp->data);
        tmp = g_list_next(tmp);
    }

    g_list_free(priv->entries);

    G_OBJECT_CLASS (entangle_config_set_parent_class)->finalize (object);
}


static void entangle_config_set_class_init(EntangleConfigSetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_config_set_finalize;
    object_class->get_property = entangle_config_set_get_property;
    object_class->set_property = entangle_config_set_property;

    g_object_class_install_property(object_class,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Name",
                                                        "Configuration value name",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_DESCRIPTION,
                                    g_param_spec_string("description",
                                                        "Description",
                                                        "Configuration value description",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleConfigSetPrivate));
}


EntangleConfigSet *entangle_config_set_new(const char *name,
                                   const char *description)
{
    return ENTANGLE_CONFIG_SET(g_object_new(ENTANGLE_TYPE_CONFIG_SET,
                                            "name", name,
                                            "description", description,
                                            NULL));
}


static void entangle_config_set_init(EntangleConfigSet *set)
{
    EntangleConfigSetPrivate *priv;

    priv = set->priv = ENTANGLE_CONFIG_SET_GET_PRIVATE(set);
    memset(priv, 0, sizeof(*priv));
}



const char *entangle_config_set_get_name(EntangleConfigSet *set)
{
    EntangleConfigSetPrivate *priv = ENTANGLE_CONFIG_SET_GET_PRIVATE(set);
    return priv->name;
}


const char *entangle_config_set_get_description(EntangleConfigSet *set)
{
    EntangleConfigSetPrivate *priv = ENTANGLE_CONFIG_SET_GET_PRIVATE(set);
    return priv->description;
}


void entangle_config_set_add_entry(EntangleConfigSet *set,
                               EntangleConfigEntry *entry)
{
    EntangleConfigSetPrivate *priv = ENTANGLE_CONFIG_SET_GET_PRIVATE(set);
    priv->entries = g_list_append(priv->entries, entry);
}


GList *entangle_config_set_get_entries(EntangleConfigSet *set)
{
    EntangleConfigSetPrivate *priv = ENTANGLE_CONFIG_SET_GET_PRIVATE(set);
    return g_list_copy(priv->entries);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
