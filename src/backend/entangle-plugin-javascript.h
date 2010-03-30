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

#ifndef __ENTANGLE_PLUGIN_JAVASCRIPT_H__
#define __ENTANGLE_PLUGIN_JAVASCRIPT_H__

#include <glib-object.h>

#include "entangle-plugin.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_PLUGIN_JAVASCRIPT            (entangle_plugin_javascript_get_type ())
#define ENTANGLE_PLUGIN_JAVASCRIPT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PLUGIN_JAVASCRIPT, EntanglePluginJavascript))
#define ENTANGLE_PLUGIN_JAVASCRIPT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_PLUGIN_JAVASCRIPT, EntanglePluginJavaScriptClass))
#define ENTANGLE_IS_PLUGIN_JAVASCRIPT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PLUGIN_JAVASCRIPT))
#define ENTANGLE_IS_PLUGIN_JAVASCRIPT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_PLUGIN_JAVASCRIPT))
#define ENTANGLE_PLUGIN_JAVASCRIPT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_PLUGIN_JAVASCRIPT, EntanglePluginJavascriptClass))


typedef struct _EntanglePluginJavascript EntanglePluginJavascript;
typedef struct _EntanglePluginJavascriptPrivate EntanglePluginJavascriptPrivate;
typedef struct _EntanglePluginJavascriptClass EntanglePluginJavascriptClass;

struct _EntanglePluginJavascript
{
    EntanglePlugin parent;

    EntanglePluginJavascriptPrivate *priv;
};

struct _EntanglePluginJavascriptClass
{
    EntanglePluginClass parent_class;
};

GType entangle_plugin_javascript_get_type(void);

G_END_DECLS

#endif /* __ENTANGLE_PLUGIN_JAVASCRIPT_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
