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

#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "preferences.h"
#include "params.h"
#include "progress.h"

#define CAPA_PREFERENCES_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_PREFERENCES, CapaPreferencesPrivate))

struct _CapaPreferencesPrivate {
  char *pictureDir;
};

G_DEFINE_TYPE(CapaPreferences, capa_preferences, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_PICTURE_DIR,
};


static char *capa_find_base_dir(void)
{
  const char *xdgUserDir = "/usr/bin/xdg-user-dir";
  const char *xdgUserDirArgs[] = {
    xdgUserDir, "PICTURES", NULL,
  };
  char *dir;
  char *tmp;
  gint status;
  GError *err = NULL;
  int len;

  g_spawn_sync(NULL, (char**)xdgUserDirArgs, NULL,
	       G_SPAWN_STDERR_TO_DEV_NULL,
	       NULL, NULL, &dir, NULL, &status, &err);

  if (dir) {
    tmp = strchr(dir, '\n');
    if (tmp) *tmp = '\0';
    return dir;
  }

  len = 1024;
  do {
    dir = g_renew(char, dir, len);
    if (dir == NULL) {
      if (errno != ERANGE)
	len += 1024;
      else
	return NULL;
    }
  } while (!dir);

  return dir;
}

static char *capa_find_picture_dir(void)
{
  char *baseDir = capa_find_base_dir();
  char *ret;
  if (baseDir) {
    ret = g_strdup_printf("%s/%s", baseDir, "Capture");
  } else {
    ret = g_strdup_printf("Capture");
  }
  g_free(baseDir);
  fprintf(stderr, "******** PICTURE '%s'\n", ret);
  return ret;
}

static void capa_preferences_get_property(GObject *object,
					  guint prop_id,
					  GValue *value,
					  GParamSpec *pspec)
{
  CapaPreferences *picker = CAPA_PREFERENCES(object);
  CapaPreferencesPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_PICTURE_DIR:
      g_value_set_string(value, priv->pictureDir);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_preferences_set_property(GObject *object,
					  guint prop_id,
					  const GValue *value,
					  GParamSpec *pspec)
{
  CapaPreferences *picker = CAPA_PREFERENCES(object);
  CapaPreferencesPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_PICTURE_DIR:
      g_free(priv->pictureDir);
      priv->pictureDir = g_value_dup_string(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void capa_preferences_finalize(GObject *object)
{
  CapaPreferences *preferences = CAPA_PREFERENCES(object);
  CapaPreferencesPrivate *priv = preferences->priv;

  fprintf(stderr, "Finalize preferences %p\n", object);

  g_free(priv->pictureDir);

  G_OBJECT_CLASS (capa_preferences_parent_class)->finalize (object);
}


static void capa_preferences_class_init(CapaPreferencesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_preferences_finalize;
  object_class->get_property = capa_preferences_get_property;
  object_class->set_property = capa_preferences_set_property;

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

  g_type_class_add_private(klass, sizeof(CapaPreferencesPrivate));
}


CapaPreferences *capa_preferences_new(void)
{
  return CAPA_PREFERENCES(g_object_new(CAPA_TYPE_PREFERENCES,
				       "picture-dir", capa_find_picture_dir(),
				       NULL));
}


static void capa_preferences_init(CapaPreferences *picker)
{
  CapaPreferencesPrivate *priv;

  priv = picker->priv = CAPA_PREFERENCES_GET_PRIVATE(picker);
}


const char *capa_preferences_picture_dir(CapaPreferences *prefs)
{
  CapaPreferencesPrivate *priv = prefs->priv;

  return priv->pictureDir;
}
