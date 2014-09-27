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
#define SETTING_INTERFACE_HISTOGRAM_LINEAR "histogram-linear"

#define SETTING_CAPTURE_FILENAME_PATTERN   "filename-pattern"
#define SETTING_CAPTURE_LAST_SESSION       "last-session"
#define SETTING_CAPTURE_CONTINUOUS_PREVIEW "continuous-preview"
#define SETTING_CAPTURE_ELECTRONIC_SHUTTER "electronic-shutter"
#define SETTING_CAPTURE_DELETE_FILE        "delete-file"
#define SETTING_CAPTURE_SYNC_CLOCK         "sync-clock"

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
#define SETTING_IMG_EMBEDDED_PREVIEW       "embedded-preview"
#define SETTING_IMG_ONION_SKIN             "onion-skin"
#define SETTING_IMG_ONION_LAYERS           "onion-layers"
#define SETTING_IMG_BACKGROUND             "background"
#define SETTING_IMG_HIGHLIGHT              "highlight"


#define PROP_NAME_INTERFACE_AUTO_CONNECT     SETTING_INTERFACE "-" SETTING_INTERFACE_AUTO_CONNECT
#define PROP_NAME_INTERFACE_SCREEN_BLANK     SETTING_INTERFACE "-" SETTING_INTERFACE_SCREEN_BLANK
#define PROP_NAME_INTERFACE_HISTOGRAM_LINEAR SETTING_INTERFACE "-" SETTING_INTERFACE_HISTOGRAM_LINEAR

#define PROP_NAME_CAPTURE_FILENAME_PATTERN   SETTING_CAPTURE "-" SETTING_CAPTURE_FILENAME_PATTERN
#define PROP_NAME_CAPTURE_LAST_SESSION       SETTING_CAPTURE "-" SETTING_CAPTURE_LAST_SESSION
#define PROP_NAME_CAPTURE_CONTINUOUS_PREVIEW SETTING_CAPTURE "-" SETTING_CAPTURE_CONTINUOUS_PREVIEW
#define PROP_NAME_CAPTURE_ELECTRONIC_SHUTTER SETTING_CAPTURE "-" SETTING_CAPTURE_ELECTRONIC_SHUTTER
#define PROP_NAME_CAPTURE_DELETE_FILE        SETTING_CAPTURE "-" SETTING_CAPTURE_DELETE_FILE
#define PROP_NAME_CAPTURE_SYNC_CLOCK         SETTING_CAPTURE "-" SETTING_CAPTURE_SYNC_CLOCK

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
#define PROP_NAME_IMG_EMBEDDED_PREVIEW       SETTING_IMG "-" SETTING_IMG_EMBEDDED_PREVIEW
#define PROP_NAME_IMG_ONION_LAYERS           SETTING_IMG "-" SETTING_IMG_ONION_LAYERS
#define PROP_NAME_IMG_ONION_SKIN             SETTING_IMG "-" SETTING_IMG_ONION_SKIN
#define PROP_NAME_IMG_BACKGROUND             SETTING_IMG "-" SETTING_IMG_BACKGROUND
#define PROP_NAME_IMG_HIGHLIGHT              SETTING_IMG "-" SETTING_IMG_HIGHLIGHT

enum {
    PROP_0,

    PROP_INTERFACE_AUTO_CONNECT,
    PROP_INTERFACE_SCREEN_BLANK,
    PROP_INTERFACE_HISTOGRAM_LINEAR,

    PROP_CAPTURE_FILENAME_PATTERN,
    PROP_CAPTURE_LAST_SESSION,
    PROP_CAPTURE_CONTINUOUS_PREVIEW,
    PROP_CAPTURE_ELECTRONIC_SHUTTER,
    PROP_CAPTURE_DELETE_FILE,
    PROP_CAPTURE_SYNC_CLOCK,

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
    PROP_IMG_EMBEDDED_PREVIEW,
    PROP_IMG_ONION_SKIN,
    PROP_IMG_ONION_LAYERS,
    PROP_IMG_BACKGROUND,
    PROP_IMG_HIGHLIGHT,
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

        case PROP_INTERFACE_HISTOGRAM_LINEAR:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->interfaceSettings,
                                                       SETTING_INTERFACE_HISTOGRAM_LINEAR));
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

        case PROP_CAPTURE_ELECTRONIC_SHUTTER:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->captureSettings,
                                                       SETTING_CAPTURE_ELECTRONIC_SHUTTER));
            break;

        case PROP_CAPTURE_DELETE_FILE:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->captureSettings,
                                                       SETTING_CAPTURE_DELETE_FILE));
            break;

        case PROP_CAPTURE_SYNC_CLOCK:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->captureSettings,
                                                       SETTING_CAPTURE_SYNC_CLOCK));
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

        case PROP_IMG_EMBEDDED_PREVIEW:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->imgSettings,
                                                       SETTING_IMG_EMBEDDED_PREVIEW));
            break;

        case PROP_IMG_ONION_SKIN:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->imgSettings,
                                                       SETTING_IMG_ONION_SKIN));
            break;

        case PROP_IMG_ONION_LAYERS:
            g_value_set_int(value,
                            g_settings_get_int(priv->imgSettings,
                                               SETTING_IMG_ONION_LAYERS));
            break;

        case PROP_IMG_BACKGROUND:
            g_value_set_string(value,
                               g_settings_get_string(priv->imgSettings,
                                                     SETTING_IMG_BACKGROUND));
            break;

        case PROP_IMG_HIGHLIGHT:
            g_value_set_string(value,
                               g_settings_get_string(priv->imgSettings,
                                                     SETTING_IMG_HIGHLIGHT));
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

        case PROP_INTERFACE_HISTOGRAM_LINEAR:
            g_settings_set_boolean(priv->interfaceSettings,
                                   SETTING_INTERFACE_HISTOGRAM_LINEAR,
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

        case PROP_CAPTURE_ELECTRONIC_SHUTTER:
            g_settings_set_boolean(priv->captureSettings,
                                   SETTING_CAPTURE_ELECTRONIC_SHUTTER,
                                   g_value_get_boolean(value));
            break;

        case PROP_CAPTURE_DELETE_FILE:
            g_settings_set_boolean(priv->captureSettings,
                                   SETTING_CAPTURE_DELETE_FILE,
                                   g_value_get_boolean(value));
            break;

        case PROP_CAPTURE_SYNC_CLOCK:
            g_settings_set_boolean(priv->captureSettings,
                                   SETTING_CAPTURE_SYNC_CLOCK,
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

        case PROP_IMG_EMBEDDED_PREVIEW:
            g_settings_set_boolean(priv->imgSettings,
                                   SETTING_IMG_EMBEDDED_PREVIEW,
                                   g_value_get_boolean(value));
            break;

        case PROP_IMG_ONION_SKIN:
            g_settings_set_boolean(priv->imgSettings,
                                   SETTING_IMG_ONION_SKIN,
                                   g_value_get_boolean(value));
            break;

        case PROP_IMG_ONION_LAYERS:
            g_settings_set_int(priv->imgSettings,
                               SETTING_IMG_ONION_LAYERS,
                               g_value_get_int(value));
            break;

        case PROP_IMG_BACKGROUND:
            g_settings_set_string(priv->imgSettings,
                                  SETTING_IMG_BACKGROUND,
                                  g_value_get_string(value));
            break;

        case PROP_IMG_HIGHLIGHT:
            g_settings_set_string(priv->imgSettings,
                                  SETTING_IMG_HIGHLIGHT,
                                  g_value_get_string(value));
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
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

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
                                    PROP_INTERFACE_HISTOGRAM_LINEAR,
                                    g_param_spec_boolean(PROP_NAME_INTERFACE_HISTOGRAM_LINEAR,
                                                         "Linear histogram",
                                                         "Use linear histogram",
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
                                    PROP_CAPTURE_SYNC_CLOCK,
                                    g_param_spec_boolean(PROP_NAME_CAPTURE_SYNC_CLOCK,
                                                         "Sync clock",
                                                         "Synchronize clock automatically",
                                                         FALSE,
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
                                    PROP_CAPTURE_ELECTRONIC_SHUTTER,
                                    g_param_spec_boolean(PROP_NAME_CAPTURE_ELECTRONIC_SHUTTER,
                                                         "Electronic shutter",
                                                         "Use preview output as capture image",
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

    g_object_class_install_property(object_class,
                                    PROP_IMG_EMBEDDED_PREVIEW,
                                    g_param_spec_boolean(PROP_NAME_IMG_EMBEDDED_PREVIEW,
                                                         "Embedded preview",
                                                         "Embedded preview for raw files",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_ONION_LAYERS,
                                    g_param_spec_int(PROP_NAME_IMG_ONION_LAYERS,
                                                     "Onion layer count",
                                                     "Overlay layers in image display",
                                                     1,
                                                     5,
                                                     3,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_ONION_SKIN,
                                    g_param_spec_boolean(PROP_NAME_IMG_ONION_SKIN,
                                                         "Onion skin",
                                                         "Enable image overlay display",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_BACKGROUND,
                                    g_param_spec_string(PROP_NAME_IMG_BACKGROUND,
                                                        "Image background color",
                                                        "Image background color",
                                                        "#000000",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_IMG_HIGHLIGHT,
                                    g_param_spec_string(PROP_NAME_IMG_HIGHLIGHT,
                                                        "Image highlight color",
                                                        "Image highlight color",
                                                        "#FFFFFF",
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

    for (dirs = g_get_system_data_dirs(); *dirs; dirs++) {
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


/**
 * entangle_preferences_interface_get_auto_connect:
 * @prefs: (transfer none): the preferences store
 *
 * Determine whether the application will attempt to automatically
 * connect to cameras at startup
 *
 * Returns: TRUE if cameras will be automatically connected, FALSE otherwise
 */
gboolean entangle_preferences_interface_get_auto_connect(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;
    return g_settings_get_boolean(priv->interfaceSettings,
                                  SETTING_INTERFACE_AUTO_CONNECT);
}


/**
 * entangle_preferences_set_auto_connect:
 * @prefs: (transfer none): the preferences store
 * @autoconn: TRUE to turn on automatic connect of cameras
 *
 * If @autoconn is set to TRUE, the application will
 * attempt to automatically connect to cameras at startup
 */
void entangle_preferences_interface_set_auto_connect(EntanglePreferences *prefs, gboolean autoconn)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;
    g_settings_set_boolean(priv->interfaceSettings,
                           SETTING_INTERFACE_AUTO_CONNECT, autoconn);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_INTERFACE_AUTO_CONNECT);
}


/**
 * entangle_preferences_interface_get_screen_blank:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if the screen wil be blanked prior to capturing
 * the image
 *
 * Returns: TRUE if the screen will blank at time of capture
 */
gboolean entangle_preferences_interface_get_screen_blank(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;
    return g_settings_get_boolean(priv->interfaceSettings,
                                  SETTING_INTERFACE_SCREEN_BLANK);
}


/**
 * entangle_preferences_interface_set_screen_blank:
 * @prefs: (transfer none): the preferences store
 * @blank: TRUE to make the screen blank during capture
 *
 * If @blank is set to TRUE, the screen will be blanked
 * during capture to avoid light from the screen influencing
 * the scene being shot.
 */
void entangle_preferences_interface_set_screen_blank(EntanglePreferences *prefs, gboolean blank)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;
    g_settings_set_boolean(priv->interfaceSettings,
                           SETTING_INTERFACE_SCREEN_BLANK, blank);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_INTERFACE_SCREEN_BLANK);
}


/**
 * entangle_preferences_interface_get_histogram_linear:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if the histogram will be displayed in
 * linear or logarithmic mode
 *
 * Returns: TRUE if the histogram is linear, FALSE for logarithmic
 */
gboolean entangle_preferences_interface_get_histogram_linear(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->interfaceSettings,
                                  SETTING_INTERFACE_HISTOGRAM_LINEAR);
}


/**
 * entangle_preferences_interface_set_histogram_linear:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to set the histogram to linear mode
 *
 * If @enabled is TRUE then the histogram will be displayed in
 * linear mode, otherwise logarithmic mode will be used
 */
void entangle_preferences_interface_set_histogram_linear(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->interfaceSettings,
                           SETTING_INTERFACE_HISTOGRAM_LINEAR, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_INTERFACE_HISTOGRAM_LINEAR);
}


/**
 * entangle_preferences_interface_get_plugins:
 * @prefs: (transfer none): the preferences store
 *
 * Get the names of the plugins that are enabled for the
 * application
 *
 * Returns: (transfer full)(array zero-terminated=1): the list of plugin names, NULL terminated
 */
gchar **entangle_preferences_interface_get_plugins(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

    EntanglePreferencesPrivate *priv = prefs->priv;
    return g_settings_get_strv(priv->interfaceSettings,
                               SETTING_INTERFACE_PLUGINS);
}


/**
 * entangle_preferences_interface_add_plugin:
 * @prefs: (transfer none): the preferences store
 * @name: (transfer none): the plugin to add
 *
 * Add the plugin called @name to the active list
 */
void entangle_preferences_interface_add_plugin(EntanglePreferences *prefs, const char *name)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;
    gchar **plugins = g_settings_get_strv(priv->interfaceSettings,
                                          SETTING_INTERFACE_PLUGINS);
    gsize len = g_strv_length(plugins);
    plugins = g_renew(gchar *, plugins, len + 2);
    len++;
    plugins[len-1] = g_strdup(name);
    plugins[len] = NULL;
    g_settings_set_strv(priv->interfaceSettings,
                        SETTING_INTERFACE_PLUGINS,
                        (const gchar *const*)plugins);
    g_strfreev(plugins);
}


/**
 * entangle_preferences_interface_remove_plugin:
 * @prefs: (transfer none): the preferences store
 * @name: (transfer none): the plugin to remove
 *
 * Remove the plugin called @name from the active list
 */
void entangle_preferences_interface_remove_plugin(EntanglePreferences *prefs, const char *name)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;
    gchar **plugins = g_settings_get_strv(priv->interfaceSettings,
                                          SETTING_INTERFACE_PLUGINS);
    gsize len = g_strv_length(plugins);
    gsize i, j;
    gchar **newplugins = g_new0(gchar *, len + 1);
    for (i = 0, j = 0; i < len; i++) {
        if (!g_str_equal(plugins[i], name)) {
            newplugins[j++] = plugins[i];
        } else {
            g_free(plugins[i]);
        }
        plugins[i] = NULL;
    }
    newplugins[j] = NULL;
    g_settings_set_strv(priv->interfaceSettings,
                        SETTING_INTERFACE_PLUGINS,
                        (const gchar *const*)newplugins);
    g_strfreev(newplugins);
    g_strfreev(plugins);
}


/**
 * entangle_preferences_capture_get_last_session:
 * @prefs: (transfer none): the preferences store
 *
 * Get the most recently used directory for the
 * image session
 *
 * Returns: (transfer full): the session directory
 */
char *entangle_preferences_capture_get_last_session(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

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


/**
 * entangle_preferences_capture_set_last_session:
 * @prefs: (transfer none): the preferences store
 * @dir: (transfer none): the new session
 *
 * Record @dir as being the most recently used session
 * directory
 */
void entangle_preferences_capture_set_last_session(EntanglePreferences *prefs, const gchar *dir)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->captureSettings,
                          SETTING_CAPTURE_LAST_SESSION, dir);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_LAST_SESSION);
}


/**
 * entangle_preferences_capture_get_filename_pattern:
 * @prefs: (transfer none): the preferences store
 *
 * Get the filename pattern to use for naming images that
 * are saved after download. In the pattern any repeated
 * sequence of 'X' characters will be replaced with the
 * image sequence number
 *
 * Returns: (transfer full): the filename pattern.
 */
char *entangle_preferences_capture_get_filename_pattern(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_string(priv->captureSettings,
                                 SETTING_CAPTURE_FILENAME_PATTERN);
}


/**
 * entangle_preferences_capture_set_filename_pattern:
 * @prefs: (transfer none): the preferences store
 * @pattern: (transfer none): the filename pattern
 *
 * Set the pattern to use for naming images that are
 * saved after download.
 */
void entangle_preferences_capture_set_filename_pattern(EntanglePreferences *prefs, const gchar *pattern)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->captureSettings,
                          SETTING_CAPTURE_FILENAME_PATTERN, pattern);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_FILENAME_PATTERN);
}


/**
 * entangle_preferences_capture_get_continuous_preview:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if the preview mode will continue to operate after each
 * image is captured.
 *
 * Returns: TRUE if preview mode will continue after capture, FALSE otherwise
 */
gboolean entangle_preferences_capture_get_continuous_preview(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->captureSettings,
                                  SETTING_CAPTURE_CONTINUOUS_PREVIEW);
}


/**
 * entangle_preferences_capture_set_continuous_preview:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to make preview mode continue after capture
 *
 * If @enabled is set to TRUE, then the preview mode will
 * continue to run after each image is captured
 */
void entangle_preferences_capture_set_continuous_preview(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->captureSettings,
                           SETTING_CAPTURE_CONTINUOUS_PREVIEW, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_CONTINUOUS_PREVIEW);
}


/**
 * entangle_preferences_capture_get_electronic_shutter:
 * @prefs: (transfer none): the preferences store
 *
 * Determines if the capture event will use the next preview output as
 * capture image, instead of triggering a normal capture, with shutter
 * mechanically moving.
 *
 * Returns: TRUE if no mechanical shutter action occurs on a capture, FALSE otherwise
 */
gboolean entangle_preferences_capture_get_electronic_shutter(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->captureSettings,
                                  SETTING_CAPTURE_ELECTRONIC_SHUTTER);
}


/**
 * entangle_preferences_capture_set_electronic_shutter:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE if no mechanical shutter action should occur on capture
 *
 * If @enabled is TRUE, then the camera shutter will not be activated
 * when capturing an image.
 */
void entangle_preferences_capture_set_electronic_shutter(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->captureSettings,
                           SETTING_CAPTURE_ELECTRONIC_SHUTTER, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_ELECTRONIC_SHUTTER);
}


/**
 * entangle_preferences_capture_get_delete_file:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if the file will be deleted from the camera after it
 * has been downloaded.
 *
 * Returns: TRUE if the files will be deleted after download
 */
gboolean entangle_preferences_capture_get_delete_file(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->captureSettings,
                                  SETTING_CAPTURE_DELETE_FILE);
}


/**
 * entangle_preferences_capture_set_delete_file:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE if files should be deleted from camera after capture
 *
 * If @enabled is set to TRUE, then files will be deleted from the
 * camera after they are downloaded.
 */
void entangle_preferences_capture_set_delete_file(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->captureSettings,
                           SETTING_CAPTURE_DELETE_FILE, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_DELETE_FILE);
}


/**
 * entangle_preferences_capture_get_sync_clock:
 * @prefs: (transfer none): the preferences store
 *
 * Determines if the camera clock will be automatically synchronized
 * to the host computer clock when first connecting
 *
 * Returns: TRUE if the clock will synchronized, FALSE otherwise
 */
gboolean entangle_preferences_capture_get_sync_clock(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->captureSettings,
                                  SETTING_CAPTURE_SYNC_CLOCK);
}


/**
 * entangle_preferences_capture_set_sync_clock:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE if the clock should be synchronized
 *
 * If @enabled is TRUE, then the camera clock will be synchronized to
 * the host computer clock when first connecting
 */
void entangle_preferences_capture_set_sync_clock(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->captureSettings,
                           SETTING_CAPTURE_SYNC_CLOCK, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CAPTURE_SYNC_CLOCK);
}


/**
 * entangle_preferences_cms_get_rgb_profile:
 * @prefs: (transfer none): the preferences store
 *
 * Get the colour profile that represents the RGB working space
 *
 * Returns: (transfer full): the colour space
 */
EntangleColourProfile *entangle_preferences_cms_get_rgb_profile(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

    EntanglePreferencesPrivate *priv = prefs->priv;
    EntangleColourProfile *prof;

    prof = entangle_colour_profile_new_file(g_settings_get_string(priv->cmsSettings,
                                                                  SETTING_CMS_RGB_PROFILE));

    return prof;
}


/**
 * entangle_preferences_cms_set_rgb_profile:
 * @prefs: (transfer none): the preferences store
 * @prof: (transfer none)(allow-none): the new rgb profile
 *
 * Set the colour profile that represents the RGB working space
 */
void entangle_preferences_cms_set_rgb_profile(EntanglePreferences *prefs,
                                              EntangleColourProfile *prof)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->cmsSettings,
                          SETTING_CMS_RGB_PROFILE,
                          prof ? entangle_colour_profile_filename(prof) : NULL);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_RGB_PROFILE);
}


/**
 * entangle_preferences_cms_get_monitor_profile:
 * @prefs: (transfer none): the preferences store
 *
 * Get the profile associated with the display, if any.
 *
 * Returns: (transfer full): the colour profile
 */
EntangleColourProfile *entangle_preferences_cms_get_monitor_profile(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

    EntanglePreferencesPrivate *priv = prefs->priv;
    EntangleColourProfile *prof;

    prof = entangle_colour_profile_new_file(g_settings_get_string(priv->cmsSettings,
                                                                  SETTING_CMS_MONITOR_PROFILE));

    return prof;
}


/**
 * entangle_preferences_cms_set_monitor_profile:
 * @prefs: (transfer none): the preferences store
 * @prof: (transfer none)(allow-none): the new monitor profile
 *
 * If @prof is not NULL, it will be used as the current monitor
 * profile. This is only honoured if automatic detection of
 * the monitor profile is disabled.
 */
void entangle_preferences_cms_set_monitor_profile(EntanglePreferences *prefs,
                                                  EntangleColourProfile *prof)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->cmsSettings,
                          SETTING_CMS_MONITOR_PROFILE,
                          prof ? entangle_colour_profile_filename(prof) : NULL);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_MONITOR_PROFILE);
}


/**
 * entangle_preferences_cms_get_enabled:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if images should be displayed with colour management
 * applied
 *
 * Returns: TRUE if colour management is active
 */
gboolean entangle_preferences_cms_get_enabled(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->cmsSettings,
                                  SETTING_CMS_ENABLED);
}


/**
 * entangle_preferences_cms_set_enabled:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to enable colour management
 *
 * If @enabled is TRUE, colour management will be applied when
 * displaying images
 */
void entangle_preferences_cms_set_enabled(EntanglePreferences *prefs,
                                          gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->cmsSettings,
                           SETTING_CMS_ENABLED, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_ENABLED);
}


/**
 * entangle_preferences_cms_get_detect_system_profile:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if the monitor profile will be automatically
 * detected, which is usually preferrable since
 * it copes with multiple monitor scenarios
 */
gboolean entangle_preferences_cms_get_detect_system_profile(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->cmsSettings,
                                  SETTING_CMS_DETECT_SYSTEM_PROFILE);
}


/**
 * entangle_preferences_cms_set_detect_system_profile:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to automatically detect the monitor profile
 *
 * If @enabled is TRUE then the monitor profile will be
 * automatically detected
 */
void entangle_preferences_cms_set_detect_system_profile(EntanglePreferences *prefs,
                                                        gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->cmsSettings,
                           SETTING_CMS_DETECT_SYSTEM_PROFILE, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_DETECT_SYSTEM_PROFILE);
}


/**
 * entangle_preferences_cms_get_rendering_intent:
 * @prefs: (transfer none): the preferences store
 *
 * Determine the rendering intent for displaying images
 *
 * Returns: the image rendering intent
 */
EntangleColourProfileIntent entangle_preferences_cms_get_rendering_intent(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), 0);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_enum(priv->cmsSettings,
                               SETTING_CMS_RENDERING_INTENT);
}


/**
 * entangle_preferences_cms_set_rendering_intent:
 * @prefs: (transfer none): the preferences store
 * @intent: the new rendering intent
 *
 * Set the rendering intent for displaying images
 */
void entangle_preferences_cms_set_rendering_intent(EntanglePreferences *prefs,
                                                   EntangleColourProfileIntent intent)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_enum(priv->cmsSettings,
                        SETTING_CMS_RENDERING_INTENT, intent);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_CMS_RENDERING_INTENT);
}


/**
 * entangle_preferences_img_get_aspect_ratio:
 * @prefs: (transfer none): the preferences store
 *
 * Get the aspect ratio for any mask to apply to the image
 *
 * Returns: (transfer full): the aspect ratio for the mask
 */
gchar *entangle_preferences_img_get_aspect_ratio(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_string(priv->imgSettings,
                                 SETTING_IMG_ASPECT_RATIO);
}


/**
 * entangle_preferences_img_set_aspect_ratio:
 * @prefs: (transfer none): the preferences store
 * @aspect: the new aspect ratio
 *
 * Set the aspect ratio for any mask to apply to the image
 */
void entangle_preferences_img_set_aspect_ratio(EntanglePreferences *prefs,
                                               const gchar *aspect)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->imgSettings,
                          SETTING_IMG_ASPECT_RATIO, aspect);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_ASPECT_RATIO);
}


/**
 * entangle_preferences_img_get_mask_opacity:
 * @prefs: (transfer none): the preferences store
 *
 * Get the opacity of any mask to apply to the image
 *
 * Returns: the opacity between 0 and 100
 */
gint entangle_preferences_img_get_mask_opacity(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), 0);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_int(priv->imgSettings,
                              SETTING_IMG_MASK_OPACITY);
}


/**
 * entangle_preferences_img_set_mask_opacity:
 * @prefs: (transfer none): the preferences store
 * @opacity: the opacity of the mask between 0 and 100
 *
 * Set the opacity to use when rendering the image mask
 */
void entangle_preferences_img_set_mask_opacity(EntanglePreferences *prefs,
                                               gint opacity)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_int(priv->imgSettings,
                       SETTING_IMG_MASK_OPACITY, opacity);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_MASK_OPACITY);
}


/**
 * entangle_preferences_img_get_mask_enabled:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if an aspect ratio mask to be applied to the
 * image display
 *
 * Returns: TRUE if an aspect ratio mask should be applied
 */
gboolean entangle_preferences_img_get_mask_enabled(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->imgSettings,
                                  SETTING_IMG_MASK_ENABLED);
}


/**
 * entangle_preferences_img_set_mask_enabled:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to apply an aspect ratio mask
 *
 * If @enabled is TRUE an aspect ratio mask will be applied
 * to the image display
 */
void entangle_preferences_img_set_mask_enabled(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->imgSettings,
                           SETTING_IMG_MASK_ENABLED, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_MASK_ENABLED);
}


/**
 * entangle_preferences_img_get_focus_point:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if the center focus point should be displayed in
 * preview mode
 *
 * Returns: TRUE if a focus point is to be rendered
 */
gboolean entangle_preferences_img_get_focus_point(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->imgSettings,
                                  SETTING_IMG_FOCUS_POINT);
}


/**
 * entangle_preferences_img_set_focus_point:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to render a focus point
 *
 * If @enabled is TRUE then a focus point will be display in
 * preview mode.
 */
void entangle_preferences_img_set_focus_point(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->imgSettings,
                           SETTING_IMG_FOCUS_POINT, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_FOCUS_POINT);
}


/**
 * entangle_preferences_img_get_grid_lines:
 * @prefs: (transfer none): the preferences store
 *
 * Determine what grid lines to display in preview mode
 *
 * Returns: the grid lines to display
 */
gint entangle_preferences_img_get_grid_lines(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), 0);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_enum(priv->imgSettings,
                               SETTING_IMG_GRID_LINES);
}


/**
 * entangle_preferences_img_set_grid_lines:
 * @prefs: (transfer none): the preferences store
 * @gridLines: the grid lines to display
 *
 * Set the grid lines to display when in image preview mode
 */
void entangle_preferences_img_set_grid_lines(EntanglePreferences *prefs, gint gridLines)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_enum(priv->imgSettings,
                        SETTING_IMG_GRID_LINES, gridLines);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_GRID_LINES);
}


/**
 * entangle_preferences_img_get_embedded_preview:
 * @prefs: (transfer none): the preferences store
 *
 * Determine whether to use the embedded preview from files
 * for thumbnails.
 *
 * Returns: TRUE if embedded preview should be used
 */
gboolean entangle_preferences_img_get_embedded_preview(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->imgSettings,
                                  SETTING_IMG_EMBEDDED_PREVIEW);
}


/**
 * entangle_preferences_img_set_embedded_preview:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to use the embedded preview
 *
 * If @enabled is TRUE, any embedded preview will be used
 * when generating thumbnails rather than loading the full
 * resolution image
 */
void entangle_preferences_img_set_embedded_preview(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->imgSettings,
                           SETTING_IMG_EMBEDDED_PREVIEW, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_EMBEDDED_PREVIEW);
}


/**
 * entangle_preferences_img_get_onion_layers:
 * @prefs: (transfer none): the preferences store
 *
 * Determine how many images to overlay in onion skinning mode
 *
 * Returns: the number of image overlays
 */
gint entangle_preferences_img_get_onion_layers(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), 0);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_int(priv->imgSettings,
                              SETTING_IMG_ONION_LAYERS);
}


/**
 * entangle_preferences_img_set_onion_layers:
 * @prefs: (transfer none): the preferences store
 * @layers: the number of image overlays
 *
 * Set the number of images to overlay when in onion skinning
 * mode
 */
void entangle_preferences_img_set_onion_layers(EntanglePreferences *prefs,
                                               gint layers)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_int(priv->imgSettings,
                       SETTING_IMG_ONION_LAYERS, layers);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_ONION_LAYERS);
}


/**
 * entangle_preferences_img_get_onion_skin:
 * @prefs: (transfer none): the preferences store
 *
 * Determine if onion skinning should be used when displaying
 * images
 *
 * Returns: TRUE if onion skinning is enabled
 */
gboolean entangle_preferences_img_get_onion_skin(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), FALSE);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->imgSettings,
                                  SETTING_IMG_ONION_SKIN);
}


/**
 * entangle_preferences_img_set_onion_skin:
 * @prefs: (transfer none): the preferences store
 * @enabled: TRUE to enable onion skinning
 *
 * If @enabled is TRUE then onion skinning will be enabled
 * when displaying images
 */
void entangle_preferences_img_set_onion_skin(EntanglePreferences *prefs, gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_boolean(priv->imgSettings,
                           SETTING_IMG_ONION_SKIN, enabled);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_ONION_SKIN);
}


/**
 * entangle_preferences_img_get_background:
 * @prefs: (transfer none): the preferences store
 *
 * Get the colour used as the background when displaying images
 *
 * Returns: (transfer full): the background colour
 */
gchar *entangle_preferences_img_get_background(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_string(priv->imgSettings,
                                 SETTING_IMG_BACKGROUND);
}


/**
 * entangle_preferences_img_set_background:
 * @prefs: (transfer none): the preferences store
 * @bkg: (transfer none): the background colour
 *
 * Set the colour used as the background when displaying images
 */
void entangle_preferences_img_set_background(EntanglePreferences *prefs, const gchar *bkg)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->imgSettings,
                          SETTING_IMG_BACKGROUND, bkg);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_BACKGROUND);
}


/**
 * entangle_preferences_img_get_highlight:
 * @prefs: (transfer none): the preferences store
 *
 * Get the colour to use when highlighting images
 *
 * Returns: (transfer full): the highlight colour
 */
gchar *entangle_preferences_img_get_highlight(EntanglePreferences *prefs)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES(prefs), NULL);

    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_string(priv->imgSettings,
                                 SETTING_IMG_HIGHLIGHT);
}


/**
 * entangle_preferences_img_set_highlight:
 * @prefs: (transfer none): the preferences store
 * @bkg: (transfer none): the highlight colour
 *
 * Set the colour to use when highlighting images
 */
void entangle_preferences_img_set_highlight(EntanglePreferences *prefs, const gchar *bkg)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES(prefs));

    EntanglePreferencesPrivate *priv = prefs->priv;

    g_settings_set_string(priv->imgSettings,
                          SETTING_IMG_HIGHLIGHT, bkg);
    g_object_notify(G_OBJECT(prefs), PROP_NAME_IMG_HIGHLIGHT);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
