

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

