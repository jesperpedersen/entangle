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
#include "entangle-params.h"
#include "entangle-device-manager.h"
#include "entangle-preferences-gconf.h"
#include "entangle-plugin-native.h"
#include "entangle-plugin-javascript.h"

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
    EntangleParams *params;

    EntangleDeviceManager *devManager;
    EntangleCameraList *cameras;

    EntanglePreferences *preferences;

    EntanglePluginManager *pluginManager;
};

G_DEFINE_TYPE(EntangleApp, entangle_app, G_TYPE_OBJECT);

enum {
    PROP_0,
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
        case PROP_CAMERAS:
            ENTANGLE_DEBUG("DAMN");
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

    if (priv->cameras)
        g_object_unref(priv->cameras);
    if (priv->preferences)
        g_object_unref(priv->preferences);
    if (priv->devManager)
        g_object_unref(priv->devManager);
    if (priv->pluginManager)
        g_object_unref(priv->pluginManager);

    entangle_params_free(priv->params);

    G_OBJECT_CLASS (entangle_app_parent_class)->finalize (object);
}


static void entangle_app_class_init(EntangleAppClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_app_finalize;
    object_class->get_property = entangle_app_get_property;
    object_class->set_property = entangle_app_set_property;

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

    entangle_params_refresh(priv->params);

    ENTANGLE_DEBUG("Detecting cameras");

    if (gp_list_new(&cams) != GP_OK)
        return;

    gp_abilities_list_detect(priv->params->caps, priv->params->ports, cams, priv->params->ctx);

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

        n = gp_abilities_list_lookup_model(priv->params->caps, model);
        gp_abilities_list_get_abilities(priv->params->caps, n, &cap);

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

EntangleApp *entangle_app_new(void)
{
    return ENTANGLE_APP(g_object_new(ENTANGLE_TYPE_APP, NULL));
}


static void entangle_app_init(EntangleApp *app)
{
    EntangleAppPrivate *priv;
    gint newmodules;

    priv = app->priv = ENTANGLE_APP_GET_PRIVATE(app);

    priv->preferences = entangle_preferences_gconf_new();
    priv->params = entangle_params_new();
    priv->cameras = entangle_camera_list_new();
    priv->devManager = entangle_device_manager_new();
    priv->pluginManager = entangle_plugin_manager_new();

    g_signal_connect(priv->devManager, "device-added", G_CALLBACK(do_device_addremove), app);
    g_signal_connect(priv->devManager, "device-removed", G_CALLBACK(do_device_addremove), app);

    do_refresh_cameras(app);

    entangle_plugin_manager_register_type(priv->pluginManager, "native",
                                      ENTANGLE_TYPE_PLUGIN_NATIVE);
    /* Scan repeatedly because each new plugin may register
     * new plugin types, requiring a repeat scan
     */
    do {
        newmodules = entangle_plugin_manager_scan(priv->pluginManager);
        entangle_plugin_manager_activate(priv->pluginManager, G_OBJECT(app));
        ENTANGLE_DEBUG("Activated %d plugins", newmodules);
    } while (newmodules > 0);
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


/**
 * entangle_app_get_plugin_manager: Retrieve the plugin manager
 *
 * Returns: (transfer none): the plugin manager
 */
EntanglePluginManager *entangle_app_get_plugin_manager(EntangleApp *app)
{
    EntangleAppPrivate *priv = app->priv;
    return priv->pluginManager;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
