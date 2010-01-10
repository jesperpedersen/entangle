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

#ifndef __CAPA_PLUGIN_BASE_H__
#define __CAPA_PLUGIN_BASE_H__

#include <glib-object.h>

#include "capa-plugin.h"

G_BEGIN_DECLS

#define CAPA_TYPE_PLUGIN_BASE            (capa_plugin_base_get_type ())
#define CAPA_PLUGIN_BASE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PLUGIN_BASE, CapaPluginBase))
#define CAPA_IS_PLUGIN_BASE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PLUGIN_BASE))
#define CAPA_PLUGIN_BASE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CAPA_TYPE_PLUGIN_BASE, CapaPluginBaseInterface))

typedef struct _CapaPluginBase CapaPluginBase;
typedef struct _CapaPluginBasePrivate CapaPluginBasePrivate;
typedef struct _CapaPluginBaseClass CapaPluginBaseClass;

struct _CapaPluginBase
{
    GObject parent;

    CapaPluginBasePrivate *priv;
};

struct _CapaPluginBaseClass
{
    GObjectClass parent_class;
};

GType capa_plugin_base_get_type(void);

const gchar *capa_plugin_base_get_dir(CapaPlugin *iface);
const gchar *capa_plugin_base_get_name(CapaPlugin *iface);
const gchar *capa_plugin_base_get_description(CapaPlugin *iface);
const gchar *capa_plugin_base_get_version(CapaPlugin *iface);
const gchar *capa_plugin_base_get_uri(CapaPlugin *iface);
const gchar *capa_plugin_base_get_email(CapaPlugin *iface);

G_END_DECLS

#endif /* __CAPA_PLUGIN_BASE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
