
#ifndef __CAPA_CAMERA__
#define __CAPA_CAMERA__

#include "params.h"

typedef struct _CapaCamera CapaCamera;


CapaCamera *capa_camera_new(const char *model,
			    const char *port);

void capa_camera_free(CapaCamera *cam);

const char *capa_camera_model(CapaCamera *cam);
const char *capa_camera_port(CapaCamera *cam);

int capa_camera_connect(CapaCamera *cap, CapaParams *params);

char *capa_camera_summary(CapaCamera *cam);
char *capa_camera_manual(CapaCamera *cam);
char *capa_camera_driver(CapaCamera *cam);

#endif /* __CAPA_CAMERA__ */
