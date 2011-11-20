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

#include <config.h>

#include <gphoto2.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "entangle-debug.h"
#include "entangle-app.h"
#include "entangle-device-manager.h"
#include "entangle-preferences.h"

/**
 * @Short_description: Global application state base class
 * @Title: EntangleApp
 *
 * <para>
 * EntangleApp maintains some global application state. At this time,
 * the list of currently attached cameras, the application preferences
 * and the plugin manager.
 * </para>
 *
 * <para>
 * This class will normally be sub-classed when creating a Entangle based
 * application, typically in order to add in UI state.
 * </para>
 */


#define ENTANGLE_APP_GET_PRIVATE(obj)                                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_APP, EntangleAppPrivate))

struct _EntangleAppPrivate {
    GApplication *app;

    GPContext *ctx;
    CameraAbilitiesList *caps;
    GPPortInfoList *ports;

    EntangleDeviceManager *devManager;
    EntangleCameraList *cameras;

    EntanglePreferences *preferences;

#if HAVE_PLUGINS
    PeasEngine *pluginEngine;
    PeasExtensionSet *pluginExt;
#endif
};

G_DEFINE_TYPE(EntangleApp, entangle_app, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_APP,
    PROP_CAMERAS,
    PROP_PREFERENCES,
    PROP_DEVMANAGER,
};

static void entangle_app_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
    EntangleApp *app = ENTANGLE_APP(object);
    EntangleAppPrivate *priv = app->priv;

    switch (prop_id)
        {
        case PROP_APP:
            g_value_set_object(value, priv->app);
            break;

        case PROP_CAMERAS:
            g_value_set_object(value, priv->cameras);
            break;

        case PROP_PREFERENCES:
            g_value_set_object(value, priv->preferences);
            break;

        case PROP_DEVMANAGER:
            g_value_set_object(value, priv->devManager);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_app_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    EntangleApp *app = ENTANGLE_APP(object);
    EntangleAppPrivate *priv = app->priv;

    switch (prop_id)
        {
        case PROP_APP:
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

        case PROP_DEVMANAGER:
            if (priv->devManager)
                g_object_unref(priv->devManager);
            priv->devManager = g_value_get_object(value);
            g_object_ref(priv->devManager);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_app_finalize(GObject *object)
{
    EntangleApp *app = ENTANGLE_APP(object);
    EntangleAppPrivate *priv = app->priv;

    ENTANGLE_DEBUG("Finalize app %p", object);

    if (priv->app)
        g_object_unref(priv->app);
    if (priv->cameras)
        g_object_unref(priv->cameras);
    if (priv->preferences)
        g_object_unref(priv->preferences);
    if (priv->devManager)
        g_object_unref(priv->devManager);
#if HAVE_PLUGINS
    if (priv->pluginEngine)
        g_object_unref(priv->pluginEngine);
    if (priv->pluginExt)
        g_object_unref(priv->pluginExt);
#endif

    if (priv->ports)
        gp_port_info_list_free(priv->ports);
    if (priv->caps)
        gp_abilities_list_free(priv->caps);
    gp_context_unref(priv->ctx);

    G_OBJECT_CLASS (entangle_app_parent_class)->finalize (object);
}


static void entangle_app_class_init(EntangleAppClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_app_finalize;
    object_class->get_property = entangle_app_get_property;
    object_class->set_property = entangle_app_set_property;

    g_object_class_install_property(object_class,
                                    PROP_APP,
                                    g_param_spec_object("app",
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

    g_object_class_install_property(object_class,
                                    PROP_DEVMANAGER,
                                    g_param_spec_object("device-manager",
                                                        "Device manager",
                                                        "Device manager for detecting cameras",
                                                        ENTANGLE_TYPE_DEVICE_MANAGER,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(EntangleAppPrivate));
}


static void do_refresh_cameras(EntangleApp *app)
{
    EntangleAppPrivate *priv = app->priv;
    CameraList *cams = NULL;
    GHashTable *toRemove;
    GHashTableIter iter;
    gpointer key, value;

    if (priv->ports)
        gp_port_info_list_free(priv->ports);
    if (gp_port_info_list_new(&priv->ports) != GP_OK)
        return;
    if (gp_port_info_list_load(priv->ports) != GP_OK)
        return;

    ENTANGLE_DEBUG("Detecting cameras");

    if (gp_list_new(&cams) != GP_OK)
        return;

    gp_abilities_list_detect(priv->caps, priv->ports, cams, priv->ctx);

    for (int i = 0 ; i < gp_list_count(cams) ; i++) {
        const char *model, *port;
        int n;
        EntangleCamera *cam;
        CameraAbilities cap;

        gp_list_get_name(cams, i, &model);
        gp_list_get_value(cams, i, &port);

        cam = entangle_camera_list_find(priv->cameras, port);

        if (cam)
            continue;

        n = gp_abilities_list_lookup_model(priv->caps, model);
        gp_abilities_list_get_abilities(priv->caps, n, &cap);

        /* For back compat, libgphoto2 always adds a default
         * USB camera called 'usb:'. We ignore that, since we
         * can go for the exact camera entries
         */
        if (strcmp(port, "usb:") == 0)
            continue;

        ENTANGLE_DEBUG("New camera '%s' '%s' %d", model, port, cap.operations);
        cam = entangle_camera_new(model, port,
                              cap.operations & GP_OPERATION_CAPTURE_IMAGE ? TRUE : FALSE,
                              cap.operations & GP_OPERATION_CAPTURE_PREVIEW ? TRUE : FALSE,
                              cap.operations & GP_OPERATION_CONFIG ? TRUE : FALSE);
        entangle_camera_list_add(priv->cameras, cam);
        g_object_unref(cam);
    }

    toRemove = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    for (int i = 0 ; i < entangle_camera_list_count(priv->cameras) ; i++) {
        gboolean found = FALSE;
        EntangleCamera *cam = entangle_camera_list_get(priv->cameras, i);

        ENTANGLE_DEBUG("Checking if %s exists", entangle_camera_get_port(cam));

        for (int j = 0 ; j < gp_list_count(cams) ; j++) {
            const char *port;
            gp_list_get_value(cams, j, &port);

            if (strcmp(port, entangle_camera_get_port(cam)) == 0) {
                found = TRUE;
                break;
            }
        }
        if (!found)
            g_hash_table_insert(toRemove, g_strdup(entangle_camera_get_port(cam)), cam);
    }

    gp_list_unref(cams);

    g_hash_table_iter_init(&iter, toRemove);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        EntangleCamera *cam = value;

        entangle_camera_list_remove(priv->cameras, cam);
    }
    g_hash_table_unref(toRemove);
}

static void do_device_addremove(EntangleDeviceManager *manager G_GNUC_UNUSED,
                                char *port G_GNUC_UNUSED,
                                EntangleApp *app)
{
    do_refresh_cameras(app);
}

EntangleApp *entangle_app_new(GApplication *app)
{
    return ENTANGLE_APP(g_object_new(ENTANGLE_TYPE_APP,
                                     "app", app,
                                     NULL));
}

#if HAVE_PLUGINS
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
#endif

static void do_entangle_log(GPLogLevel level G_GNUC_UNUSED,
                            const char *domain,
                            const char *format,
                            va_list args,
                            void *data G_GNUC_UNUSED)
{
    char *msg = g_strdup_vprintf(format, args);
    g_debug("%s: %s", domain, msg);
}

static void entangle_app_init(EntangleApp *app)
{
    EntangleAppPrivate *priv;
#if HAVE_PLUGINS
    gchar **peasPath;
    int i;
#endif

    priv = app->priv = ENTANGLE_APP_GET_PRIVATE(app);

    priv->preferences = entangle_preferences_new();
    priv->cameras = entangle_camera_list_new();
    priv->devManager = entangle_device_manager_new();

    if (entangle_debug_gphoto) {
        gp_log_add_func(GP_LOG_DEBUG, do_entangle_log, NULL);
    }

    priv->ctx = gp_context_new();

    if (gp_abilities_list_new(&priv->caps) != GP_OK)
        g_error("Cannot initialize gphoto2 abilities");

    if (gp_abilities_list_load(priv->caps, priv->ctx) != GP_OK)
        g_error("Cannot load gphoto2 abilities");

    if (gp_port_info_list_new(&priv->ports) != GP_OK)
        g_error("Cannot initialize gphoto2 ports");

    if (gp_port_info_list_load(priv->ports) != GP_OK)
        g_error("Cannot load gphoto2 ports");

#if HAVE_PLUGINS
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
                                             "object", app,
                                             NULL);

    peas_extension_set_call(priv->pluginExt, "activate");

    g_signal_connect(priv->pluginExt, "extension-added",
                     G_CALLBACK(on_extension_added), app);
    g_signal_connect(priv->pluginExt, "extension-removed",
                     G_CALLBACK(on_extension_removed), app);

    g_free(peasPath[0]);
    g_free(peasPath[1]);
    g_free(peasPath[2]);
    g_free(peasPath[3]);
    g_free(peasPath[4]);
    g_free(peasPath);
#endif

    g_signal_connect(priv->devManager, "device-added", G_CALLBACK(do_device_addremove), app);
    g_signal_connect(priv->devManager, "device-removed", G_CALLBACK(do_device_addremove), app);

    do_refresh_cameras(app);

#if HAVE_PLUGINS
    const GList *plugins = peas_engine_get_plugin_list(priv->pluginEngine);
    for (i = 0 ; i < g_list_length((GList *)plugins) ; i++) {
        PeasPluginInfo *plugin = g_list_nth_data((GList*)plugins, i);
        peas_engine_load_plugin(priv->pluginEngine, plugin);
    }
#endif
}


/*
 * entangle_app_refresh_cameras: Refresh the camera list
 *
 * Rescans all attached devices to look for new cameras.
 * This method is unneccessary if only interested in USB
 * cameras, since those are automatically detected via the
 * device manager when plugged in.
 */
void entangle_app_refresh_cameras(EntangleApp *app)
{
    do_refresh_cameras(app);
}

/**
 * entangle_app_get_cameras: Retrieve the camera list
 *
 * Returns: (transfer none): the camera list
 */
EntangleCameraList *entangle_app_get_cameras(EntangleApp *app)
{
    EntangleAppPrivate *priv = app->priv;
    return priv->cameras;
}


/**
 * entangle_app_get_preferences: Retrieve the application preferences object
 *
 * Returns: (transfer none): the application preferences
 */
EntanglePreferences *entangle_app_get_preferences(EntangleApp *app)
{
    EntangleAppPrivate *priv = app->priv;
    return priv->preferences;
}

#if HAVE_PLUGINS
/**
 * entangle_app_get_plugin_engine: Retrieve the plugin manager
 *
 * Returns: (transfer none): the plugin engine
 */
PeasEngine *entangle_app_get_plugin_engine(EntangleApp *app)
{
    EntangleAppPrivate *priv = app->priv;
    return priv->pluginEngine;
}
#endif


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
