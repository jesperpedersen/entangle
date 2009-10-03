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
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "session.h"
#include "image.h"

#define CAPA_SESSION_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_SESSION, CapaSessionPrivate))

struct _CapaSessionPrivate {
  char *directory;
  char *filenamePattern;
  int nextFilenameDigit;

  GList *images;
};

G_DEFINE_TYPE(CapaSession, capa_session, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_DIRECTORY,
  PROP_FILENAME_PATTERN,
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
      g_mkdir_with_parents(priv->directory, 0777);
      break;

    case PROP_FILENAME_PATTERN:
      g_value_set_string(value, priv->filenamePattern);
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

    case PROP_FILENAME_PATTERN:
      g_free(priv->filenamePattern);
      priv->filenamePattern = g_value_dup_string(value);
      priv->nextFilenameDigit = 0;
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

  g_free(priv->filenamePattern);
  g_free(priv->directory);

  G_OBJECT_CLASS (capa_session_parent_class)->finalize (object);
}


static void capa_session_class_init(CapaSessionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_session_finalize;
  object_class->get_property = capa_session_get_property;
  object_class->set_property = capa_session_set_property;

  g_signal_new("session-image-added",
               G_TYPE_FROM_CLASS(klass),
               G_SIGNAL_RUN_FIRST,
               G_STRUCT_OFFSET(CapaSessionClass, session_image_added),
               NULL, NULL,
               g_cclosure_marshal_VOID__OBJECT,
               G_TYPE_NONE,
               1,
               CAPA_TYPE_IMAGE);

  g_signal_new("session-image-removed",
               G_TYPE_FROM_CLASS(klass),
               G_SIGNAL_RUN_FIRST,
               G_STRUCT_OFFSET(CapaSessionClass, session_image_removed),
               NULL, NULL,
               g_cclosure_marshal_VOID__OBJECT,
               G_TYPE_NONE,
               1,
               CAPA_TYPE_IMAGE);

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

  g_object_class_install_property(object_class,
                                  PROP_FILENAME_PATTERN,
                                  g_param_spec_string("filename-pattern",
                                                      "Filename patern",
                                                      "Pattern for creating new filenames",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaSessionPrivate));
}


CapaSession *capa_session_new(const char *directory,
			      const char *filenamePattern)
{
  return CAPA_SESSION(g_object_new(CAPA_TYPE_SESSION,
				   "directory", directory,
				   "filename-pattern", filenamePattern,
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

const char *capa_session_filename_pattern(CapaSession *session)
{
  CapaSessionPrivate *priv = session->priv;

  return priv->filenamePattern;
}

char *capa_session_next_filename(CapaSession *session)
{
  CapaSessionPrivate *priv = session->priv;
  const char *template = strchr(priv->filenamePattern, 'X');
  const char *postfix;
  char *prefix;
  char *format;
  char *filename;
  int i;
  int max;
  int ndigits;

  fprintf(stderr, "NEXT FILENAME '%s'\n", template);

  if (!template)
    return NULL;

  postfix = template;
  while (*postfix == 'X')
    postfix++;

  prefix = g_strndup(priv->filenamePattern,
		     template - priv->filenamePattern);

  ndigits = postfix - template;

  for (max = 1, i = 0 ; i < ndigits ; i++)
    max *= 10;

  format = g_strdup_printf("%%s/%%s%%0%dd%%s", ndigits);

  fprintf(stderr, "TEST %d (%d) possible with '%s' prefix='%s' postfix='%s'\n", max, ndigits, format, prefix, postfix);

  for (i = priv->nextFilenameDigit ; i < max ; i++) {
    filename = g_strdup_printf(format, priv->directory,
			       prefix, i, postfix);

    fprintf(stderr, "Test filename '%s'\n", filename);
    if (access(filename, R_OK) < 0) {
      if (errno != ENOENT) {
	g_free(filename);
	filename = NULL;
      }
      /* Cache digit offset to avoid stat() sooo many files next time */
      priv->nextFilenameDigit = i + 1;
      break;
    }
    g_free(filename);
    filename = NULL;
  }

  g_free(prefix);
  g_free(format);
  return filename;
}


char *capa_session_temp_filename(CapaSession *session)
{
  CapaSessionPrivate *priv = session->priv;
  char *pattern = g_strdup_printf("%s/%s", priv->directory, "previewXXXXXX");
  int fd = mkstemp(pattern);

  if (fd < 0) {
    g_free(pattern);
    return NULL;
  }

  close(fd);
  return pattern;
}


void capa_session_add(CapaSession *session, CapaImage *image)
{
  CapaSessionPrivate *priv = session->priv;

  g_object_ref(G_OBJECT(image));
  priv->images = g_list_prepend(priv->images, image);

  g_signal_emit_by_name(G_OBJECT(session), "session-image-added", image);
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
