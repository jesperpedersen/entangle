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

#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "entangle-debug.h"
#include "entangle-preferences.h"
#include "entangle-progress.h"

#define ENTANGLE_PREFERENCES_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PREFERENCES, EntanglePreferencesPrivate))

struct _EntanglePreferencesPrivate {
    char *pictureDir;
    char *filenamePattern;

    gboolean enableColourManagement;
    EntangleColourProfile *rgbProfile;
    EntangleColourProfile *monitorProfile;
    gboolean detectSystemProfile;
    EntangleColourProfileIntent renderingIntent;
};

G_DEFINE_TYPE(EntanglePreferences, entangle_preferences, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_PICTURE_DIR,
    PROP_FILENAME_PATTERN,
    PROP_COLOUR_MANAGED_DISPLAY,
    PROP_RGB_PROFILE,
    PROP_MONITOR_PROFILE,
    PROP_DETECT_SYSTEM_PROFILE,
    PROP_PROFILE_RENDERING_INTENT,
};


static char *entangle_find_picture_dir(void)
{
    const gchar *baseDir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    gchar *ret;
    if (baseDir) {
        ret = g_strdup_printf("%s/%s", baseDir, "Capture");
    } else {
        ret = g_strdup_printf("Capture");
    }
    ENTANGLE_DEBUG("******** PICTURE '%s'", ret);
    return ret;
}

static void entangle_preferences_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    EntanglePreferences *picker = ENTANGLE_PREFERENCES(object);
    EntanglePreferencesPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_PICTURE_DIR:
            g_value_set_string(value, priv->pictureDir);
            break;

        case PROP_FILENAME_PATTERN:
            g_value_set_string(value, priv->filenamePattern);
            break;

        case PROP_COLOUR_MANAGED_DISPLAY:
            g_value_set_boolean(value, priv->enableColourManagement);
            break;

        case PROP_RGB_PROFILE:
            g_value_set_object(value, priv->rgbProfile);
            break;

        case PROP_MONITOR_PROFILE:
            g_value_set_object(value, priv->monitorProfile);
            break;

        case PROP_DETECT_SYSTEM_PROFILE:
            g_value_set_boolean(value, priv->detectSystemProfile);
            break;

        case PROP_PROFILE_RENDERING_INTENT:
            g_value_set_enum(value, priv->renderingIntent);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_preferences_set_property(GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
    EntanglePreferences *picker = ENTANGLE_PREFERENCES(object);
    EntanglePreferencesPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_PICTURE_DIR:
            g_free(priv->pictureDir);
            priv->pictureDir = g_value_dup_string(value);
            break;

        case PROP_FILENAME_PATTERN:
            g_free(priv->filenamePattern);
            priv->filenamePattern = g_value_dup_string(value);
            break;

        case PROP_COLOUR_MANAGED_DISPLAY:
            priv->enableColourManagement = g_value_get_boolean(value);
            break;

        case PROP_RGB_PROFILE:
            if (priv->rgbProfile)
                g_object_unref(priv->rgbProfile);
            priv->rgbProfile = g_value_get_object(value);
            g_object_ref(priv->rgbProfile);
            break;

        case PROP_MONITOR_PROFILE:
            if (priv->monitorProfile)
                g_object_unref(priv->monitorProfile);
            priv->monitorProfile = g_value_get_object(value);
            g_object_ref(priv->monitorProfile);
            break;

        case PROP_DETECT_SYSTEM_PROFILE:
            priv->detectSystemProfile = g_value_get_boolean(value);
            break;

        case PROP_PROFILE_RENDERING_INTENT:
            priv->renderingIntent = g_value_get_enum(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_preferences_finalize(GObject *object)
{
    EntanglePreferences *preferences = ENTANGLE_PREFERENCES(object);
    EntanglePreferencesPrivate *priv = preferences->priv;

    ENTANGLE_DEBUG("Finalize preferences %p", object);

    g_free(priv->pictureDir);
    g_free(priv->filenamePattern);
    if (priv->rgbProfile)
        g_object_unref(priv->rgbProfile);
    if (priv->monitorProfile)
        g_object_unref(priv->monitorProfile);

    G_OBJECT_CLASS (entangle_preferences_parent_class)->finalize (object);
}


static void entangle_preferences_class_init(EntanglePreferencesClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_preferences_finalize;
    object_class->get_property = entangle_preferences_get_property;
    object_class->set_property = entangle_preferences_set_property;

    g_object_class_install_property(object_class,
                                    PROP_PICTURE_DIR,
                                    g_param_spec_string("picture-dir",
                                                        "Pictures directory",
                                                        "Directory to store pictures in",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_FILENAME_PATTERN,
                                    g_param_spec_string("filename-pattern",
                                                        "Filename pattern",
                                                        "Pattern for creating new filenames",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_COLOUR_MANAGED_DISPLAY,
                                    g_param_spec_boolean("colour-managed-display",
                                                         "Colour managed display",
                                                         "Whether to enable colour management on display",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_RGB_PROFILE,
                                    g_param_spec_object("rgb-profile",
                                                        "RGB Profile",
                                                        "Colour profile for workspace",
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_MONITOR_PROFILE,
                                    g_param_spec_object("monitor-profile",
                                                        "Monitor profile",
                                                        "Colour profile for monitor",
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_DETECT_SYSTEM_PROFILE,
                                    g_param_spec_boolean("detect-system-profile",
                                                         "Detect system profile",
                                                         "Detect the monitor colour profile",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PROFILE_RENDERING_INTENT,
                                    g_param_spec_enum("profile-rendering-intent",
                                                      "Profile rendering intent",
                                                      "Rendering intent for images",
                                                      ENTANGLE_TYPE_COLOUR_PROFILE_INTENT,
                                                      ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntanglePreferencesPrivate));
}


EntanglePreferences *entangle_preferences_new(void)
{
    return ENTANGLE_PREFERENCES(g_object_new(ENTANGLE_TYPE_PREFERENCES, NULL));
}


static void entangle_preferences_init(EntanglePreferences *picker)
{
    EntanglePreferencesPrivate *priv;

    priv = picker->priv = ENTANGLE_PREFERENCES_GET_PRIVATE(picker);
    memset(priv, 0, sizeof(*priv));

    priv->pictureDir = entangle_find_picture_dir();
    priv->filenamePattern = g_strdup("captureXXXXXX");

    priv->enableColourManagement = TRUE;
    priv->detectSystemProfile = TRUE;
    priv->renderingIntent = ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL;
    if (access("./frontend/entangle-camera-manager.xml", R_OK) == 0)
        priv->rgbProfile = entangle_colour_profile_new_file(g_strdup("./sRGB.icc"));
    else
        priv->rgbProfile = entangle_colour_profile_new_file(g_strdup(PKGDATADIR "/sRGB.icc"));
}


const char *entangle_preferences_picture_dir(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return priv->pictureDir;
}


const char *entangle_preferences_filename_pattern(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return priv->filenamePattern;
}


EntangleColourProfile *entangle_preferences_rgb_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return priv->rgbProfile;
}


EntangleColourProfile *entangle_preferences_monitor_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return priv->monitorProfile;
}


gboolean entangle_preferences_enable_color_management(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return priv->enableColourManagement;
}

gboolean entangle_preferences_detect_monitor_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return priv->detectSystemProfile;
}


EntangleColourProfileIntent entangle_preferences_profile_rendering_intent(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return priv->renderingIntent;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
