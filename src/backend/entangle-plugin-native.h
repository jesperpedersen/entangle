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

#ifndef __ENTANGLE_PLUGIN_NATIVE_H__
#define __ENTANGLE_PLUGIN_NATIVE_H__

#include <glib-object.h>

#include "entangle-plugin.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_PLUGIN_NATIVE            (entangle_plugin_native_get_type ())
#define ENTANGLE_PLUGIN_NATIVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PLUGIN_NATIVE, EntanglePluginNative))
#define ENTANGLE_PLUGIN_NATIVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_PLUGIN_NATIVE, EntanglePluginNativeClass))
#define ENTANGLE_IS_PLUGIN_NATIVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PLUGIN_NATIVE))
#define ENTANGLE_IS_PLUGIN_NATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_PLUGIN_NATIVE))
#define ENTANGLE_PLUGIN_NATIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_PLUGIN_NATIVE, EntanglePluginNativeClass))

typedef struct _EntanglePluginNative EntanglePluginNative;
typedef struct _EntanglePluginNativePrivate EntanglePluginNativePrivate;
typedef struct _EntanglePluginNativeClass EntanglePluginNativeClass;

struct _EntanglePluginNative
{
    EntanglePlugin parent;

    EntanglePluginNativePrivate *priv;
};

struct _EntanglePluginNativeClass
{
    EntanglePluginClass parent_class;
};

GType entangle_plugin_native_get_type(void);

G_END_DECLS

#endif /* __ENTANGLE_PLUGIN_NATIVE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
