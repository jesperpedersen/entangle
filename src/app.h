
#ifndef __CAPA_APP__
#define __CAPA_APP__

#include "camera-list.h"
#include "params.h"

typedef struct _CapaApp CapaApp;

CapaApp *capa_app_new(void);

void capa_app_free(CapaApp *app);

CapaParams *capa_app_params(CapaApp *app);

CapaCameraList *capa_app_detect_cameras(CapaApp *app);


#endif /* __CAPA_APPLICATION__ */
