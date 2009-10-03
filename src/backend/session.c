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
#include <string.h>
#include <dirent.h>

#include "session.h"
#include "image.h"

#define CAPA_SESSION_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_SESSION, CapaSessionPrivate))

struct _CapaSessionPrivate {
  char *directory;

  GList *images;
};

G_DEFINE_TYPE(CapaSession, capa_session, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_DIRECTORY,
};

static void capa_session_get_property(GObject *object,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec)
{
  CapaSession *picker = CAPA_SESSION(object);
  CapaSessionPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_DIRECTORY:
      g_value_set_string(value, priv->directory);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_session_set_property(GObject *object,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
  CapaSession *picker = CAPA_SESSION(object);
  CapaSessionPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_DIRECTORY:
      g_free(priv->directory);
      priv->directory = g_value_dup_string(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void do_image_unref(gpointer object,
			   gpointer opaque G_GNUC_UNUSED)
{
  CapaImage *image = object;

  g_object_unref(G_OBJECT(image));
}

static void capa_session_finalize(GObject *object)
{
  CapaSession *session = CAPA_SESSION(object);
  CapaSessionPrivate *priv = session->priv;

  fprintf(stderr, "Finalize session %p\n", object);

  if (priv->images) {
    g_list_foreach(priv->images, do_image_unref, NULL);
    g_list_free(priv->images);
  }

  g_free(priv->directory);

  G_OBJECT_CLASS (capa_session_parent_class)->finalize (object);
}


static void capa_session_class_init(CapaSessionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_session_finalize;
  object_class->get_property = capa_session_get_property;
  object_class->set_property = capa_session_set_property;

  g_object_class_install_property(object_class,
                                  PROP_DIRECTORY,
                                  g_param_spec_string("directory",
                                                      "Session directory",
                                                      "Full path to session file",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaSessionPrivate));
}


CapaSession *capa_session_new(const char *directory)
{
  return CAPA_SESSION(g_object_new(CAPA_TYPE_SESSION,
				 "directory", directory,
				 NULL));
}


static void capa_session_init(CapaSession *session)
{
  CapaSessionPrivate *priv;

  priv = session->priv = CAPA_SESSION_GET_PRIVATE(session);
}


const char *capa_session_directory(CapaSession *session)
{
  CapaSessionPrivate *priv = session->priv;

  return priv->directory;
}

void capa_session_add(CapaSession *session, CapaImage *image)
{
  CapaSessionPrivate *priv = session->priv;

  g_object_ref(G_OBJECT(image));
  priv->images = g_list_prepend(priv->images, image);
}

gboolean capa_session_load(CapaSession *session)
{
  CapaSessionPrivate *priv = session->priv;
  DIR *dh;
  struct dirent *ent;

  dh = opendir(priv->directory);
  if (!dh)
    return FALSE;

  while ((ent = readdir(dh)) != NULL) {
    if (ent->d_name[0] == '.')
      continue;

    char *filename = g_strdup_printf("%s/%s", priv->directory, ent->d_name);
    CapaImage *image = capa_image_new(filename);

    if (capa_image_load(image))
      capa_session_add(session, image);

    g_object_unref(G_OBJECT(image));
  }
  closedir(dh);

  return TRUE;
}
