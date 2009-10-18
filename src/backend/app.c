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

#include <gphoto2.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "internal.h"
#include "app.h"
#include "params.h"
#include "device-manager.h"

#define CAPA_APP_GET_PRIVATE(obj)                                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_APP, CapaAppPrivate))

struct _CapaAppPrivate {
    CapaParams *params;

    CapaDeviceManager *devManager;
    CapaCameraList *cameras;

    CapaPreferences *preferences;
};

G_DEFINE_TYPE(CapaApp, capa_app, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_CAMERAS,
    PROP_PREFERENCES,
    PROP_DEVMANAGER,
};

static void capa_app_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
    CapaApp *app = CAPA_APP(object);
    CapaAppPrivate *priv = app->priv;

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

static void capa_app_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    CapaApp *app = CAPA_APP(object);
    CapaAppPrivate *priv = app->priv;

    switch (prop_id)
        {
        case PROP_CAMERAS:
            CAPA_DEBUG("DAMN");
            if (priv->cameras)
                g_object_unref(G_OBJECT(priv->cameras));
            priv->cameras = g_value_get_object(value);
            g_object_ref(priv->cameras);
            break;

        case PROP_PREFERENCES:
            if (priv->preferences)
                g_object_unref(G_OBJECT(priv->preferences));
            priv->preferences = g_value_get_object(value);
            g_object_ref(priv->preferences);
            break;

        case PROP_DEVMANAGER:
            if (priv->devManager)
                g_object_unref(G_OBJECT(priv->devManager));
            priv->devManager = g_value_get_object(value);
            g_object_ref(priv->devManager);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void capa_app_finalize(GObject *object)
{
    CapaApp *app = CAPA_APP(object);
    CapaAppPrivate *priv = app->priv;

    CAPA_DEBUG("Finalize app %p", object);

    if (priv->cameras)
        g_object_unref(priv->cameras);
    if (priv->preferences)
        g_object_unref(priv->preferences);
    if (priv->devManager)
        g_object_unref(priv->devManager);

    capa_params_free(priv->params);

    G_OBJECT_CLASS (capa_app_parent_class)->finalize (object);
}


static void capa_app_class_init(CapaAppClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_app_finalize;
    object_class->get_property = capa_app_get_property;
    object_class->set_property = capa_app_set_property;

    g_object_class_install_property(object_class,
                                    PROP_CAMERAS,
                                    g_param_spec_object("cameras",
                                                        "Camera list",
                                                        "List of detected cameras",
                                                        CAPA_TYPE_CAMERA_LIST,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PREFERENCES,
                                    g_param_spec_object("preferences",
                                                        "Preferences",
                                                        "Application preferences",
                                                        CAPA_TYPE_PREFERENCES,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_DEVMANAGER,
                                    g_param_spec_object("device-manager",
                                                        "Device manager",
                                                        "Device manager for detecting cameras",
                                                        CAPA_TYPE_SESSION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(CapaAppPrivate));
}


static void do_refresh_cameras(CapaApp *app)
{
    CapaAppPrivate *priv = app->priv;
    CameraList *cams = NULL;
    GHashTable *toRemove;
    GHashTableIter iter;
    gpointer key, value;

    capa_params_refresh(priv->params);

    CAPA_DEBUG("Detecting cameras");

    if (gp_list_new(&cams) != GP_OK)
        return;

    gp_abilities_list_detect(priv->params->caps, priv->params->ports, cams, priv->params->ctx);

    for (int i = 0 ; i < gp_list_count(cams) ; i++) {
        const char *model, *port;
        int n;
        CapaCamera *cam;
        CameraAbilities cap;

        gp_list_get_name(cams, i, &model);
        gp_list_get_value(cams, i, &port);

        cam = capa_camera_list_find(priv->cameras, port);

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

        CAPA_DEBUG("New camera '%s' '%s' %d", model, port, cap.operations);
        cam = capa_camera_new(model, port,
                              cap.operations & GP_OPERATION_CAPTURE_IMAGE ? TRUE : FALSE,
                              cap.operations & GP_OPERATION_CAPTURE_PREVIEW ? TRUE : FALSE,
                              cap.operations & GP_OPERATION_CONFIG ? TRUE : FALSE);
        capa_camera_list_add(priv->cameras, cam);
        g_object_unref(G_OBJECT(cam));
    }

    toRemove = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    for (int i = 0 ; i < capa_camera_list_count(priv->cameras) ; i++) {
        gboolean found = FALSE;
        CapaCamera *cam = capa_camera_list_get(priv->cameras, i);

        CAPA_DEBUG("Checking if %s exists", capa_camera_port(cam));

        for (int j = 0 ; j < gp_list_count(cams) ; j++) {
            const char *port;
            gp_list_get_value(cams, j, &port);

            if (strcmp(port, capa_camera_port(cam)) == 0) {
                found = TRUE;
                break;
            }
        }
        if (!found)
            g_hash_table_insert(toRemove, g_strdup(capa_camera_port(cam)), cam);
    }

    gp_list_unref(cams);

    g_hash_table_iter_init(&iter, toRemove);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        CapaCamera *cam = value;

        capa_camera_list_remove(priv->cameras, cam);
    }
    g_hash_table_unref(toRemove);
}

static void do_device_addremove(CapaDeviceManager *manager G_GNUC_UNUSED,
                                char *port G_GNUC_UNUSED,
                                CapaApp *app)
{
    do_refresh_cameras(app);
}

CapaApp *capa_app_new(void)
{
    return CAPA_APP(g_object_new(CAPA_TYPE_APP, NULL));
}


static void capa_app_init(CapaApp *app)
{
    CapaAppPrivate *priv;

    priv = app->priv = CAPA_APP_GET_PRIVATE(app);

    priv->preferences = capa_preferences_new();
    priv->params = capa_params_new();
    priv->cameras = capa_camera_list_new();
    priv->devManager = capa_device_manager_new();

    g_signal_connect(priv->devManager, "device-added", G_CALLBACK(do_device_addremove), app);
    g_signal_connect(priv->devManager, "device-removed", G_CALLBACK(do_device_addremove), app);

    do_refresh_cameras(app);
}


void capa_app_refresh_cameras(CapaApp *app)
{
    do_refresh_cameras(app);
}

CapaCameraList *capa_app_cameras(CapaApp *app)
{
    CapaAppPrivate *priv = app->priv;
    return priv->cameras;
}

CapaPreferences *capa_app_preferences(CapaApp *app)
{
    CapaAppPrivate *priv = app->priv;
    return priv->preferences;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
