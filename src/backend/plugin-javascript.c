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

#include "internal.h"
#include "plugin-javascript.h"

#include <string.h>
#include <gjs/gjs.h>
#include <gjs/gi/object.h>

#define CAPA_PLUGIN_JAVASCRIPT_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_PLUGIN_JAVASCRIPT, CapaPluginJavascriptPrivate))

struct _CapaPluginJavascriptPrivate {
    GjsContext *context;
    gboolean active;
};

static void capa_plugin_interface_init(gpointer g_iface,
                                       gpointer iface_data);

G_DEFINE_TYPE_EXTENDED(CapaPluginJavascript, capa_plugin_javascript, CAPA_TYPE_PLUGIN_BASE, 0,
                       G_IMPLEMENT_INTERFACE(CAPA_TYPE_PLUGIN, capa_plugin_interface_init));

/* XXX
 * Evil hack. THis method is defined in the .so, but not in the
 * header files. This is the only apparant way to get the
 * JSContext that we need in order to define new global vars
 */
JSContext* gjs_context_get_context(GjsContext *js_context);

static gboolean
capa_plugin_javascript_eval(CapaPluginJavascript *plugin,
                            GObject *app,
                            const gchar *script)
{
    CapaPluginJavascriptPrivate *priv = plugin->priv;
    gchar *file;
    int status;
    const gchar *searchpath[2];
    JSContext *jscontext;
    JSObject *jsapp;
    JSObject *jsglobal;

    file = g_strdup_printf("%s/main.js", capa_plugin_get_dir(CAPA_PLUGIN(plugin)));

    searchpath[0] = capa_plugin_get_dir(CAPA_PLUGIN(plugin));
    searchpath[1] = NULL;

    if (!priv->context) {
        priv->context = gjs_context_new_with_search_path((gchar **)searchpath);
        /* We need to do this eval now so that the later
         *    'gjs_object_from_g_object'
         * call can find the introspection data it requires */
        gjs_context_eval(priv->context,
                         "const Capa = imports.gi.Capa",
                         -1,
                         file,
                         &status,
                         NULL);
    }


    /* XXX
     * Evil hack. GJS ought to provide a official public API
     * for defining a global variable from a GObject.
     */
    jscontext = gjs_context_get_context(priv->context);
    jsglobal = JS_GetGlobalObject(jscontext);
    JS_EnterLocalRootScope(jscontext);
    jsapp = gjs_object_from_g_object(jscontext, G_OBJECT(app));
    JS_DefineProperty(jscontext, jsglobal,
                      "app", OBJECT_TO_JSVAL(jsapp),
                      NULL, NULL,
                      JSPROP_READONLY | JSPROP_PERMANENT);
    JS_LeaveLocalRootScope(jscontext);

    CAPA_DEBUG("Eval javascript '%s' in '%s'\n", script, file);
    gjs_context_eval(priv->context,
                     script,
                     -1,
                     file,
                     &status,
                     NULL);

    g_free(file);
    return status == 0 ? TRUE : FALSE;
}

static gboolean
capa_plugin_javascript_activate(CapaPlugin *iface, GObject *app)
{
    CapaPluginJavascript *plugin = CAPA_PLUGIN_JAVASCRIPT(iface);
    CapaPluginJavascriptPrivate *priv = plugin->priv;

    if (priv->active)
        return TRUE;

    priv->active = TRUE;

    return capa_plugin_javascript_eval(plugin, app,
                                       "const Main = imports.main; Main.activate(app);");
}

static gboolean
capa_plugin_javascript_deactivate(CapaPlugin *iface, GObject *app)
{
    CapaPluginJavascript *plugin = CAPA_PLUGIN_JAVASCRIPT(iface);
    CapaPluginJavascriptPrivate *priv = plugin->priv;

    if (!priv->active)
        return TRUE;

    priv->active = FALSE;

    return capa_plugin_javascript_eval(plugin, app,
                                       "const Main = imports.main; Main.deactivate(app);");
}

static gboolean
capa_plugin_javascript_is_active(CapaPlugin *iface)
{
    CapaPluginJavascript *plugin = CAPA_PLUGIN_JAVASCRIPT(iface);
    CapaPluginJavascriptPrivate *priv = plugin->priv;

    return priv->active;
}


static void capa_plugin_interface_init(gpointer g_iface,
                                       gpointer iface_data G_GNUC_UNUSED)
{
    CapaPluginInterface *iface = g_iface;
    iface->activate = capa_plugin_javascript_activate;
    iface->deactivate = capa_plugin_javascript_deactivate;
    iface->is_active = capa_plugin_javascript_is_active;
    iface->get_dir = capa_plugin_base_get_dir;
    iface->get_name = capa_plugin_base_get_name;
    iface->get_description = capa_plugin_base_get_description;
    iface->get_version = capa_plugin_base_get_version;
    iface->get_uri = capa_plugin_base_get_uri;
    iface->get_email = capa_plugin_base_get_email;
}

static void capa_plugin_javascript_finalize(GObject *object)
{
    CapaPluginJavascript *plugin = CAPA_PLUGIN_JAVASCRIPT(object);
    CapaPluginJavascriptPrivate *priv = plugin->priv;

    g_object_unref(priv->context);

    G_OBJECT_CLASS(capa_plugin_javascript_parent_class)->finalize (object);
}

static void capa_plugin_javascript_class_init(CapaPluginJavascriptClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_plugin_javascript_finalize;

    g_type_class_add_private(klass, sizeof(CapaPluginJavascriptPrivate));
}


void capa_plugin_javascript_init(CapaPluginJavascript *plugin)
{
    CapaPluginJavascriptPrivate *priv;

    priv = plugin->priv = CAPA_PLUGIN_JAVASCRIPT_GET_PRIVATE(plugin);
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