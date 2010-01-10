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

#ifndef __CAPA_PLUGIN_NATIVE_H__
#define __CAPA_PLUGIN_NATIVE_H__

#include <glib-object.h>

#include "capa-plugin.h"

G_BEGIN_DECLS

#define CAPA_TYPE_PLUGIN_NATIVE            (capa_plugin_native_get_type ())
#define CAPA_PLUGIN_NATIVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PLUGIN_NATIVE, CapaPluginNative))
#define CAPA_PLUGIN_NATIVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_PLUGIN_NATIVE, CapaPluginNativeClass))
#define CAPA_IS_PLUGIN_NATIVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PLUGIN_NATIVE))
#define CAPA_IS_PLUGIN_NATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_PLUGIN_NATIVE))
#define CAPA_PLUGIN_NATIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_PLUGIN_NATIVE, CapaPluginNativeClass))

typedef struct _CapaPluginNative CapaPluginNative;
typedef struct _CapaPluginNativePrivate CapaPluginNativePrivate;
typedef struct _CapaPluginNativeClass CapaPluginNativeClass;

struct _CapaPluginNative
{
    CapaPlugin parent;

    CapaPluginNativePrivate *priv;
};

struct _CapaPluginNativeClass
{
    CapaPluginClass parent_class;
};

GType capa_plugin_native_get_type(void);

G_END_DECLS

#endif /* __CAPA_PLUGIN_NATIVE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
