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

#include "internal.h"
#include "image.h"

#define CAPA_IMAGE_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_IMAGE, CapaImagePrivate))

struct _CapaImagePrivate {
  char *filename;

  GdkPixbuf *thumbnail;

  gboolean hasStat;
  struct stat st;
};

G_DEFINE_TYPE(CapaImage, capa_image, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_FILENAME,
};

static void capa_image_get_property(GObject *object,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec)
{
  CapaImage *picker = CAPA_IMAGE(object);
  CapaImagePrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_FILENAME:
      g_value_set_string(value, priv->filename);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_image_set_property(GObject *object,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
  CapaImage *picker = CAPA_IMAGE(object);
  CapaImagePrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_FILENAME:
      g_free(priv->filename);
      priv->filename = g_value_dup_string(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void capa_image_finalize(GObject *object)
{
  CapaImage *image = CAPA_IMAGE(object);
  CapaImagePrivate *priv = image->priv;

  CAPA_DEBUG("Finalize image %p", object);

  if (priv->thumbnail)
    g_object_unref(G_OBJECT(priv->thumbnail));

  g_free(priv->filename);

  G_OBJECT_CLASS (capa_image_parent_class)->finalize (object);
}


static void capa_image_class_init(CapaImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_image_finalize;
  object_class->get_property = capa_image_get_property;
  object_class->set_property = capa_image_set_property;

  g_object_class_install_property(object_class,
                                  PROP_FILENAME,
                                  g_param_spec_string("filename",
                                                      "Image filename",
                                                      "Full path to image file",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaImagePrivate));
}


CapaImage *capa_image_new(const char *filename)
{
  return CAPA_IMAGE(g_object_new(CAPA_TYPE_IMAGE,
				 "filename", filename,
				 NULL));
}


static void capa_image_init(CapaImage *picker)
{
  CapaImagePrivate *priv;

  priv = picker->priv = CAPA_IMAGE_GET_PRIVATE(picker);

  priv->thumbnail = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 96, 96);
  gdk_pixbuf_fill(priv->thumbnail, 0x000000FF);
}


const char *capa_image_filename(CapaImage *image)
{
  CapaImagePrivate *priv = image->priv;
  return priv->filename;
}

gboolean capa_image_load(CapaImage *image)
{
  CapaImagePrivate *priv = image->priv;
  GdkPixbuf *thumb;
  int tw, th;

  if (stat(priv->filename, &priv->st) < 0) {
    memset(&priv->st, 0, sizeof priv->st);
    return FALSE;
  }

  /* XXX stupidly inefficient to load it here, or in this way.
   * Switch to a background thread + cached thumbnails as
   * per fd.o spec */
  thumb = gdk_pixbuf_new_from_file_at_size(priv->filename,
					   96, 96, NULL);

  tw = gdk_pixbuf_get_width(thumb);
  th = gdk_pixbuf_get_height(thumb);

  if (thumb) {
    gdk_pixbuf_copy_area(thumb,
			 0, 0,
			 tw, th,
			 priv->thumbnail,
			 (96-tw)/2, (96-th)/2);
  }
  g_object_unref(thumb);

  return TRUE;
}

time_t capa_image_last_modified(CapaImage *image)
{
  CapaImagePrivate *priv = image->priv;

  return priv->st.st_mtime;
}

off_t capa_image_file_size(CapaImage *image)
{
  CapaImagePrivate *priv = image->priv;

  return priv->st.st_mtime;
}


GdkPixbuf *capa_image_thumbnail(CapaImage *image)
{
  CapaImagePrivate *priv = image->priv;

  return priv->thumbnail;
}
