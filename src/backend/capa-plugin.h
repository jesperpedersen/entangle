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

#ifndef __CAPA_PLUGIN_H__
#define __CAPA_PLUGIN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_PLUGIN            (capa_plugin_get_type ())
#define CAPA_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PLUGIN, CapaPlugin))
#define CAPA_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_PLUGIN, CapaPluginClass))
#define CAPA_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PLUGIN))
#define CAPA_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_PLUGIN))
#define CAPA_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_PLUGIN, CapaPluginClass))

typedef struct _CapaPlugin CapaPlugin;
typedef struct _CapaPluginPrivate CapaPluginPrivate;
typedef struct _CapaPluginClass CapaPluginClass;

struct _CapaPlugin
{
    GObject parent;

    CapaPluginPrivate *priv;
};

struct _CapaPluginClass
{
    GObjectClass parent;

    gboolean (*activate)(CapaPlugin *plugin, GObject *app);
    gboolean (*deactivate)(CapaPlugin *plugin, GObject *app);
    gboolean (*is_active)(CapaPlugin *plugin);
};

GType capa_plugin_get_type(void);

gboolean capa_plugin_activate(CapaPlugin *plugin,
                              GObject *app);

gboolean capa_plugin_deactivate(CapaPlugin *plugin,
                                GObject *app);

gboolean capa_plugin_is_active(CapaPlugin *plugin);

const gchar * capa_plugin_get_dir(CapaPlugin *plugin);
const gchar * capa_plugin_get_name(CapaPlugin *plugin);
const gchar * capa_plugin_get_version(CapaPlugin *plugin);
const gchar * capa_plugin_get_description(CapaPlugin *plugin);
const gchar * capa_plugin_get_uri(CapaPlugin *plugin);
const gchar * capa_plugin_get_email(CapaPlugin *plugin);


G_END_DECLS

#endif /* __CAPA_PLUGIN_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
