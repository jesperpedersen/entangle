
#ifndef __CAPA_PARAMS__
#define __CAPA_PARAMS__

#include <gphoto2.h>

typedef struct _CapaParams CapaParams;

struct _CapaParams {
  GPContext *ctx;
  CameraAbilitiesList *caps;
  GPPortInfoList *ports;
};


CapaParams *capa_params_new(void);

void capa_params_free(CapaParams *params);

void capa_params_refresh(CapaParams *params);


#endif /* __CAPA_PARAMS__ */
