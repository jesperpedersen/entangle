/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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

#include <glib.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>

#include "entangle-debug.h"
#include "entangle-context.h"
#include "entangle-preferences.h"

/**
 * @Short_description: Global application state base class
 * @Title: EntangleContext
 *
 * <para>
 * EntangleContext maintains some global application state. At this time,
 * the list of currently attached cameras, the application preferences
 * and the plugin manager.
 * </para>
 *
 * <para>
 * This class will normally be sub-classed when creating a Entangle based
 * application, typically in order to add in UI state.
 * </para>
 */


#define ENTANGLE_CONTEXT_GET_PRIVATE(obj)                                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTEXT, EntangleContextPrivate))

struct _EntangleContextPrivate {
    GApplication *app;

    EntangleCameraList *cameras;

    EntanglePreferences *preferences;

    PeasEngine *pluginEngine;
    PeasExtensionSet *pluginExt;
};

G_DEFINE_TYPE(EntangleContext, entangle_context, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_APPLICATION,
    PROP_CAMERAS,
    PROP_PREFERENCES,
};

static void entangle_context_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
    EntangleContext *context = ENTANGLE_CONTEXT(object);
    EntangleContextPrivate *priv = context->priv;

    switch (prop_id)
        {
        case PROP_APPLICATION:
            g_value_set_object(value, priv->app);
            break;

        case PROP_CAMERAS:
            g_value_set_object(value, priv->cameras);
            break;

        case PROP_PREFERENCES:
            g_value_set_object(value, priv->preferences);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_context_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    EntangleContext *context = ENTANGLE_CONTEXT(object);
    EntangleContextPrivate *priv = context->priv;

    switch (prop_id)
        {
        case PROP_APPLICATION:
            if (priv->app)
                g_object_unref(priv->app);
            priv->app = g_value_get_object(value);
            g_object_ref(priv->app);
            break;

        case PROP_CAMERAS:
            if (priv->cameras)
                g_object_unref(priv->cameras);
            priv->cameras = g_value_get_object(value);
            g_object_ref(priv->cameras);
            break;

        case PROP_PREFERENCES:
            if (priv->preferences)
                g_object_unref(priv->preferences);
            priv->preferences = g_value_get_object(value);
            g_object_ref(priv->preferences);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_context_finalize(GObject *object)
{
    EntangleContext *context = ENTANGLE_CONTEXT(object);
    EntangleContextPrivate *priv = context->priv;

    ENTANGLE_DEBUG("Finalize context %p", object);

    if (priv->app)
        g_object_unref(priv->app);
    if (priv->cameras)
        g_object_unref(priv->cameras);
    if (priv->preferences)
        g_object_unref(priv->preferences);
    if (priv->pluginEngine)
        g_object_unref(priv->pluginEngine);
    if (priv->pluginExt)
        g_object_unref(priv->pluginExt);

    G_OBJECT_CLASS (entangle_context_parent_class)->finalize (object);
}


static void entangle_context_class_init(EntangleContextClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_context_finalize;
    object_class->get_property = entangle_context_get_property;
    object_class->set_property = entangle_context_set_property;

    g_object_class_install_property(object_class,
                                    PROP_APPLICATION,
                                    g_param_spec_object("context",
                                                        "Application",
                                                        "Application",
                                                        G_TYPE_APPLICATION,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CAMERAS,
                                    g_param_spec_object("cameras",
                                                        "Camera list",
                                                        "List of detected cameras",
                                                        ENTANGLE_TYPE_CAMERA_LIST,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PREFERENCES,
                                    g_param_spec_object("preferences",
                                                        "Preferences",
                                                        "Application preferences",
                                                        ENTANGLE_TYPE_PREFERENCES,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleContextPrivate));
}


EntangleContext *entangle_context_new(GApplication *context)
{
    return ENTANGLE_CONTEXT(g_object_new(ENTANGLE_TYPE_CONTEXT,
                                     "context", context,
                                     NULL));
}

static void
on_extension_added(PeasExtensionSet *set G_GNUC_UNUSED,
                   PeasPluginInfo *info G_GNUC_UNUSED,
                   PeasExtension *exten,
                   gpointer opaque)
{
    peas_extension_call(exten, "activate", opaque);
}

static void
on_extension_removed(PeasExtensionSet *set G_GNUC_UNUSED,
                     PeasPluginInfo *info G_GNUC_UNUSED,
                     PeasExtension *exten,
                     gpointer opaque G_GNUC_UNUSED)
{
    peas_extension_call(exten, "deactivate");
}

static void entangle_context_init(EntangleContext *context)
{
    EntangleContextPrivate *priv;
    gchar **peasPath;
    int i;

    priv = context->priv = ENTANGLE_CONTEXT_GET_PRIVATE(context);

    priv->preferences = entangle_preferences_new();
    priv->cameras = entangle_camera_list_new();


    peasPath = g_new0(gchar *, 5);
    peasPath[0] = g_build_filename(g_get_user_config_dir (), "entangle/plugins", NULL);
    peasPath[1] = g_build_filename(g_get_user_config_dir (), "entangle/plugins", NULL);
    peasPath[2] = g_strdup(LIBDIR "/entangle/plugins");
    peasPath[3] = g_strdup(DATADIR "/entangle/plugins");
    peasPath[4] = NULL;

    g_mkdir_with_parents(peasPath[0], 0777);

    priv->pluginEngine = peas_engine_get_default();

    priv->pluginExt = peas_extension_set_new(priv->pluginEngine,
                                             PEAS_TYPE_ACTIVATABLE,
                                             "object", context,
                                             NULL);

    peas_extension_set_call(priv->pluginExt, "activate");

    g_signal_connect(priv->pluginExt, "extension-added",
                     G_CALLBACK(on_extension_added), context);
    g_signal_connect(priv->pluginExt, "extension-removed",
                     G_CALLBACK(on_extension_removed), context);

    g_free(peasPath[0]);
    g_free(peasPath[1]);
    g_free(peasPath[2]);
    g_free(peasPath[3]);
    g_free(peasPath[4]);
    g_free(peasPath);

    const GList *plugins = peas_engine_get_plugin_list(priv->pluginEngine);
    for (i = 0 ; i < g_list_length((GList *)plugins) ; i++) {
        PeasPluginInfo *plugin = g_list_nth_data((GList*)plugins, i);
        peas_engine_load_plugin(priv->pluginEngine, plugin);
    }
}


/*
 * entangle_context_refresh_cameras: Refresh the camera list
 *
 * Rescans all attached devices to look for new cameras.
 * This method is unneccessary if only interested in USB
 * cameras, since those are automatically detected via the
 * device manager when plugged in.
 */
void entangle_context_refresh_cameras(EntangleContext *context)
{
    EntangleContextPrivate *priv = context->priv;
    entangle_camera_list_refresh(priv->cameras, NULL);
}


GApplication *entangle_context_get_application(EntangleContext *context)
{
    EntangleContextPrivate *priv = context->priv;
    return priv->app;
}

/**
 * entangle_context_get_cameras: Retrieve the camera list
 *
 * Returns: (transfer none): the camera list
 */
EntangleCameraList *entangle_context_get_cameras(EntangleContext *context)
{
    EntangleContextPrivate *priv = context->priv;
    return priv->cameras;
}


/**
 * entangle_context_get_preferences: Retrieve the application preferences object
 *
 * Returns: (transfer none): the application preferences
 */
EntanglePreferences *entangle_context_get_preferences(EntangleContext *context)
{
    EntangleContextPrivate *priv = context->priv;
    return priv->preferences;
}

/**
 * entangle_context_get_plugin_engine: Retrieve the plugin manager
 *
 * Returns: (transfer none): the plugin engine
 */
PeasEngine *entangle_context_get_plugin_engine(EntangleContext *context)
{
    EntangleContextPrivate *priv = context->priv;
    return priv->pluginEngine;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
