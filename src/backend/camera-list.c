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

#include "camera-list.h"

#define CAPA_CAMERA_LIST_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_LIST, CapaCameraListPrivate))

struct _CapaCameraListPrivate {
  size_t ncamera;
  CapaCamera **cameras;
};

G_DEFINE_TYPE(CapaCameraList, capa_camera_list, G_TYPE_OBJECT);


static void capa_camera_list_finalize (GObject *object)
{
  CapaCameraList *list = CAPA_CAMERA_LIST(object);
  CapaCameraListPrivate *priv = list->priv;
  fprintf(stderr, "Finalize list\n");

  for (int i = 0 ; i < priv->ncamera ; i++) {
    fprintf(stderr, "Unref camera in list %p\n", priv->cameras[i]);
    g_object_unref(G_OBJECT(priv->cameras[i]));
  }
  g_free(priv->cameras);

  G_OBJECT_CLASS (capa_camera_list_parent_class)->finalize (object);
}


static void capa_camera_list_class_init(CapaCameraListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_camera_list_finalize;

  g_signal_new("camera-added",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraListClass, camera_added),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__OBJECT,
	       G_TYPE_NONE,
	       1,
	       CAPA_TYPE_CAMERA);

  g_signal_new("camera-removed",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraListClass, camera_removed),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__OBJECT,
	       G_TYPE_NONE,
	       1,
	       CAPA_TYPE_CAMERA);


  g_type_class_add_private(klass, sizeof(CapaCameraListPrivate));
}


CapaCameraList *capa_camera_list_new(void)
{
  return CAPA_CAMERA_LIST(g_object_new(CAPA_TYPE_CAMERA_LIST, NULL));
}


static void capa_camera_list_init(CapaCameraList *list)
{
  CapaCameraListPrivate *priv;

  priv = list->priv = CAPA_CAMERA_LIST_GET_PRIVATE(list);
}


int capa_camera_list_count(CapaCameraList *list)
{
  CapaCameraListPrivate *priv = list->priv;

  return priv->ncamera;
}

void capa_camera_list_add(CapaCameraList *list,
			  CapaCamera *cam)
{
  CapaCameraListPrivate *priv = list->priv;

  priv->cameras = g_renew(CapaCamera *, priv->cameras, priv->ncamera+1);
  priv->cameras[priv->ncamera++] = cam;
  g_object_ref(G_OBJECT(cam));

  g_signal_emit_by_name(G_OBJECT(list), "camera-added", cam);
  fprintf(stderr, "Added camera %p\n", cam);
}

void capa_camera_list_remove(CapaCameraList *list,
			     CapaCamera *cam)
{
  CapaCameraListPrivate *priv = list->priv;
  gboolean removed = FALSE;

  for (int i = 0 ; i < priv->ncamera ; i++) {
    if (priv->cameras[i] == cam) {
      removed = TRUE;
      if (i < (priv->ncamera-1))
	memmove(priv->cameras + i,
		priv->cameras + i + 1,
		sizeof(*priv->cameras) * (priv->ncamera - i - 1));
      priv->ncamera--;
    }
  }

  fprintf(stderr, "Removed camera %p from list\n", cam);
  g_signal_emit_by_name(G_OBJECT(list), "camera-removed", cam);

  g_object_unref(cam);
}

CapaCamera *capa_camera_list_get(CapaCameraList *list,
				 int entry)
{
  CapaCameraListPrivate *priv = list->priv;

  if (entry < 0 || entry >= priv->ncamera)
    return NULL;

  return priv->cameras[entry];
}

CapaCamera *capa_camera_list_find(CapaCameraList *list,
				  const char *port)
{
  CapaCameraListPrivate *priv = list->priv;
  int i;

  for (i = 0 ; i < priv->ncamera ; i++) {
    const char *thisport = capa_camera_port(priv->cameras[i]);

    fprintf(stderr, "Compare '%s' '%s'\n", port, thisport);

    if (strcmp(thisport, port) == 0)
      return priv->cameras[i];
  }

  return NULL;
}

