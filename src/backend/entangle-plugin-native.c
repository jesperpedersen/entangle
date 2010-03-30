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
#include "entangle-plugin-native.h"

#include <gmodule.h>
#include <string.h>
#include <stdio.h>

#define ENTANGLE_PLUGIN_NATIVE_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PLUGIN_NATIVE, EntanglePluginNativePrivate))

struct _EntanglePluginNativePrivate {
    GModule *module;

    gboolean (*activate)(EntanglePlugin *plugin, GObject *app);
    gboolean (*deactivate)(EntanglePlugin *plugin, GObject *app);
};


G_DEFINE_TYPE(EntanglePluginNative, entangle_plugin_native, ENTANGLE_TYPE_PLUGIN);

static gboolean
entangle_plugin_native_activate(EntanglePlugin *iface, GObject *app)
{
    EntanglePluginNative *plugin = ENTANGLE_PLUGIN_NATIVE(iface);
    EntanglePluginNativePrivate *priv = plugin->priv;
    gchar *path = NULL;

    if (!priv->module) {
        gpointer sym;
        path = g_strdup_printf("%s/%s.%s",
                               entangle_plugin_get_dir(ENTANGLE_PLUGIN(plugin)),
                               entangle_plugin_get_name(ENTANGLE_PLUGIN(plugin)),
                               G_MODULE_SUFFIX);

        priv->module = g_module_open(path, G_MODULE_BIND_LOCAL);

        if (!priv->module) {
            g_free(path);
            path = g_strdup_printf("%s/entangle/plugins/%s.%s",
                                   LIBDIR,
                                   entangle_plugin_get_name(ENTANGLE_PLUGIN(plugin)),
                                   G_MODULE_SUFFIX);
            priv->module = g_module_open(path, G_MODULE_BIND_LOCAL);
        }

        if (!priv->module) {
            ENTANGLE_DEBUG("Unable to load module '%s'", path);
            goto error;
        }

        if (!g_module_symbol(priv->module, "plugin_activate", &sym) || !sym) {
            ENTANGLE_DEBUG("Missing symbol 'plugin_activate' in '%s'", path);
            goto error;
        }
        priv->activate = sym;

        if (!g_module_symbol(priv->module, "plugin_deactivate", &sym) || !sym) {
            ENTANGLE_DEBUG("Missing symbol 'plugin_deactivate' in '%s'", path);
            goto error;
        }
        priv->deactivate = sym;

        g_module_make_resident(priv->module);
    }

    g_free(path);
    return (priv->activate)(iface, app);

 error:
    if (priv->module)
        g_module_close(priv->module);
    priv->module = NULL;
    priv->activate = NULL;
    priv->deactivate = NULL;
    g_free(path);
    return FALSE;
}

static gboolean
entangle_plugin_native_deactivate(EntanglePlugin *iface, GObject *app)
{
    EntanglePluginNative *plugin = ENTANGLE_PLUGIN_NATIVE(iface);
    EntanglePluginNativePrivate *priv = plugin->priv;
    if (!priv->module)
        return FALSE;

    return (priv->deactivate)(iface, app);
}

static gboolean
entangle_plugin_native_is_active(EntanglePlugin *iface)
{
    EntanglePluginNative *plugin = ENTANGLE_PLUGIN_NATIVE(iface);
    EntanglePluginNativePrivate *priv = plugin->priv;

    return priv->module ? TRUE : FALSE;
}


static void entangle_plugin_native_finalize(GObject *object)
{
    EntanglePluginNative *plugin = ENTANGLE_PLUGIN_NATIVE(object);
    EntanglePluginNativePrivate *priv = plugin->priv;

    g_module_close(priv->module);

    G_OBJECT_CLASS(entangle_plugin_native_parent_class)->finalize (object);
}

static void entangle_plugin_native_class_init(EntanglePluginNativeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    EntanglePluginClass *plugin_class = ENTANGLE_PLUGIN_CLASS (klass);

    object_class->finalize = entangle_plugin_native_finalize;

    plugin_class->activate = entangle_plugin_native_activate;
    plugin_class->deactivate = entangle_plugin_native_deactivate;
    plugin_class->is_active = entangle_plugin_native_is_active;

    g_type_class_add_private(klass, sizeof(EntanglePluginNativePrivate));
}


void entangle_plugin_native_init(EntanglePluginNative *plugin)
{
    EntanglePluginNativePrivate *priv;

    priv = plugin->priv = ENTANGLE_PLUGIN_NATIVE_GET_PRIVATE(plugin);
    memset(priv, 0, sizeof(*priv));
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
