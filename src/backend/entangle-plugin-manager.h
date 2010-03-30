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

#ifndef __ENTANGLE_PLUGIN_MANAGER_H__
#define __ENTANGLE_PLUGIN_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_PLUGIN_MANAGER            (entangle_plugin_manager_get_type ())
#define ENTANGLE_PLUGIN_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PLUGIN_MANAGER, EntanglePluginManager))
#define ENTANGLE_IS_PLUGIN_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PLUGIN_MANAGER))
#define ENTANGLE_PLUGIN_MANAGER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), ENTANGLE_TYPE_PLUGIN_MANAGER, EntanglePluginManagerInterface))

typedef struct _EntanglePluginManager EntanglePluginManager;
typedef struct _EntanglePluginManagerPrivate EntanglePluginManagerPrivate;
typedef struct _EntanglePluginManagerClass EntanglePluginManagerClass;

struct _EntanglePluginManager
{
    GObject parent;

    EntanglePluginManagerPrivate *priv;
};

struct _EntanglePluginManagerClass
{
    GObjectClass parent_class;
};

GType entangle_plugin_manager_get_type(void);

EntanglePluginManager *entangle_plugin_manager_new(void);

gint entangle_plugin_manager_scan(EntanglePluginManager *manager);

void entangle_plugin_manager_register_type(EntanglePluginManager *manager,
                                       const gchar *name,
                                       GType type);

void entangle_plugin_manager_activate(EntanglePluginManager *manager, GObject *app);
void entangle_plugin_manager_deactivate(EntanglePluginManager *manager, GObject *app);

GList *entangle_plugin_manager_get_all(EntanglePluginManager *manager);

G_END_DECLS

#endif /* __ENTANGLE_PLUGIN_MANAGER_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
