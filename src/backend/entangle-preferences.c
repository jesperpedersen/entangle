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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "entangle-debug.h"
#include "entangle-preferences.h"
#include "entangle-progress.h"

#define ENTANGLE_PREFERENCES_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PREFERENCES, EntanglePreferencesPrivate))

struct _EntanglePreferencesPrivate {
    GSettings *interfaceSettings;
    GSettings *captureSettings;
    GSettings *cmsSettings;
    GSettings *imgSettings;
};

G_DEFINE_TYPE(EntanglePreferences, entangle_preferences, G_TYPE_OBJECT);

#define SETTING_INTERFACE                  "interface"
#define SETTING_CAPTURE                    "capture"
#define SETTING_CMS                        "cms"
#define SETTING_IMG                        "img"

#define SETTING_INTERFACE_AUTO_CONNECT     "auto-connect"
#define SETTING_INTERFACE_SCREEN_BLANK     "screen-blank"
#define SETTING_INTERFACE_PLUGINS          "plugins"

#define SETTING_CAPTURE_FILENAME_PATTERN   "filename-pattern"
#define SETTING_CAPTURE_LAST_SESSION       "last-session"
#define SETTING_CAPTURE_CONTINUOUS_PREVIEW "continuous-preview"
#define SETTING_CAPTURE_DELETE_FILE        "delete-file"

#define SETTING_CMS_ENABLED                "enabled"
#define SETTING_CMS_DETECT_SYSTEM_PROFILE  "detect-system-profile"
#define SETTING_CMS_RGB_PROFILE            "rgb-profile"
#define SETTING_CMS_MONITOR_PROFILE        "monitor-profile"
#define SETTING_CMS_RENDERING_INTENT       "rendering-intent"

#define SETTING_IMG_ASPECT_RATIO           "aspect-ratio"
#define SETTING_IMG_MASK_OPACITY           "mask-opacity"
#define SETTING_IMG_MASK_ENABLED           "mask-enabled"
#define SETTING_IMG_FOCUS_POINT            "focus-point"
#define SETTING_IMG_GRID_LINES             "grid-lines"

#define PROP_NAME_INTERFACE_AUTO_CONNECT     SETTING_INTERFACE "-" SETTING_INTERFACE_AUTO_CONNECT
#define PROP_NAME_INTERFACE_SCREEN_BLANK     SETTING_INTERFACE "-" SETTING_INTERFACE_SCREEN_BLANK

#define PROP_NAME_CAPTURE_FILENAME_PATTERN   SETTING_CAPTURE "-" SETTING_CAPTURE_FILENAME_PATTERN
#define PROP_NAME_CAPTURE_LAST_SESSION       SETTING_CAPTURE "-" SETTING_CAPTURE_LAST_SESSION
#define PROP_NAME_CAPTURE_CONTINUOUS_PREVIEW SETTING_CAPTURE "-" SETTING_CAPTURE_CONTINUOUS_PREVIEW
#define PROP_NAME_CAPTURE_DELETE_FILE        SETTING_CAPTURE "-" SETTING_CAPTURE_DELETE_FILE

#define PROP_NAME_CMS_ENABLED                SETTING_CMS "-" SETTING_CMS_ENABLED
#define PROP_NAME_CMS_DETECT_SYSTEM_PROFILE  SETTING_CMS "-" SETTING_CMS_DETECT_SYSTEM_PROFILE
#define PROP_NAME_CMS_RGB_PROFILE            SETTING_CMS "-" SETTING_CMS_RGB_PROFILE
#define PROP_NAME_CMS_MONITOR_PROFILE        SETTING_CMS "-" SETTING_CMS_MONITOR_PROFILE
#define PROP_NAME_CMS_RENDERING_INTENT       SETTING_CMS "-" SETTING_CMS_RENDERING_INTENT

#define PROP_NAME_IMG_ASPECT_RATIO           SETTING_IMG "-" SETTING_IMG_ASPECT_RATIO
#define PROP_NAME_IMG_MASK_OPACITY           SETTING_IMG "-" SETTING_IMG_MASK_OPACITY
#define PROP_NAME_IMG_MASK_ENABLED           SETTING_IMG "-" SETTING_IMG_MASK_ENABLED
#define PROP_NAME_IMG_FOCUS_POINT            SETTING_IMG "-" SETTING_IMG_FOCUS_POINT
#define PROP_NAME_IMG_GRID_LINES             SETTING_IMG "-" SETTING_IMG_GRID_LINES

enum {
    PROP_0,

    PROP_INTERFACE_AUTO_CONNECT,
    PROP_INTERFACE_SCREEN_BLANK,

    PROP_CAPTURE_FILENAME_PATTERN,
    PROP_CAPTURE_LAST_SESSION,
    PROP_CAPTURE_CONTINUOUS_PREVIEW,
    PROP_CAPTURE_DELETE_FILE,

    PROP_CMS_ENABLED,
    PROP_CMS_RGB_PROFILE,
    PROP_CMS_MONITOR_PROFILE,
    PROP_CMS_DETECT_SYSTEM_PROFILE,
    PROP_CMS_RENDERING_INTENT,

    PROP_IMG_ASPECT_RATIO,
    PROP_IMG_MASK_OPACITY,
    PROP_IMG_MASK_ENABLED,
    PROP_IMG_FOCUS_POINT,
    PROP_IMG_GRID_LINES,
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


static char *entangle_find_srgb_profile(void)
{
    gchar *filename;
    if (access("./sRGB.icc", R_OK) == 0)
        filename = g_strdup("./sRGB.icc");
    else
        filename = g_strdup(PKGDATADIR "/sRGB.icc");
    return filename;
}

static void entangle_preferences_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    EntanglePreferences *picker = ENTANGLE_PREFERENCES(object);
    EntanglePreferencesPrivate *priv = picker->priv;
    EntangleColourProfile *prof;
    gchar *dir;
    gchar *file;

    switch (prop_id)
        {
        case PROP_INTERFACE_AUTO_CONNECT:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->interfaceSettings,
                                                       SETTING_INTERFACE_AUTO_CONNECT));
            break;

        case PROP_INTERFACE_SCREEN_BLANK:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->interfaceSettings,
                                                       SETTING_INTERFACE_SCREEN_BLANK));
            break;

        case PROP_CAPTURE_LAST_SESSION:
            dir = g_settings_get_string(priv->captureSettings,
                                        SETTING_CAPTURE_LAST_SESSION);
            if (!dir)
                dir = entangle_find_picture_dir();
            g_value_set_string(value, dir);
            break;

        case PROP_CAPTURE_FILENAME_PATTERN:
            file = g_settings_get_string(priv->captureSettings,
                                         SETTING_CAPTURE_FILENAME_PATTERN);
            g_value_set_string(value, file);
            g_free(file);
            break;

        case PROP_CAPTURE_CONTINUOUS_PREVIEW:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->captureSettings,
                                                       SETTING_CAPTURE_CONTINUOUS_PREVIEW));
            break;

        case PROP_CAPTURE_DELETE_FILE:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->captureSettings,
                                                       SETTING_CAPTURE_DELETE_FILE));
            break;

        case PROP_CMS_ENABLED:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->cmsSettings,
                                                       SETTING_CMS_ENABLED));
            break;

        case PROP_CMS_RGB_PROFILE:
            file = g_settings_get_string(priv->cmsSettings,
                                         SETTING_CMS_RGB_PROFILE);
            if (!file)
                file = entangle_find_srgb_profile();
            prof = entangle_colour_profile_new_file(file);
            g_value_set_object(value, prof);
            g_object_unref(prof);
            g_free(file);
            break;

        case PROP_CMS_MONITOR_PROFILE:
            file = g_settings_get_string(priv->cmsSettings,
                                         SETTING_CMS_MONITOR_PROFILE);
            if (file)
                prof = entangle_colour_profile_new_file(file);
            else
                prof = NULL;
            g_value_set_object(value, prof);
            g_object_unref(prof);
            g_free(file);
            break;

        case PROP_CMS_DETECT_SYSTEM_PROFILE:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->cmsSettings,
                                                       SETTING_CMS_DETECT_SYSTEM_PROFILE));
            break;

        case PROP_CMS_RENDERING_INTENT:
            g_value_set_enum(value,
                             g_settings_get_enum(priv->cmsSettings,
                                                 SETTING_CMS_RENDERING_INTENT));
            break;

        case PROP_IMG_MASK_ENABLED:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->imgSettings,
                                                       SETTING_IMG_MASK_ENABLED));
            break;

        case PROP_IMG_ASPECT_RATIO:
            g_value_set_string(value,
                               g_settings_get_string(priv->imgSettings,
                                                     SETTING_IMG_ASPECT_RATIO));
            break;

        case PROP_IMG_MASK_OPACITY:
            g_value_set_int(value,
                            g_settings_get_int(priv->imgSettings,
                                               SETTING_IMG_MASK_OPACITY));
            break;

        case PROP_IMG_FOCUS_POINT:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->imgSettings,
                                                       SETTING_IMG_FOCUS_POINT));
            break;

        case PROP_IMG_GRID_LINES:
            g_value_set_int(value,
                            g_settings_get_enum(priv->imgSettings,
                                                SETTING_IMG_GRID_LINES));
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
    EntangleColourProfile *prof;

    switch (prop_id)
        {
        case PROP_INTERFACE_AUTO_CONNECT:
            g_settings_set_boolean(priv->interfaceSettings,
                                   SETTING_INTERFACE_AUTO_CONNECT,
                                   g_value_get_boolean(value));
            break;

        case PROP_INTERFACE_SCREEN_BLANK:
            g_settings_set_boolean(priv->interfaceSettings,
                                   SETTING_INTERFACE_SCREEN_BLANK,
                                   g_value_get_boolean(value));
            break;

        case PROP_CAPTURE_LAST_SESSION:
            g_settings_set_string(priv->captureSettings,
                                  SETTING_CAPTURE_LAST_SESSION,
                                  g_value_get_string(value));
            break;

        case PROP_CAPTURE_FILENAME_PATTERN:
            g_settings_set_string(priv->captureSettings,
                                  SETTING_CAPTURE_FILENAME_PATTERN,
                                  g_value_get_string(value));
            break;

        case PROP_CAPTURE_CONTINUOUS_PREVIEW:
            g_settings_set_boolean(priv->captureSettings,
                                   SETTING_CAPTURE_CONTINUOUS_PREVIEW,
                                   g_value_get_boolean(value));
            break;

        case PROP_CAPTURE_DELETE_FILE:
            g_settings_set_boolean(priv->captureSettings,
                                   SETTING_CAPTURE_LAST_SESSION,
                                   g_value_get_boolean(value));
            break;

        case PROP_CMS_ENABLED:
            g_settings_set_boolean(priv->cmsSettings,
                                   SETTING_CMS_ENABLED,
                                   g_value_get_boolean(value));
            break;

        case PROP_CMS_RGB_PROFILE:
            prof = g_value_get_object(value);
            if (prof)
                g_settings_set_string(priv->cmsSettings,
                                      SETTING_CMS_RGB_PROFILE,
                                      entangle_colour_profile_filename(prof));
            else
                g_settings_set_string(priv->cmsSettings,
                                      SETTING_CMS_RGB_PROFILE,
                                      NULL);
            break;

        case PROP_CMS_MONITOR_PROFILE:
            prof = g_value_get_object(value);
            if (prof)
                g_settings_set_string(priv->cmsSettings,
                                      SETTING_CMS_MONITOR_PROFILE,
                                      entangle_colour_profile_filename(prof));
            else
                g_settings_set_string(priv->cmsSettings,
                                      SETTING_CMS_MONITOR_PROFILE,
                                      NULL);
            break;

        case PROP_CMS_DETECT_SYSTEM_PROFILE:
            g_settings_set_boolean(priv->cmsSettings,
                                   SETTING_CMS_DETECT_SYSTEM_PROFILE,
                                   g_value_get_boolean(value));
            break;

        case PROP_CMS_RENDERING_INTENT:
            g_settings_set_enum(priv->cmsSettings,
                                SETTING_CMS_RENDERING_INTENT,
                                g_value_get_enum(value));
            break;

        case PROP_IMG_MASK_ENABLED:
            g_settings_set_boolean(priv->imgSettings,
                                   SETTING_IMG_MASK_ENABLED,
                                   g_value_get_boolean(value));
            break;

        case PROP_IMG_ASPECT_RATIO:
            g_settings_set_string(priv->imgSettings,
                                  SETTING_IMG_ASPECT_RATIO,
                                  g_value_get_string(value));
            break;

        case PROP_IMG_MASK_OPACITY:
            g_settings_set_int(priv->imgSettings,
                               SETTING_IMG_MASK_OPACITY,
                               g_value_get_int(value));
            break;

        case PROP_IMG_FOCUS_POINT:
            g_settings_set_boolean(priv->imgSettings,
                                   SETTING_IMG_FOCUS_POINT,
                                   g_value_get_boolean(value));
            break;

        case PROP_IMG_GRID_LINES:
            g_settings_set_enum(priv->imgSettings,
                                SETTING_IMG_GRID_LINES,
                                g_value_get_int(value));
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

    g_object_unref(priv->interfaceSettings);
    g_object_unref(priv->captureSettings);
    g_object_unref(priv->cmsSettings);
    g_object_unref(priv->imgSettings);

    G_OBJECT_CLASS(entangle_preferences_parent_class)->finalize(object);
}


static void entangle_preferences_class_init(EntanglePreferencesClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_preferences_finalize;
    object_class->get_property = entangle_preferences_get_property;
    object_class->set_property = entangle_preferences_set_property;


    g_object_class_install_property(object_class,
                                    PROP_INTERFACE_AUTO_CONNECT,
                                    g_param_spec_boolean(PROP_NAME_INTERFACE_AUTO_CONNECT,
                                                         "Auto connect",
                                                         "Automatically connect to cameras at startup",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_INTERFACE_SCREEN_BLANK,
                                    g_param_spec_boolean(PROP_NAME_INTERFACE_SCREEN_BLANK,
                                                         "Screen blank",
                                                         "Blank screen while capturing images",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CAPTURE_LAST_SESSION,
                                    g_param_spec_string(PROP_NAME_CAPTURE_LAST_SESSION,
                                                        "Pictures directory",
                                                        "Directory to store pictures in",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CAPTURE_FILENAME_PATTERN,
                                    g_param_spec_string(PROP_NAME_CAPTURE_FILENAME_PATTERN,
                                                        "Filename pattern",
                                                        "Pattern for creating new filenames",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CAPTURE_DELETE_FILE,
                                    g_param_spec_boolean(PROP_NAME_CAPTURE_DELETE_FILE,
                                                         "Delete file",
                                                         "Delete file after capturing",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CAPTURE_CONTINUOUS_PREVIEW,
                                    g_param_spec_boolean(PROP_NAME_CAPTURE_CONTINUOUS_PREVIEW,
                                                         "Continuous preview",
                                                         "Continue preview after capturing",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CMS_ENABLED,
                                    g_param_spec_boolean(PROP_NAME_CMS_ENABLED,
                                                         "Colour managed display",
                                                         "Whether to enable colour management on display",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CMS_RGB_PROFILE,
                                    g_param_spec_object(PROP_NAME_CMS_RGB_PROFILE,
                                                        "RGB Profile",
                                                        "Colour profile for workspace",
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CMS_MONITOR_PROFILE,
                                    g_param_spec_object(PROP_NAME_CMS_MONITOR_PROFILE,
                                                        "Monitor profile",
                                                        "Colour profile for monitor",
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CMS_DETECT_SYSTEM_PROFILE,
                                    g_param_spec_boolean(PROP_NAME_CMS_DETECT_SYSTEM_PROFILE,
                                                         "Detect system profile",
                                                         "Detect the monitor colour profile",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CMS_RENDERING_INTENT,
                                    g_param_spec_enum(PROP_NAME_CMS_RENDERING_INTENT,
                                                      "Profile rendering intent",
                                                      "Rendering intent for images",
                                                      ENTANGLE_TYPE_COLOUR_PROFILE_INTENT,
                                                      ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_ASPECT_RATIO,
                                    g_param_spec_string(PROP_NAME_IMG_ASPECT_RATIO,
                                                        "Aspect ratio",
                                                        "Image mask aspect ratio",
                                                        "1.33",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_MASK_OPACITY,
                                    g_param_spec_int(PROP_NAME_IMG_MASK_OPACITY,
                                                     "Mask opacity",
                                                     "Image mask border opacity",
                                                     0,
                                                     100,
                                                     90,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_MASK_ENABLED,
                                    g_param_spec_boolean(PROP_NAME_IMG_MASK_ENABLED,
                                                         "Mask enabled",
                                                         "Enable aspect ratio image mask",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_FOCUS_POINT,
                                    g_param_spec_boolean(PROP_NAME_IMG_FOCUS_POINT,
                                                         "Focus point",
                                                         "Focus point during preview",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_GRID_LINES,
                                    g_param_spec_int(PROP_NAME_IMG_GRID_LINES,
                                                     "Grid lines",
                                                     "Grid lines during preview",
                                                     0, 4, 4,
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


static void entangle_preferences_ensure_data_dir(void)
{
    const char *const *dirs;
    const char *dir;

    for (dirs = g_get_system_data_dirs (); *dirs; dirs++) {
        if (g_str_equal(*dirs, DATADIR)) {
            ENTANGLE_DEBUG("Found dir %s in system search dirs", *dirs);
            return;
        }
    }

    if ((dir = getenv("GSETTINGS_SCHEMA_DIR")) &&
        g_str_equal(dir, DATADIR "/glib-2.0/schemas")) {
        ENTANGLE_DEBUG("Found dir %s in GSETTINGS_SCHEMA_DIR", dir);
        return;
    }

    ENTANGLE_DEBUG("Setting %s in GSETTINGS_SCHEMA_DIR", DATADIR "/glib-2.0/schemas");
    setenv("GSETTINGS_SCHEMA_DIR", DATADIR "/glib-2.0/schemas", 1);
}

static void entangle_preferences_init(EntanglePreferences *picker)
{
    EntanglePreferencesPrivate *priv;
    GSettings *settings;

    priv = picker->priv = ENTANGLE_PREFERENCES_GET_PRIVATE(picker);

    entangle_preferences_ensure_data_dir();

    settings = g_settings_new("org.entangle-photo.manager");
    priv->interfaceSettings = g_settings_get_child(settings,
                                                   SETTING_INTERFACE);
    priv->captureSettings = g_settings_get_child(settings,
                                                SETTING_CAPTURE);
    priv->cmsSettings = g_settings_get_child(settings,
                                             SETTING_CMS);
    priv->imgSettings = g_settings_get_child(settings,
                                             SETTING_IMG);
    g_object_unref(settings);
}


gboolean entangle_preferences_interface_get_auto_connect(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    return g_settings_get_boolean(priv->interfaceSettings,
                                  SETTING_INTERFACE_AUTO_CONNECT);
}


void entangle_preferences_interface_set_auto_connect(EntanglePreferences *prefs, gboolean autoconn)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    g_settings_set_boolean(priv->interfaceSettings,
                           SETTING_INTERFACE_AUTO_CONNECT, autoconn);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_INTERFACE_AUTO_CONNECT);
}


gboolean entangle_preferences_interface_get_screen_blank(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    return g_settings_get_boolean(priv->interfaceSettings,
                                  SETTING_INTERFACE_SCREEN_BLANK);
}


void entangle_preferences_interface_set_screen_blank(EntanglePreferences *prefs, gboolean blank)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    g_settings_set_boolean(priv->interfaceSettings,
                           SETTING_INTERFACE_SCREEN_BLANK, blank);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_INTERFACE_SCREEN_BLANK);
}


gchar **entangle_preferences_interface_get_plugins(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    return g_settings_get_strv(priv->interfaceSettings,
                               SETTING_INTERFACE_PLUGINS);
}


void entangle_preferences_interface_add_plugin(EntanglePreferences *prefs, const char *name)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    gchar **plugins = g_settings_get_strv(priv->interfaceSettings,
                                          SETTING_INTERFACE_PLUGINS);
    gsize len = g_strv_length(plugins);

    plugins = g_renew(gchar *, plugins, len + 1);
    plugins[len] = g_strdup(name);
    plugins[len+1] = NULL;
    g_settings_set_strv(priv->interfaceSettings,
                        SETTING_INTERFACE_PLUGINS,
                        (const gchar *const*)plugins);
    g_strfreev(plugins);
}


void entangle_preferences_interface_remove_plugin(EntanglePreferences *prefs, const char *name)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    gchar **plugins = g_settings_get_strv(priv->interfaceSettings,
                                          SETTING_INTERFACE_PLUGINS);
    gsize len = g_strv_length(plugins);
    gsize i;

    for (i = 0 ; i < len ; i++) {
        if (g_str_equal(plugins[i], name)) {
            g_free(plugins[i]);
            plugins[i] = NULL;
            if (i < (len - 1)) {
                memmove(plugins + i,
                        plugins + i + 1,
                        len - i - 1);
                plugins[len-1] = NULL;
            }
            break;
        }
    }
    g_settings_set_strv(priv->interfaceSettings,
                        SETTING_INTERFACE_PLUGINS,
                        (const gchar *const*)plugins);
    g_strfreev(plugins);
}


char *entangle_preferences_capture_get_last_session(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    char *dir = g_settings_get_string(priv->captureSettings,
                                      SETTING_CAPTURE_LAST_SESSION);
    if (dir && g_str_equal(dir, "")) {
        g_free(dir);
        dir = NULL;
    }
    if (!dir)
        dir = entangle_find_picture_dir();
    return dir;
}


void entangle_preferences_capture_set_last_session(EntanglePreferences *prefs, const gchar *dir)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->captureSettings,
                          SETTING_CAPTURE_LAST_SESSION, dir);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_LAST_SESSION);
}


char *entangle_preferences_capture_get_filename_pattern(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_string(priv->captureSettings,
                                 SETTING_CAPTURE_FILENAME_PATTERN);
}


void entangle_preferences_capture_set_filename_pattern(EntanglePreferences *prefs, const gchar *dir)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->captureSettings,
                          SETTING_CAPTURE_FILENAME_PATTERN, dir);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_FILENAME_PATTERN);
}


gboolean entangle_preferences_capture_get_continuous_preview(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->captureSettings,
                                  SETTING_CAPTURE_CONTINUOUS_PREVIEW);
}


void entangle_preferences_capture_set_continuous_preview(EntanglePreferences *prefs, gboolean enabled)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->captureSettings,
                           SETTING_CAPTURE_CONTINUOUS_PREVIEW, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_CONTINUOUS_PREVIEW);
}


gboolean entangle_preferences_capture_get_delete_file(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->captureSettings,
                                  SETTING_CAPTURE_DELETE_FILE);
}


void entangle_preferences_capture_set_delete_file(EntanglePreferences *prefs, gboolean enabled)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->captureSettings,
                           SETTING_CAPTURE_DELETE_FILE, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_DELETE_FILE);
}


EntangleColourProfile *entangle_preferences_cms_get_rgb_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    EntangleColourProfile *prof;

    prof = entangle_colour_profile_new_file(g_settings_get_string(priv->cmsSettings,
                                                                  SETTING_CMS_RGB_PROFILE));

    return prof;
}


void entangle_preferences_cms_set_rgb_profile(EntanglePreferences *prefs,
                                              EntangleColourProfile *prof)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->cmsSettings,
                          SETTING_CMS_RGB_PROFILE,
                          prof ? entangle_colour_profile_filename(prof) : NULL);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_RGB_PROFILE);
}


EntangleColourProfile *entangle_preferences_cms_get_monitor_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    EntangleColourProfile *prof;

    prof = entangle_colour_profile_new_file(g_settings_get_string(priv->cmsSettings,
                                                                  SETTING_CMS_MONITOR_PROFILE));

    return prof;
}


void entangle_preferences_cms_set_monitor_profile(EntanglePreferences *prefs,
                                                  EntangleColourProfile *prof)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->cmsSettings,
                          SETTING_CMS_MONITOR_PROFILE,
                          prof ? entangle_colour_profile_filename(prof) : NULL);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_MONITOR_PROFILE);
}


gboolean entangle_preferences_cms_get_enabled(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->cmsSettings,
                                  SETTING_CMS_ENABLED);
}


void entangle_preferences_cms_set_enabled(EntanglePreferences *prefs,
                                          gboolean enabled)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->cmsSettings,
                           SETTING_CMS_ENABLED, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_ENABLED);
}


gboolean entangle_preferences_cms_get_detect_system_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->cmsSettings,
                                  SETTING_CMS_DETECT_SYSTEM_PROFILE);
}


void entangle_preferences_cms_set_detect_system_profile(EntanglePreferences *prefs,
                                                        gboolean enabled)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->cmsSettings,
                           SETTING_CMS_DETECT_SYSTEM_PROFILE, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_DETECT_SYSTEM_PROFILE);
}


EntangleColourProfileIntent entangle_preferences_cms_get_rendering_intent(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_enum(priv->cmsSettings,
                               SETTING_CMS_RENDERING_INTENT);
}


void entangle_preferences_cms_set_rendering_intent(EntanglePreferences *prefs,
                                                   EntangleColourProfileIntent intent)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_enum(priv->cmsSettings,
                        SETTING_CMS_RENDERING_INTENT, intent);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_RENDERING_INTENT);
}


gchar *entangle_preferences_img_get_aspect_ratio(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_string(priv->imgSettings,
                                 SETTING_IMG_ASPECT_RATIO);
}


void entangle_preferences_img_set_aspect_ratio(EntanglePreferences *prefs,
                                               const gchar *aspect)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->imgSettings,
                          SETTING_IMG_ASPECT_RATIO, aspect);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_ASPECT_RATIO);
}

gint entangle_preferences_img_get_mask_opacity(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_int(priv->imgSettings,
                              SETTING_IMG_MASK_OPACITY);
}


void entangle_preferences_img_set_mask_opacity(EntanglePreferences *prefs,
                                               gint mask)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_int(priv->imgSettings,
                       SETTING_IMG_MASK_OPACITY, mask);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_MASK_OPACITY);
}


gboolean entangle_preferences_img_get_mask_enabled(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->imgSettings,
                                  SETTING_IMG_MASK_ENABLED);
}


void entangle_preferences_img_set_mask_enabled(EntanglePreferences *prefs, gboolean enabled)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->imgSettings,
                           SETTING_IMG_MASK_ENABLED, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_MASK_ENABLED);
}


gboolean entangle_preferences_img_get_focus_point(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->imgSettings,
                                  SETTING_IMG_FOCUS_POINT);
}


void entangle_preferences_img_set_focus_point(EntanglePreferences *prefs, gboolean enabled)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->imgSettings,
                           SETTING_IMG_FOCUS_POINT, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_FOCUS_POINT);
}


gint entangle_preferences_img_get_grid_lines(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_enum(priv->imgSettings,
                               SETTING_IMG_GRID_LINES);
}


void entangle_preferences_img_set_grid_lines(EntanglePreferences *prefs, gint gridLines)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_enum(priv->imgSettings,
                        SETTING_IMG_GRID_LINES, gridLines);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_GRID_LINES);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
