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

#ifndef __CAPA_PLUGIN_MANAGER_H__
#define __CAPA_PLUGIN_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_PLUGIN_MANAGER            (capa_plugin_manager_get_type ())
#define CAPA_PLUGIN_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PLUGIN_MANAGER, CapaPluginManager))
#define CAPA_IS_PLUGIN_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PLUGIN_MANAGER))
#define CAPA_PLUGIN_MANAGER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CAPA_TYPE_PLUGIN_MANAGER, CapaPluginManagerInterface))

typedef struct _CapaPluginManager CapaPluginManager;
typedef struct _CapaPluginManagerPrivate CapaPluginManagerPrivate;
typedef struct _CapaPluginManagerClass CapaPluginManagerClass;

struct _CapaPluginManager
{
    GObject parent;

    CapaPluginManagerPrivate *priv;
};

struct _CapaPluginManagerClass
{
    GObjectClass parent_class;
};

GType capa_plugin_manager_get_type(void);

CapaPluginManager *capa_plugin_manager_new(void);

gint capa_plugin_manager_scan(CapaPluginManager *manager);

void capa_plugin_manager_register_type(CapaPluginManager *manager,
                                       const gchar *name,
                                       GType type);

void capa_plugin_manager_activate(CapaPluginManager *manager, GObject *app);
void capa_plugin_manager_deactivate(CapaPluginManager *manager, GObject *app);

G_END_DECLS

#endif /* __CAPA_PLUGIN_MANAGER_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
