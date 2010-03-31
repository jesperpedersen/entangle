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
#include "entangle-config-entry.h"
#include "entangle-config-entry-enums.h"

#define ENTANGLE_CONFIG_ENTRY_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONFIG_ENTRY, EntangleConfigEntryPrivate))

struct _EntangleConfigEntryPrivate {
    EntangleConfigEntryDatatype datatype;
    char *name;
    char *description;
};

G_DEFINE_TYPE(EntangleConfigEntry, entangle_config_entry, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_DATATYPE,
    PROP_NAME,
    PROP_DESCRIPTION,
};


static void entangle_config_entry_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
    EntangleConfigEntry *entry = ENTANGLE_CONFIG_ENTRY(object);
    EntangleConfigEntryPrivate *priv = entry->priv;

    switch (prop_id)
        {
        case PROP_DATATYPE:
            g_value_set_enum(value, priv->datatype);
            break;

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

static void entangle_config_entry_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
    EntangleConfigEntry *entry = ENTANGLE_CONFIG_ENTRY(object);
    EntangleConfigEntryPrivate *priv = entry->priv;

    switch (prop_id)
        {
        case PROP_DATATYPE:
            priv->datatype = g_value_get_enum(value);
            break;

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


static void entangle_config_entry_finalize(GObject *object)
{
    EntangleConfigEntry *config = ENTANGLE_CONFIG_ENTRY(object);
    EntangleConfigEntryPrivate *priv = config->priv;

    ENTANGLE_DEBUG("Finalize config entry %p", object);

    g_free(priv->name);
    g_free(priv->description);

    G_OBJECT_CLASS (entangle_config_entry_parent_class)->finalize (object);
}


static void entangle_config_entry_class_init(EntangleConfigEntryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_config_entry_finalize;
    object_class->get_property = entangle_config_entry_get_property;
    object_class->set_property = entangle_config_entry_set_property;

    g_object_class_install_property(object_class,
                                    PROP_DATATYPE,
                                    g_param_spec_enum("datatype",
                                                      "Datatype",
                                                      "Configuration value datatype",
                                                      ENTANGLE_TYPE_CONFIG_ENTRY_DATATYPE,
                                                      ENTANGLE_CONFIG_ENTRY_STRING,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

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

    g_type_class_add_private(klass, sizeof(EntangleConfigEntryPrivate));
}


EntangleConfigEntry *entangle_config_entry_new(EntangleConfigEntryDatatype datatype,
                                       const char *name,
                                       const char *description)
{
    return ENTANGLE_CONFIG_ENTRY(g_object_new(ENTANGLE_TYPE_CONFIG_ENTRY,
                                          "datatype", datatype,
                                          "name", name,
                                          "description", description,
                                          NULL));
}


static void entangle_config_entry_init(EntangleConfigEntry *entry)
{
    EntangleConfigEntryPrivate *priv;

    priv = entry->priv = ENTANGLE_CONFIG_ENTRY_GET_PRIVATE(entry);
    memset(priv, 0, sizeof(*priv));

}


EntangleConfigEntryDatatype entangle_config_entry_get_datatype(EntangleConfigEntry *entry)
{
    EntangleConfigEntryPrivate *priv = ENTANGLE_CONFIG_ENTRY_GET_PRIVATE(entry);
    return priv->datatype;
}


const char *entangle_config_entry_get_name(EntangleConfigEntry *entry)
{
    EntangleConfigEntryPrivate *priv = ENTANGLE_CONFIG_ENTRY_GET_PRIVATE(entry);
    return priv->name;
}


const char *entangle_config_entry_get_description(EntangleConfigEntry *entry)
{
    EntangleConfigEntryPrivate *priv = ENTANGLE_CONFIG_ENTRY_GET_PRIVATE(entry);
    return priv->description;
}





/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */