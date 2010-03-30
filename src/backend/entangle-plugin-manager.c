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

#include "entangle-debug.h"
#include "entangle-plugin.h"
#include "entangle-plugin-manager.h"

#include <string.h>
#include <dirent.h>
#include <stdio.h>

#define ENTANGLE_PLUGIN_MANAGER_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PLUGIN_MANAGER, EntanglePluginManagerPrivate))

struct _EntanglePluginManagerPrivate {
    GHashTable *pluginTypes;
    GHashTable *plugins;
};

G_DEFINE_TYPE(EntanglePluginManager, entangle_plugin_manager, G_TYPE_OBJECT);


static void entangle_plugin_manager_finalize(GObject *object)
{
    EntanglePluginManager *plugin = ENTANGLE_PLUGIN_MANAGER(object);
    EntanglePluginManagerPrivate *priv = plugin->priv;

    g_hash_table_unref(priv->pluginTypes);
    g_hash_table_unref(priv->plugins);

    G_OBJECT_CLASS(entangle_plugin_manager_parent_class)->finalize (object);
}

static void entangle_plugin_manager_class_init(EntanglePluginManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_plugin_manager_finalize;

    g_type_class_add_private(klass, sizeof(EntanglePluginManagerPrivate));
}


static gboolean entangle_plugin_manager_load(EntanglePluginManager *manager,
                                         const gchar *path,
                                         const gchar *name)
{
    EntanglePluginManagerPrivate *priv = manager->priv;
    GKeyFile *keyfile;
    EntanglePlugin *plugin;
    GType type;
    gchar *typeName;
    gchar *description;
    gchar *version;
    gchar *uri;
    gchar *email;
    gchar *dir;
    gchar *file;
    gboolean newfile = FALSE;

    ENTANGLE_DEBUG("Load pliugin '%s' from '%s'", name, path);
    keyfile = g_key_file_new();

    dir = g_strdup_printf("%s/%s", path, name);
    file = g_strdup_printf("%s/plugin.cfg", dir);
    g_key_file_load_from_file(keyfile, file, 0, NULL);

    typeName = g_key_file_get_string(keyfile, "plugin", "type", NULL);
    description = g_key_file_get_string(keyfile, "plugin", "description", NULL);
    version = g_key_file_get_string(keyfile, "plugin", "version", NULL);
    uri = g_key_file_get_string(keyfile, "plugin", "uri", NULL);
    email = g_key_file_get_string(keyfile, "plugin", "email", NULL);

    if (!typeName) {
        ENTANGLE_DEBUG("Missing plugin config 'type' parameter");
        goto cleanup;
    }
    if (!description) {
        ENTANGLE_DEBUG("Missing plugin config 'description' parameter");
        goto cleanup;
    }
    if (!version) {
        ENTANGLE_DEBUG("Missing plugin config 'version' parameter");
        goto cleanup;
    }
    if (!email) {
        ENTANGLE_DEBUG("Missing plugin config 'email' parameter");
        goto cleanup;
    }
    if (!uri) {
        ENTANGLE_DEBUG("Missing plugin config 'uri' parameter");
        goto cleanup;
    }

    if (!g_hash_table_lookup(priv->plugins, uri)) {
        type = GPOINTER_TO_INT(g_hash_table_lookup(priv->pluginTypes, typeName));

        if (!type) {
            ENTANGLE_DEBUG("No plugin type '%s'", typeName);
            goto cleanup;
        }

        plugin = ENTANGLE_PLUGIN(g_object_new(type,
                                          "dir", dir,
                                          "name", name,
                                          "description", description,
                                          "version", version,
                                          "email", email,
                                          "uri", uri,
                                          NULL));

        g_hash_table_insert(priv->plugins, g_strdup(uri), plugin);
        newfile = TRUE;
    }

 cleanup:
    g_free(typeName);
    g_free(description);
    g_free(version);
    g_free(email);
    g_free(uri);
    g_free(dir);
    g_free(file);
    return newfile;
}

static gint entangle_plugin_manager_scan_dir(EntanglePluginManager *manager,
                                         const char *path)
{
    DIR *dir;
    struct dirent *ent;
    gint newfiles = 0;

    ENTANGLE_DEBUG("Scan dir '%s'", path);
    dir = opendir(path);

    if (!dir)
        return 0;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;

        if (entangle_plugin_manager_load(manager, path, ent->d_name))
            newfiles++;
    }
    closedir(dir);

    return newfiles;
}

gint entangle_plugin_manager_scan(EntanglePluginManager *manager)
{
    const gchar *userdir;
    gchar *userpath;
    guint newfiles = 0;

    newfiles += entangle_plugin_manager_scan_dir(manager, PKGDATADIR "/plugins");

    userdir = g_get_user_data_dir();
    userpath = g_strdup_printf("%s/%s/%s", userdir, "entangle", "plugins");
    newfiles += entangle_plugin_manager_scan_dir(manager, userpath);
    g_free(userpath);

    return newfiles;
}

void entangle_plugin_manager_init(EntanglePluginManager *manager)
{
    EntanglePluginManagerPrivate *priv;

    priv = manager->priv = ENTANGLE_PLUGIN_MANAGER_GET_PRIVATE(manager);
    memset(priv, 0, sizeof(*priv));

    priv->pluginTypes = g_hash_table_new_full(g_str_hash, g_str_equal,
                                              g_free, NULL);
    priv->plugins = g_hash_table_new_full(g_str_hash, g_str_equal,
                                          g_free, g_object_unref);

}


EntanglePluginManager *entangle_plugin_manager_new(void)
{
    return ENTANGLE_PLUGIN_MANAGER(g_object_new(ENTANGLE_TYPE_PLUGIN_MANAGER,
                                            NULL));
}

void entangle_plugin_manager_register_type(EntanglePluginManager *manager,
                                       const gchar *name,
                                       GType type)
{
    EntanglePluginManagerPrivate *priv = manager->priv;
    g_hash_table_insert(priv->pluginTypes, g_strdup(name), GINT_TO_POINTER(type));
}


void entangle_plugin_manager_activate(EntanglePluginManager *manager, GObject *app)
{
    EntanglePluginManagerPrivate *priv = manager->priv;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, priv->plugins);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        EntanglePlugin *plugin = ENTANGLE_PLUGIN(value);

        if (!entangle_plugin_is_active(plugin)) {
            ENTANGLE_DEBUG("Activate plugin named '%s'", entangle_plugin_get_name(plugin));
            entangle_plugin_activate(plugin, app);
        }
    }
}


void entangle_plugin_manager_deactivate(EntanglePluginManager *manager, GObject *app)
{
    EntanglePluginManagerPrivate *priv = manager->priv;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, priv->plugins);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        EntanglePlugin *plugin = ENTANGLE_PLUGIN(value);

        if (entangle_plugin_is_active(plugin)) {
            ENTANGLE_DEBUG("Deactivate plugin named '%s'", entangle_plugin_get_name(plugin));
            entangle_plugin_deactivate(plugin, app);
        }
    }
}


GList *entangle_plugin_manager_get_all(EntanglePluginManager *manager)
{
    EntanglePluginManagerPrivate *priv = manager->priv;
    return g_hash_table_get_values(priv->plugins);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
