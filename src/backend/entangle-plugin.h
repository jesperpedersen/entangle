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

#ifndef __ENTANGLE_PLUGIN_H__
#define __ENTANGLE_PLUGIN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_PLUGIN            (entangle_plugin_get_type ())
#define ENTANGLE_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PLUGIN, EntanglePlugin))
#define ENTANGLE_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_PLUGIN, EntanglePluginClass))
#define ENTANGLE_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PLUGIN))
#define ENTANGLE_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_PLUGIN))
#define ENTANGLE_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_PLUGIN, EntanglePluginClass))

typedef struct _EntanglePlugin EntanglePlugin;
typedef struct _EntanglePluginPrivate EntanglePluginPrivate;
typedef struct _EntanglePluginClass EntanglePluginClass;

struct _EntanglePlugin
{
    GObject parent;

    EntanglePluginPrivate *priv;
};

struct _EntanglePluginClass
{
    GObjectClass parent;

    gboolean (*activate)(EntanglePlugin *plugin, GObject *app);
    gboolean (*deactivate)(EntanglePlugin *plugin, GObject *app);
    gboolean (*is_active)(EntanglePlugin *plugin);
};

GType entangle_plugin_get_type(void);

gboolean entangle_plugin_activate(EntanglePlugin *plugin,
                              GObject *app);

gboolean entangle_plugin_deactivate(EntanglePlugin *plugin,
                                GObject *app);

gboolean entangle_plugin_is_active(EntanglePlugin *plugin);

const gchar * entangle_plugin_get_dir(EntanglePlugin *plugin);
const gchar * entangle_plugin_get_name(EntanglePlugin *plugin);
const gchar * entangle_plugin_get_version(EntanglePlugin *plugin);
const gchar * entangle_plugin_get_description(EntanglePlugin *plugin);
const gchar * entangle_plugin_get_uri(EntanglePlugin *plugin);
const gchar * entangle_plugin_get_email(EntanglePlugin *plugin);


G_END_DECLS

#endif /* __ENTANGLE_PLUGIN_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
