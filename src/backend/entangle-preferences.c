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
    GSettings *cmsSettings;
    GSettings *folderSettings;
};

G_DEFINE_TYPE(EntanglePreferences, entangle_preferences, G_TYPE_OBJECT);

#define SETTING_FOLDER                     "folder"
#define SETTING_CMS                        "cms"

#define SETTING_FOLDER_FILENAME_PATTERN    "filename-pattern"
#define SETTING_FOLDER_PICTURE_DIR         "picture-dir"

#define SETTING_CMS_ENABLED                "enabled"
#define SETTING_CMS_DETECT_SYSTEM_PROFILE  "detect-system-profile"
#define SETTING_CMS_RGB_PROFILE            "rgb-profile"
#define SETTING_CMS_MONITOR_PROFILE        "monitor-profile"
#define SETTING_CMS_RENDERING_INTENT       "rendering-intent"

#define PROP_NAME_FOLDER_FILENAME_PATTERN SETTING_FOLDER "-" SETTING_FOLDER_FILENAME_PATTERN
#define PROP_NAME_FOLDER_PICTURE_DIR      SETTING_FOLDER "-" SETTING_FOLDER_PICTURE_DIR

#define PROP_NAME_CMS_ENABLED                SETTING_CMS "-" SETTING_CMS_ENABLED
#define PROP_NAME_CMS_DETECT_SYSTEM_PROFILE  SETTING_CMS "-" SETTING_CMS_DETECT_SYSTEM_PROFILE
#define PROP_NAME_CMS_RGB_PROFILE            SETTING_CMS "-" SETTING_CMS_RGB_PROFILE
#define PROP_NAME_CMS_MONITOR_PROFILE        SETTING_CMS "-" SETTING_CMS_MONITOR_PROFILE
#define PROP_NAME_CMS_RENDERING_INTENT       SETTING_CMS "-" SETTING_CMS_RENDERING_INTENT

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
        case PROP_PICTURE_DIR:
            dir = g_settings_get_string(priv->folderSettings,
                                        SETTING_FOLDER_PICTURE_DIR);
            if (!dir)
                dir = entangle_find_picture_dir();
            g_value_set_string(value, dir);
            break;

        case PROP_FILENAME_PATTERN:
            file = g_settings_get_string(priv->folderSettings,
                                         SETTING_FOLDER_FILENAME_PATTERN);
            g_value_set_string(value, file);
            g_free(file);
            break;

        case PROP_COLOUR_MANAGED_DISPLAY:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->cmsSettings,
                                                       SETTING_CMS_ENABLED));
            break;

        case PROP_RGB_PROFILE:
            file = g_settings_get_string(priv->cmsSettings,
                                         SETTING_CMS_RGB_PROFILE);
            if (!file)
                file = entangle_find_srgb_profile();
            prof = entangle_colour_profile_new_file(file);
            g_value_set_object(value, prof);
            g_object_unref(prof);
            g_free(file);
            break;

        case PROP_MONITOR_PROFILE:
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

        case PROP_DETECT_SYSTEM_PROFILE:
            g_value_set_boolean(value,
                                g_settings_get_boolean(priv->cmsSettings,
                                                       SETTING_CMS_DETECT_SYSTEM_PROFILE));
            break;

        case PROP_PROFILE_RENDERING_INTENT:
            g_value_set_enum(value,
                             g_settings_get_enum(priv->cmsSettings,
                                                 SETTING_CMS_RENDERING_INTENT));
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
        case PROP_PICTURE_DIR:
            g_settings_set_string(priv->folderSettings,
                                  SETTING_FOLDER_PICTURE_DIR,
                                  g_value_get_string(value));
            break;

        case PROP_FILENAME_PATTERN:
            g_settings_set_string(priv->folderSettings,
                                  SETTING_FOLDER_FILENAME_PATTERN,
                                  g_value_get_string(value));
            break;

        case PROP_COLOUR_MANAGED_DISPLAY:
            g_settings_set_boolean(priv->cmsSettings,
                                   SETTING_CMS_ENABLED,
                                   g_value_get_boolean(value));
            break;

        case PROP_RGB_PROFILE:
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

        case PROP_MONITOR_PROFILE:
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

        case PROP_DETECT_SYSTEM_PROFILE:
            g_settings_set_boolean(priv->cmsSettings,
                                   SETTING_CMS_DETECT_SYSTEM_PROFILE,
                                   g_value_get_boolean(value));
            break;

        case PROP_PROFILE_RENDERING_INTENT:
            g_settings_set_enum(priv->cmsSettings,
                                SETTING_CMS_RENDERING_INTENT,
                                g_value_get_enum(value));
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

    g_object_unref(priv->folderSettings);
    g_object_unref(priv->cmsSettings);

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
                                    g_param_spec_string(PROP_NAME_FOLDER_PICTURE_DIR,
                                                        "Pictures directory",
                                                        "Directory to store pictures in",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_FILENAME_PATTERN,
                                    g_param_spec_string(PROP_NAME_FOLDER_FILENAME_PATTERN,
                                                        "Filename pattern",
                                                        "Pattern for creating new filenames",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_COLOUR_MANAGED_DISPLAY,
                                    g_param_spec_boolean(PROP_NAME_CMS_ENABLED,
                                                         "Colour managed display",
                                                         "Whether to enable colour management on display",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_RGB_PROFILE,
                                    g_param_spec_object(PROP_NAME_CMS_RGB_PROFILE,
                                                        "RGB Profile",
                                                        "Colour profile for workspace",
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_MONITOR_PROFILE,
                                    g_param_spec_object(PROP_NAME_CMS_MONITOR_PROFILE,
                                                        "Monitor profile",
                                                        "Colour profile for monitor",
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_DETECT_SYSTEM_PROFILE,
                                    g_param_spec_boolean(PROP_NAME_CMS_DETECT_SYSTEM_PROFILE,
                                                         "Detect system profile",
                                                         "Detect the monitor colour profile",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PROFILE_RENDERING_INTENT,
                                    g_param_spec_enum(PROP_NAME_CMS_RENDERING_INTENT,
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
    GSettings *settings;

    priv = picker->priv = ENTANGLE_PREFERENCES_GET_PRIVATE(picker);

    settings = g_settings_new("org.entangle-photo.manager");
    priv->folderSettings = g_settings_get_child(settings,
                                                SETTING_FOLDER);
    priv->cmsSettings = g_settings_get_child(settings,
                                             SETTING_CMS);
    g_object_unref(settings);
}


char *entangle_preferences_folder_picture_dir(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    char *dir = g_settings_get_string(priv->folderSettings,
                                      SETTING_FOLDER_PICTURE_DIR);
    if (!dir)
        dir = entangle_find_picture_dir();
    return dir;
}


char *entangle_preferences_folder_filename_pattern(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_string(priv->folderSettings,
                                 SETTING_FOLDER_FILENAME_PATTERN);
}


EntangleColourProfile *entangle_preferences_cms_rgb_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    EntangleColourProfile *prof;

    prof = entangle_colour_profile_new_file(g_settings_get_string(priv->cmsSettings,
                                                                  SETTING_CMS_RGB_PROFILE));

    return prof;
}


EntangleColourProfile *entangle_preferences_cms_monitor_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;
    EntangleColourProfile *prof;

    prof = entangle_colour_profile_new_file(g_settings_get_string(priv->cmsSettings,
                                                                  SETTING_CMS_MONITOR_PROFILE));

    return prof;
}


gboolean entangle_preferences_cms_enabled(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->cmsSettings,
                                  SETTING_CMS_ENABLED);
}

gboolean entangle_preferences_cms_detect_system_profile(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_boolean(priv->cmsSettings,
                                  SETTING_CMS_DETECT_SYSTEM_PROFILE);
}


EntangleColourProfileIntent entangle_preferences_cms_rendering_intent(EntanglePreferences *prefs)
{
    EntanglePreferencesPrivate *priv = prefs->priv;

    return g_settings_get_enum(priv->cmsSettings,
                               SETTING_CMS_RENDERING_INTENT);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
