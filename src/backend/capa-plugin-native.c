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
#include "capa-plugin-native.h"

#include <gmodule.h>
#include <string.h>
#include <stdio.h>

#define CAPA_PLUGIN_NATIVE_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_PLUGIN_NATIVE, CapaPluginNativePrivate))

struct _CapaPluginNativePrivate {
    GModule *module;

    gboolean (*activate)(CapaPlugin *plugin, GObject *app);
    gboolean (*deactivate)(CapaPlugin *plugin, GObject *app);
};

static void capa_plugin_interface_init(gpointer g_iface,
                                       gpointer iface_data);

G_DEFINE_TYPE_EXTENDED(CapaPluginNative, capa_plugin_native, CAPA_TYPE_PLUGIN_BASE, 0,
                       G_IMPLEMENT_INTERFACE(CAPA_TYPE_PLUGIN, capa_plugin_interface_init));

static gboolean
capa_plugin_native_activate(CapaPlugin *iface, GObject *app)
{
    CapaPluginNative *plugin = CAPA_PLUGIN_NATIVE(iface);
    CapaPluginNativePrivate *priv = plugin->priv;
    gchar *path = NULL;

    if (!priv->module) {
        gpointer sym;
        path = g_strdup_printf("%s/%s.%s",
                               capa_plugin_get_dir(CAPA_PLUGIN(plugin)),
                               capa_plugin_get_name(CAPA_PLUGIN(plugin)),
                               G_MODULE_SUFFIX);

        priv->module = g_module_open(path, G_MODULE_BIND_LOCAL);

        if (!priv->module) {
            g_free(path);
            path = g_strdup_printf("%s/capa/plugins/%s.%s",
                                   LIBDIR,
                                   capa_plugin_get_name(CAPA_PLUGIN(plugin)),
                                   G_MODULE_SUFFIX);
            priv->module = g_module_open(path, G_MODULE_BIND_LOCAL);
        }

        if (!priv->module) {
            CAPA_DEBUG("Unable to load module '%s'", path);
            goto error;
        }

        if (!g_module_symbol(priv->module, "plugin_activate", &sym) || !sym) {
            CAPA_DEBUG("Missing symbol 'plugin_activate' in '%s'", path);
            goto error;
        }
        priv->activate = sym;

        if (!g_module_symbol(priv->module, "plugin_deactivate", &sym) || !sym) {
            CAPA_DEBUG("Missing symbol 'plugin_deactivate' in '%s'", path);
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
capa_plugin_native_deactivate(CapaPlugin *iface, GObject *app)
{
    CapaPluginNative *plugin = CAPA_PLUGIN_NATIVE(iface);
    CapaPluginNativePrivate *priv = plugin->priv;
    if (!priv->module)
        return FALSE;

    return (priv->deactivate)(iface, app);
}

static gboolean
capa_plugin_native_is_active(CapaPlugin *iface)
{
    CapaPluginNative *plugin = CAPA_PLUGIN_NATIVE(iface);
    CapaPluginNativePrivate *priv = plugin->priv;

    return priv->module ? TRUE : FALSE;
}

static void capa_plugin_interface_init(gpointer g_iface,
                                       gpointer iface_data G_GNUC_UNUSED)
{
    CapaPluginInterface *iface = g_iface;
    iface->activate = capa_plugin_native_activate;
    iface->deactivate = capa_plugin_native_deactivate;
    iface->is_active = capa_plugin_native_is_active;
    iface->get_dir = capa_plugin_base_get_dir;
    iface->get_name = capa_plugin_base_get_name;
    iface->get_description = capa_plugin_base_get_description;
    iface->get_version = capa_plugin_base_get_version;
    iface->get_uri = capa_plugin_base_get_uri;
    iface->get_email = capa_plugin_base_get_email;
}

static void capa_plugin_native_finalize(GObject *object)
{
    CapaPluginNative *plugin = CAPA_PLUGIN_NATIVE(object);
    CapaPluginNativePrivate *priv = plugin->priv;

    g_module_close(priv->module);

    G_OBJECT_CLASS(capa_plugin_native_parent_class)->finalize (object);
}

static void capa_plugin_native_class_init(CapaPluginNativeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_plugin_native_finalize;

    g_type_class_add_private(klass, sizeof(CapaPluginNativePrivate));
}


void capa_plugin_native_init(CapaPluginNative *plugin)
{
    CapaPluginNativePrivate *priv;

    priv = plugin->priv = CAPA_PLUGIN_NATIVE_GET_PRIVATE(plugin);
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
