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


gboolean capa_plugin_activate(CapaPlugin *plugin, GObject *app)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->activate(plugin, app);
}

gboolean capa_plugin_deactivate(CapaPlugin *plugin, GObject *app)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->deactivate(plugin, app);
}

gboolean capa_plugin_is_active(CapaPlugin *plugin)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->is_active(plugin);
}

const gchar * capa_plugin_get_dir(CapaPlugin *plugin)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->get_dir(plugin);
}

const gchar * capa_plugin_get_name(CapaPlugin *plugin)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->get_name(plugin);
}

const gchar * capa_plugin_get_description(CapaPlugin *plugin)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->get_description(plugin);
}

const gchar * capa_plugin_get_version(CapaPlugin *plugin)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->get_version(plugin);
}

const gchar * capa_plugin_get_uri(CapaPlugin *plugin)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->get_uri(plugin);
}

const gchar * capa_plugin_get_email(CapaPlugin *plugin)
{
    return CAPA_PLUGIN_GET_INTERFACE(plugin)->get_email(plugin);
}

GType
capa_plugin_get_type (void)
{
    static GType plugin_type = 0;

    if (!plugin_type) {
        plugin_type =
            g_type_register_static_simple(G_TYPE_INTERFACE, "CapaPlugin",
                                          sizeof (CapaPluginInterface),
                                          NULL, 0, NULL, 0);

        g_type_interface_add_prerequisite(plugin_type, G_TYPE_OBJECT);
    }

    return plugin_type;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
