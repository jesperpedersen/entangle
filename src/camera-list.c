
#include <glib.h>

#include "camera-list.h"

struct _CapaCameraList {
  size_t ncamera;
  CapaCamera **cameras;
};

CapaCameraList *capa_camera_list_new(void)
{
  CapaCameraList *list = g_new0(CapaCameraList, 1);

  return list;
}

void capa_camera_list_free(CapaCameraList *list)
{
  if (!list)
    return;

  for (int i = 0 ; i < list->ncamera ; i++) {
    capa_camera_free(list->cameras[i]);
  }
  g_free(list->cameras);
  g_free(list);
}

int capa_camera_list_count(CapaCameraList *list)
{
  return list->ncamera;
}

void capa_camera_list_add(CapaCameraList *list,
			  CapaCamera *cam)
{
  list->cameras = g_renew(CapaCamera *, list->cameras, list->ncamera+1);
  list->cameras[list->ncamera++] = cam;
}

CapaCamera *capa_camera_list_get(CapaCameraList *list,
				 int entry)
{
  if (entry < 0 || entry >= list->ncamera)
    return NULL;

  return list->cameras[entry];
}

