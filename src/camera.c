
#include <glib.h>

#include "camera.h"

struct _CapaCamera {
  char *model;
  char *port;
};

CapaCamera *capa_camera_new(const char *model,
			    const char *port)
{
  CapaCamera *cam = g_new0(CapaCamera, 1);

  cam->model = g_strdup(model);
  cam->port = g_strdup(port);

  return cam;
}

void capa_camera_free(CapaCamera *cam)
{
  if (!cam)
    return;
  g_free(cam->model);
  g_free(cam->port);
  g_free(cam);
}

const char *capa_camera_model(CapaCamera *cam)
{
  return cam->model;
}

const char *capa_camera_port(CapaCamera *cam)
{
  return cam->port;
}


int capa_camera_connect(CapaCamera *cap, CapaParams *params)
{
  return 0;
}
