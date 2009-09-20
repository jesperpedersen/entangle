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

