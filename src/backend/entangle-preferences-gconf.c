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

#include <string.h>
#include <gconf/gconf-client.h>

#include "entangle-debug.h"
#include "entangle-preferences-gconf.h"

#define ENTANGLE_GCONF_DIRECTORY "/apps/entangle"

#define ENTANGLE_PREFERENCES_GCONF_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PREFERENCES_GCONF, EntanglePreferencesGConfPrivate))

struct _EntanglePreferencesGConfPrivate {
    GConfClient *gconf;
};

G_DEFINE_TYPE(EntanglePreferencesGConf, entangle_preferences_gconf, ENTANGLE_TYPE_PREFERENCES);


static void entangle_preferences_gconf_finalize(GObject *object)
{
    EntanglePreferencesGConf *preferences = ENTANGLE_PREFERENCES_GCONF(object);
    EntanglePreferencesGConfPrivate *priv = preferences->priv;

    ENTANGLE_DEBUG("Finalize preferences %p", object);

    g_object_unref(priv->gconf);

    G_OBJECT_CLASS (entangle_preferences_gconf_parent_class)->finalize (object);
}

static void entangle_preferences_gconf_notify_external(GConfClient *client G_GNUC_UNUSED,
                                                   guint cnxn_id G_GNUC_UNUSED,
                                                   GConfEntry *entry,
                                                   gpointer opaque)
{
    EntanglePreferencesGConf *preferences = ENTANGLE_PREFERENCES_GCONF(opaque);
    const char *name;

    ENTANGLE_DEBUG("External Set %p %s", preferences, entry->key);

    if (strncmp(entry->key, ENTANGLE_GCONF_DIRECTORY, strlen(ENTANGLE_GCONF_DIRECTORY)) != 0)
        return;

    name = entry->key + strlen(ENTANGLE_GCONF_DIRECTORY) + 1;

    if (strcmp(name, "colour-managed-display") == 0 ||
        strcmp(name, "detect-system-profile") == 0) {
        gboolean newvalue;
        gboolean oldvalue;
        newvalue = gconf_value_get_bool(entry->value);
        g_object_get(preferences, name, &oldvalue, NULL);

        if (newvalue != oldvalue)
            g_object_set(preferences, name, newvalue, NULL);
    } else if (strcmp(name, "rgb-profile") == 0 ||
               strcmp(name, "monitor-profile") == 0) {
        EntangleColourProfile *oldprofile;
        const gchar *oldvalue;
        const gchar *newvalue;
        newvalue = gconf_value_get_string(entry->value);
        g_object_get(preferences, name, &oldprofile, NULL);
        oldvalue = oldprofile ? entangle_colour_profile_filename(oldprofile) : NULL;

        if (!oldvalue || !newvalue ||
            (strcmp(oldvalue, newvalue) != 0)) {
            EntangleColourProfile *newprofile = newvalue ?
                entangle_colour_profile_new_file(newvalue) : NULL;
            g_object_set(preferences, name, newprofile, NULL);
            if (newprofile)
                g_object_unref(newprofile);
        }
        if (oldprofile)
            g_object_unref(oldprofile);
    } else if (strcmp(name, "picture-dir") == 0 ||
               strcmp(name, "filename-pattern") == 0) {
        const gchar *newvalue;
        gchar *oldvalue;
        newvalue = gconf_value_get_string(entry->value);
        g_object_get(preferences, name, &oldvalue, NULL);

        if (!oldvalue || !newvalue ||
            (strcmp(oldvalue, newvalue) != 0))
            g_object_set(preferences, name, newvalue, NULL);

        g_free(oldvalue);
    } else if (strcmp(name, "profile-rendering-intent") == 0) {
        int newvalue;
        int oldvalue;
        newvalue = gconf_value_get_int(entry->value);
        g_object_get(preferences, name, &oldvalue, NULL);

        if (!oldvalue || !newvalue ||
            (oldvalue != newvalue))
            g_object_set(preferences, name, newvalue, NULL);
    }
}

static void entangle_preferences_gconf_notify_internal(GObject *object, GParamSpec *spec)
{
    EntanglePreferencesGConf *preferences = ENTANGLE_PREFERENCES_GCONF(object);
    EntanglePreferencesGConfPrivate *priv = preferences->priv;
    char *key = g_strdup_printf(ENTANGLE_GCONF_DIRECTORY "/%s", spec->name);

    ENTANGLE_DEBUG("Internal Set %p %s", object, spec->name);
    if (strcmp(spec->name, "colour-managed-display") == 0 ||
        strcmp(spec->name, "detect-system-profile") == 0) {
        gboolean newvalue;
        gboolean oldvalue;
        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gconf_client_get_bool(priv->gconf, key, NULL);

        if (newvalue != oldvalue)
            gconf_client_set_bool(priv->gconf, key, newvalue, NULL);
    } else if (strcmp(spec->name, "rgb-profile") == 0 ||
               strcmp(spec->name, "monitor-profile") == 0) {
        EntangleColourProfile *profile;
        const gchar *newvalue;
        gchar *oldvalue;
        g_object_get(object, spec->name, &profile, NULL);
        newvalue = profile ? entangle_colour_profile_filename(profile) : NULL;
        oldvalue = gconf_client_get_string(priv->gconf, key, NULL);

        if (!oldvalue || !newvalue ||
            (strcmp(oldvalue, newvalue) != 0))
            gconf_client_set_string(priv->gconf, key, newvalue, NULL);

        g_free(oldvalue);
        if (profile)
            g_object_unref(profile);
    } else if (strcmp(spec->name, "picture-dir") == 0 ||
               strcmp(spec->name, "filename-pattern") == 0) {
        gchar *newvalue;
        gchar *oldvalue;
        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gconf_client_get_string(priv->gconf, key, NULL);

        if (!oldvalue || !newvalue ||
            (strcmp(oldvalue, newvalue) != 0))
            gconf_client_set_string(priv->gconf, key, newvalue, NULL);

        g_free(oldvalue);
        g_free(newvalue);
    } else if (strcmp(spec->name, "profile-rendering-intent") == 0) {
        int newvalue;
        int oldvalue;
        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gconf_client_get_int(priv->gconf, key, NULL);

        if (!oldvalue || !newvalue ||
            (oldvalue != newvalue))
            gconf_client_set_int(priv->gconf, key, newvalue, NULL);
    }

    g_free(key);
}


static void entangle_preferences_gconf_load_all(EntanglePreferencesGConf *preferences)
{
    EntanglePreferencesGConfPrivate *priv = preferences->priv;
    const gchar *keys[] = {
        ENTANGLE_GCONF_DIRECTORY "/colour-managed-display",
        ENTANGLE_GCONF_DIRECTORY "/detect-system-profile",
        ENTANGLE_GCONF_DIRECTORY "/rgb-profile",
        ENTANGLE_GCONF_DIRECTORY "/monitor-profile",
        ENTANGLE_GCONF_DIRECTORY "/picture-folder",
        ENTANGLE_GCONF_DIRECTORY "/filename-pattern",
        ENTANGLE_GCONF_DIRECTORY "/profile-rendering-intent",
    };
    int i;

    for (i = 0 ; i < (sizeof(keys)/sizeof(keys[0])) ; i++) {
        GConfValue *value = gconf_client_get_without_default(priv->gconf, keys[i], NULL);
        if (value) {
            GConfEntry entry = {
                g_strdup(keys[i]),
                value,
            };

            entangle_preferences_gconf_notify_external(priv->gconf, 0, &entry, preferences);

            g_free(entry.key);
            gconf_value_free(value);
        }
    }
}


static void entangle_preferences_gconf_class_init(EntanglePreferencesGConfClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_preferences_gconf_finalize;
    object_class->notify = entangle_preferences_gconf_notify_internal;

    g_type_class_add_private(klass, sizeof(EntanglePreferencesGConfPrivate));
}


EntanglePreferences *entangle_preferences_gconf_new(void)
{
    return ENTANGLE_PREFERENCES(g_object_new(ENTANGLE_TYPE_PREFERENCES_GCONF, NULL));
}


static void entangle_preferences_gconf_init(EntanglePreferencesGConf *prefs)
{
    EntanglePreferencesGConfPrivate *priv;

    priv = prefs->priv = ENTANGLE_PREFERENCES_GCONF_GET_PRIVATE(prefs);
    memset(priv, 0, sizeof(*priv));

    priv->gconf = gconf_client_get_default();

    gconf_client_add_dir(priv->gconf,
                         ENTANGLE_GCONF_DIRECTORY,
                         GCONF_CLIENT_PRELOAD_ONELEVEL,
                         NULL);

    gconf_client_notify_add(priv->gconf,
                            ENTANGLE_GCONF_DIRECTORY,
                            entangle_preferences_gconf_notify_external,
                            prefs,
                            NULL,
                            NULL);

    entangle_preferences_gconf_load_all(prefs);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
