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
#include "entangle-application.h"
#include "entangle-preferences.h"
#include "entangle-camera-manager.h"

/**
 * SECTION:entangle-application
 * @Short_description: Global application state base class
 * @Title: EntangleApplication
 *
 * <para>
 * EntangleApplication maintains some global application state. At this time,
 * the list of currently attached cameras, the application preferences
 * and the plugin manager.
 * </para>
 *
 * <para>
 * This class will normally be sub-classed when creating a Entangle based
 * application, typically in order to add in UI state.
 * </para>
 */


#define ENTANGLE_APPLICATION_GET_PRIVATE(obj)                           \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_APPLICATION, EntangleApplicationPrivate))

struct _EntangleApplicationPrivate {
    EntangleCameraList *activeCameras;
    EntangleCameraList *supportedCameras;

    EntanglePreferences *preferences;

    PeasEngine *pluginEngine;
    PeasExtensionSet *pluginExt;
};

G_DEFINE_TYPE(EntangleApplication, entangle_application, GTK_TYPE_APPLICATION);

enum {
    PROP_0,
    PROP_ACTIVE_CAMERAS,
    PROP_SUPPORTED_CAMERAS,
    PROP_PREFERENCES,
};

static void entangle_application_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    EntangleApplication *app = ENTANGLE_APPLICATION(object);
    EntangleApplicationPrivate *priv = app->priv;

    switch (prop_id) {
    case PROP_ACTIVE_CAMERAS:
        g_value_set_object(value, priv->activeCameras);
        break;

    case PROP_SUPPORTED_CAMERAS:
        g_value_set_object(value, priv->supportedCameras);
        break;

    case PROP_PREFERENCES:
        g_value_set_object(value, priv->preferences);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void entangle_application_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    EntangleApplication *app = ENTANGLE_APPLICATION(object);
    EntangleApplicationPrivate *priv = app->priv;

    switch (prop_id) {
    case PROP_ACTIVE_CAMERAS:
        if (priv->activeCameras)
            g_object_unref(priv->activeCameras);
        priv->activeCameras = g_value_get_object(value);
        g_object_ref(priv->activeCameras);
        break;

    case PROP_SUPPORTED_CAMERAS:
        if (priv->supportedCameras)
            g_object_unref(priv->supportedCameras);
        priv->supportedCameras = g_value_get_object(value);
        g_object_ref(priv->supportedCameras);
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


static void entangle_application_finalize(GObject *object)
{
    EntangleApplication *app = ENTANGLE_APPLICATION(object);
    EntangleApplicationPrivate *priv = app->priv;

    ENTANGLE_DEBUG("Finalize application %p", object);

    if (priv->activeCameras)
        g_object_unref(priv->activeCameras);
    if (priv->supportedCameras)
        g_object_unref(priv->supportedCameras);
    if (priv->preferences)
        g_object_unref(priv->preferences);
    if (priv->pluginEngine)
        g_object_unref(priv->pluginEngine);
    if (priv->pluginExt)
        g_object_unref(priv->pluginExt);

    G_OBJECT_CLASS(entangle_application_parent_class)->finalize(object);
}


static void entangle_application_activate(GApplication *gapp)
{
    GList *wins = gtk_application_get_windows(GTK_APPLICATION(gapp));
    while (wins) {
        GtkWindow *win = wins->data;
        gtk_widget_show(GTK_WIDGET(win));
        gtk_window_present(win);
        wins = wins->next;
    }
}

static void entangle_application_startup(GApplication *gapp)
{
    g_return_if_fail(ENTANGLE_IS_APPLICATION(gapp));

    EntangleApplication *app = ENTANGLE_APPLICATION(gapp);
    EntangleApplicationPrivate *priv = app->priv;
    GList *cameras = NULL, *tmp = NULL;
    gboolean gotcamera = FALSE;

    (*G_APPLICATION_CLASS(entangle_application_parent_class)->startup)(gapp);

    if (entangle_preferences_interface_get_auto_connect(priv->preferences))
        cameras = tmp = entangle_camera_list_get_cameras(priv->activeCameras);

    while (tmp) {
        EntangleCamera *cam = ENTANGLE_CAMERA(tmp->data);

        ENTANGLE_DEBUG("Opening window for %s",
                       entangle_camera_get_port(cam));

        if (entangle_camera_get_has_capture(cam)) {
            EntangleCameraManager *manager = entangle_camera_manager_new();
            gtk_application_add_window(GTK_APPLICATION(gapp), GTK_WINDOW(manager));
            gtk_widget_show(GTK_WIDGET(manager));
            entangle_camera_manager_set_camera(manager, cam);
        }

        tmp = tmp->next;
    }
    g_list_free(cameras);

    if (!gotcamera) {
        EntangleCameraManager *manager = entangle_camera_manager_new();
        gtk_application_add_window(GTK_APPLICATION(gapp), GTK_WINDOW(manager));
        gtk_widget_show(GTK_WIDGET(manager));
    }
}


static void entangle_application_class_init(EntangleApplicationClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GApplicationClass *app_class = G_APPLICATION_CLASS(klass);

    object_class->finalize = entangle_application_finalize;
    object_class->get_property = entangle_application_get_property;
    object_class->set_property = entangle_application_set_property;

    app_class->activate = entangle_application_activate;
    app_class->startup = entangle_application_startup;

    g_object_class_install_property(object_class,
                                    PROP_ACTIVE_CAMERAS,
                                    g_param_spec_object("active-cameras",
                                                        "Active cameras",
                                                        "List of active cameras",
                                                        ENTANGLE_TYPE_CAMERA_LIST,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_SUPPORTED_CAMERAS,
                                    g_param_spec_object("supported-cameras",
                                                        "Supported cameras",
                                                        "List of supported cameras",
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

    g_type_class_add_private(klass, sizeof(EntangleApplicationPrivate));
}


EntangleApplication *entangle_application_new(void)
{
    return ENTANGLE_APPLICATION(g_object_new(ENTANGLE_TYPE_APPLICATION,
                                             "application-id", "org.entangle_photo.Manager",
                                             NULL));
}


static void
on_extension_added(PeasExtensionSet *set G_GNUC_UNUSED,
                   PeasPluginInfo *info G_GNUC_UNUSED,
                   PeasExtension *exten,
                   gpointer opaque G_GNUC_UNUSED)
{
    peas_activatable_activate(PEAS_ACTIVATABLE(exten));
}


static void
on_extension_removed(PeasExtensionSet *set G_GNUC_UNUSED,
                     PeasPluginInfo *info G_GNUC_UNUSED,
                     PeasExtension *exten,
                     gpointer opaque G_GNUC_UNUSED)
{
    peas_activatable_deactivate(PEAS_ACTIVATABLE(exten));
}


static void
on_plugin_load(PeasEngine *engine G_GNUC_UNUSED,
               PeasPluginInfo *info,
               gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_APPLICATION(data));

    EntangleApplication *app = data;
    EntangleApplicationPrivate *priv = app->priv;

    entangle_preferences_interface_add_plugin(priv->preferences,
                                              peas_plugin_info_get_module_name(info));
}


static void
on_plugin_unload(PeasEngine *engine G_GNUC_UNUSED,
                 PeasPluginInfo *info,
                 gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_APPLICATION(data));

    EntangleApplication *app = data;
    EntangleApplicationPrivate *priv = app->priv;

    entangle_preferences_interface_remove_plugin(priv->preferences,
                                                 peas_plugin_info_get_module_name(info));
}


static void entangle_application_init(EntangleApplication *app)
{
    EntangleApplicationPrivate *priv;
    int i;
    gchar *userdir;
    gchar **plugins;
    GtkSettings *gtk_settings;

    gtk_settings = gtk_settings_get_default();
    g_object_set(G_OBJECT(gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

    priv = app->priv = ENTANGLE_APPLICATION_GET_PRIVATE(app);

    priv->preferences = entangle_preferences_new();
    priv->activeCameras = entangle_camera_list_new_active();
    priv->supportedCameras = entangle_camera_list_new_supported();

    g_irepository_require(g_irepository_get_default(),
                          "Peas", "1.0", 0, NULL);

    userdir = g_build_filename(g_get_user_config_dir(), "entangle/plugins", NULL);
    g_mkdir_with_parents(userdir, 0777);

    priv->pluginEngine = peas_engine_get_default();
    peas_engine_enable_loader(priv->pluginEngine, "python3");

    peas_engine_add_search_path(priv->pluginEngine,
                                userdir, userdir);
    g_free(userdir);
    if (access("./entangle-256x256.png", R_OK) == 0)
        peas_engine_add_search_path(priv->pluginEngine,
                                    "./plugins",
                                    "./plugins");
    else
        peas_engine_add_search_path(priv->pluginEngine,
                                    LIBDIR "/entangle/plugins",
                                    DATADIR "/entangle/plugins");
    peas_engine_rescan_plugins(priv->pluginEngine);

    priv->pluginExt = peas_extension_set_new(priv->pluginEngine,
                                             PEAS_TYPE_ACTIVATABLE,
                                             "object", app,
                                             NULL);

    peas_extension_set_foreach(priv->pluginExt,
                               (PeasExtensionSetForeachFunc)on_extension_added,
                               NULL);

    g_signal_connect(priv->pluginExt, "extension-added",
                     G_CALLBACK(on_extension_added), NULL);
    g_signal_connect(priv->pluginExt, "extension-removed",
                     G_CALLBACK(on_extension_removed), NULL);

    g_signal_connect(priv->pluginEngine, "load-plugin",
                     G_CALLBACK(on_plugin_load), app);
    g_signal_connect(priv->pluginEngine, "unload-plugin",
                     G_CALLBACK(on_plugin_unload), app);

    plugins = entangle_preferences_interface_get_plugins(priv->preferences);
    for (i = 0; plugins[i] != NULL; i++) {
        PeasPluginInfo *plugin = peas_engine_get_plugin_info(priv->pluginEngine,
                                                             plugins[i]);
        if (plugin) {
            ENTANGLE_DEBUG("Plugin %p %s", plugin, plugins[i]);
            peas_engine_load_plugin(priv->pluginEngine, plugin);
        }
    }
    g_strfreev(plugins);
}


/**
 * entangle_application_get_active_cameras:
 *
 * Retrieve the active camera list
 *
 * Returns: (transfer none): the camera list
 */
EntangleCameraList *entangle_application_get_active_cameras(EntangleApplication *app)
{
    g_return_val_if_fail(ENTANGLE_IS_APPLICATION(app), NULL);

    EntangleApplicationPrivate *priv = app->priv;
    return priv->activeCameras;
}


/**
 * entangle_application_get_supported_cameras:
 *
 * Retrieve the supported camera list
 *
 * Returns: (transfer none): the camera list
 */
EntangleCameraList *entangle_application_get_supported_cameras(EntangleApplication *app)
{
    g_return_val_if_fail(ENTANGLE_IS_APPLICATION(app), NULL);

    EntangleApplicationPrivate *priv = app->priv;
    return priv->supportedCameras;
}


/**
 * entangle_application_get_preferences:
 *
 * Retrieve the application preferences object
 *
 * Returns: (transfer none): the application preferences
 */
EntanglePreferences *entangle_application_get_preferences(EntangleApplication *app)
{
    g_return_val_if_fail(ENTANGLE_IS_APPLICATION(app), NULL);

    EntangleApplicationPrivate *priv = app->priv;
    return priv->preferences;
}

/**
 * entangle_application_get_plugin_engine:
 *
 * Retrieve the plugin manager
 *
 * Returns: (transfer none): the plugin engine
 */
PeasEngine *entangle_application_get_plugin_engine(EntangleApplication *app)
{
    g_return_val_if_fail(ENTANGLE_IS_APPLICATION(app), NULL);

    EntangleApplicationPrivate *priv = app->priv;
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
