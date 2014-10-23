/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2014 Daniel P. Berrange
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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "entangle-debug.h"
#include "entangle-camera-preferences.h"

#define ENTANGLE_CAMERA_PREFERENCES_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_PREFERENCES, EntangleCameraPreferencesPrivate))

struct _EntangleCameraPreferencesPrivate {
    EntangleCamera *camera;
    GSettings *settings;
};

G_DEFINE_TYPE(EntangleCameraPreferences, entangle_camera_preferences, G_TYPE_OBJECT);

enum {
    PROP_O,
    PROP_CAMERA,
};

static void entangle_camera_preferences_get_property(GObject *object,
                                                     guint prop_id,
                                                     GValue *value,
                                                     GParamSpec *pspec)
{
    EntangleCameraPreferences *preferences = ENTANGLE_CAMERA_PREFERENCES(object);
    EntangleCameraPreferencesPrivate *priv = preferences->priv;

    switch (prop_id)
        {
        case PROP_CAMERA:
            g_value_set_object(value, priv->camera);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_preferences_set_property(GObject *object,
                                                     guint prop_id,
                                                     const GValue *value,
                                                     GParamSpec *pspec)
{
    EntangleCameraPreferences *preferences = ENTANGLE_CAMERA_PREFERENCES(object);

    ENTANGLE_DEBUG("Set prop on camera preferences %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            entangle_camera_preferences_set_camera(preferences, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_preferences_finalize(GObject *object)
{
    EntangleCameraPreferences *preferences = ENTANGLE_CAMERA_PREFERENCES(object);
    EntangleCameraPreferencesPrivate *priv = preferences->priv;

    ENTANGLE_DEBUG("Finalize preferences %p", object);

    if (priv->settings)
        g_object_unref(priv->settings);

    G_OBJECT_CLASS(entangle_camera_preferences_parent_class)->finalize(object);
}


static void entangle_camera_preferences_class_init(EntangleCameraPreferencesClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_camera_preferences_finalize;
    object_class->get_property = entangle_camera_preferences_get_property;
    object_class->set_property = entangle_camera_preferences_set_property;

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to managed",
                                                        ENTANGLE_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleCameraPreferencesPrivate));
}


EntangleCameraPreferences *entangle_camera_preferences_new(void)
{
    return ENTANGLE_CAMERA_PREFERENCES(g_object_new(ENTANGLE_TYPE_CAMERA_PREFERENCES,
                                                    NULL));
}


static void entangle_camera_preferences_init(EntangleCameraPreferences *prefs)
{
    prefs->priv = ENTANGLE_CAMERA_PREFERENCES_GET_PRIVATE(prefs);
}


/**
 * entangle_camera_preferences_set_camera:
 * @preferences: the camera widget
 * @camera: (transfer none)(allow-none): the camera to display cameras for, or NULL
 *
 * Set the camera to display cameras for
 */
void entangle_camera_preferences_set_camera(EntangleCameraPreferences *prefs,
                                            EntangleCamera *camera)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_PREFERENCES(prefs));
    g_return_if_fail(!camera || ENTANGLE_IS_CAMERA(camera));

    EntangleCameraPreferencesPrivate *priv = prefs->priv;

    if (priv->camera) {
        g_object_unref(priv->camera);
        g_object_unref(priv->settings);
        priv->camera = NULL;
        priv->settings = NULL;
    }
    if (camera) {
        priv->camera = g_object_ref(camera);

        gchar *suffix = g_strdup(entangle_camera_get_model(priv->camera));
        gsize i;
        for (i = 0; suffix[i] != '\0'; i++) {
            if (g_ascii_isalnum(suffix[i]) ||
                suffix[i] == '/' ||
                suffix[i] == '-')
                continue;
            suffix[i] = '-';
        }
        gchar *path = g_strdup_printf("/org/entangle-photo/manager/camera/%s/",
                                      suffix);
        priv->settings = g_settings_new_with_path("org.entangle-photo.manager.camera",
                                                  path);

        g_free(suffix);
        g_free(path);
    }
    g_object_notify(G_OBJECT(prefs), "camera");
}


/**
 * entangle_camera_preferences_get_camera:
 * @preferences: the camera widget
 *
 * Get the camera whose cameras are displayed
 *
 * Returns: (transfer none): the camera or NULL
 */
EntangleCamera *entangle_camera_preferences_get_camera(EntangleCameraPreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_PREFERENCES(prefs), NULL);

    EntangleCameraPreferencesPrivate *priv = prefs->priv;

    return priv->camera;
}


/**
 * entangle_camera_preferences_get_controls:
 * @prefs: (transfer none): the camera preferences
 *
 * Returns: (transfer full): the list of controls
 */
gchar **entangle_camera_preferences_get_controls(EntangleCameraPreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_PREFERENCES(prefs), NULL);

    EntangleCameraPreferencesPrivate *priv = prefs->priv;

    if (!priv->settings)
        return NULL;

    return g_settings_get_strv(priv->settings, "controls");
}


/**
 * entangle_camera_preferences_set_controls:
 * @prefs: (transfer none): the camera preferences
 * @controls: (transfer none): the list of controls
 */
void entangle_camera_preferences_set_controls(EntangleCameraPreferences *prefs,
                                              const gchar *const *controls)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_PREFERENCES(prefs));

    EntangleCameraPreferencesPrivate *priv = prefs->priv;

    if (!priv->settings)
        return;

    g_settings_set_strv(priv->settings, "controls", controls);
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
