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

#include "capa-debug.h"
#include "capa-plugin.h"
#include "capa-plugin-manager.h"

#include <string.h>
#include <dirent.h>
#include <stdio.h>

#define CAPA_PLUGIN_MANAGER_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_PLUGIN_MANAGER, CapaPluginManagerPrivate))

struct _CapaPluginManagerPrivate {
    GHashTable *pluginTypes;
    GHashTable *plugins;
};

G_DEFINE_TYPE(CapaPluginManager, capa_plugin_manager, G_TYPE_OBJECT);


static void capa_plugin_manager_finalize(GObject *object)
{
    CapaPluginManager *plugin = CAPA_PLUGIN_MANAGER(object);
    CapaPluginManagerPrivate *priv = plugin->priv;

    g_hash_table_unref(priv->pluginTypes);
    g_hash_table_unref(priv->plugins);

    G_OBJECT_CLASS(capa_plugin_manager_parent_class)->finalize (object);
}

static void capa_plugin_manager_class_init(CapaPluginManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_plugin_manager_finalize;

    g_type_class_add_private(klass, sizeof(CapaPluginManagerPrivate));
}


static gboolean capa_plugin_manager_load(CapaPluginManager *manager,
                                         const gchar *path,
                                         const gchar *name)
{
    CapaPluginManagerPrivate *priv = manager->priv;
    GKeyFile *keyfile;
    CapaPlugin *plugin;
    GType type;
    gchar *typeName;
    gchar *description;
    gchar *version;
    gchar *uri;
    gchar *email;
    gchar *dir;
    gchar *file;
    gboolean newfile = FALSE;

    CAPA_DEBUG("Load pliugin '%s' from '%s'", name, path);
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
        CAPA_DEBUG("Missing plugin config 'type' parameter");
        goto cleanup;
    }
    if (!description) {
        CAPA_DEBUG("Missing plugin config 'description' parameter");
        goto cleanup;
    }
    if (!version) {
        CAPA_DEBUG("Missing plugin config 'version' parameter");
        goto cleanup;
    }
    if (!email) {
        CAPA_DEBUG("Missing plugin config 'email' parameter");
        goto cleanup;
    }
    if (!uri) {
        CAPA_DEBUG("Missing plugin config 'uri' parameter");
        goto cleanup;
    }

    if (!g_hash_table_lookup(priv->plugins, uri)) {
        type = GPOINTER_TO_INT(g_hash_table_lookup(priv->pluginTypes, typeName));

        if (!type) {
            CAPA_DEBUG("No plugin type '%s'", typeName);
            goto cleanup;
        }

        plugin = CAPA_PLUGIN(g_object_new(type,
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

static gint capa_plugin_manager_scan_dir(CapaPluginManager *manager,
                                         const char *path)
{
    DIR *dir;
    struct dirent *ent;
    gint newfiles = 0;

    CAPA_DEBUG("Scan dir '%s'", path);
    dir = opendir(path);

    if (!dir)
        return 0;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;

        if (capa_plugin_manager_load(manager, path, ent->d_name))
            newfiles++;
    }
    closedir(dir);

    return newfiles;
}

gint capa_plugin_manager_scan(CapaPluginManager *manager)
{
    const gchar *userdir;
    gchar *userpath;
    guint newfiles = 0;

    newfiles += capa_plugin_manager_scan_dir(manager, PKGDATADIR "/plugins");

    userdir = g_get_user_data_dir();
    userpath = g_strdup_printf("%s/%s/%s", userdir, "capa", "plugins");
    newfiles += capa_plugin_manager_scan_dir(manager, userpath);
    g_free(userpath);

    return newfiles;
}

void capa_plugin_manager_init(CapaPluginManager *manager)
{
    CapaPluginManagerPrivate *priv;

    priv = manager->priv = CAPA_PLUGIN_MANAGER_GET_PRIVATE(manager);
    memset(priv, 0, sizeof(*priv));

    priv->pluginTypes = g_hash_table_new_full(g_str_hash, g_str_equal,
                                              g_free, NULL);
    priv->plugins = g_hash_table_new_full(g_str_hash, g_str_equal,
                                          g_free, g_object_unref);

}


CapaPluginManager *capa_plugin_manager_new(void)
{
    return CAPA_PLUGIN_MANAGER(g_object_new(CAPA_TYPE_PLUGIN_MANAGER,
                                            NULL));
}

void capa_plugin_manager_register_type(CapaPluginManager *manager,
                                       const gchar *name,
                                       GType type)
{
    CapaPluginManagerPrivate *priv = manager->priv;
    g_hash_table_insert(priv->pluginTypes, g_strdup(name), GINT_TO_POINTER(type));
}


void capa_plugin_manager_activate(CapaPluginManager *manager, GObject *app)
{
    CapaPluginManagerPrivate *priv = manager->priv;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, priv->plugins);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        CapaPlugin *plugin = CAPA_PLUGIN(value);

        if (!capa_plugin_is_active(plugin)) {
            CAPA_DEBUG("Activate plugin named '%s'", capa_plugin_get_name(plugin));
            capa_plugin_activate(plugin, app);
        }
    }
}


void capa_plugin_manager_deactivate(CapaPluginManager *manager, GObject *app)
{
    CapaPluginManagerPrivate *priv = manager->priv;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, priv->plugins);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        CapaPlugin *plugin = CAPA_PLUGIN(value);

        if (capa_plugin_is_active(plugin)) {
            CAPA_DEBUG("Deactivate plugin named '%s'", capa_plugin_get_name(plugin));
            capa_plugin_deactivate(plugin, app);
        }
    }
}


GList *capa_plugin_manager_get_all(CapaPluginManager *manager)
{
    CapaPluginManagerPrivate *priv = manager->priv;
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
