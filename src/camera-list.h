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


#ifndef __CAPA_CAMERA_LIST__
#define __CAPA_CAMERA_LIST__

#include "camera.h"

typedef struct _CapaCameraList CapaCameraList;

CapaCameraList *capa_camera_list_new(void);

void capa_camera_list_free(CapaCameraList *list);

int capa_camera_list_count(CapaCameraList *list);

void capa_camera_list_add(CapaCameraList *list,
			  CapaCamera *cam);

CapaCamera *capa_camera_list_get(CapaCameraList *list,
				 int entry);



#endif /* __CAPA_CAMERA_LIST__ */

