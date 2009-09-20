
#include <gphoto2.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "app.h"

struct _CapaApp {
  CapaParams *params;
};

CapaApp *capa_app_new(void)
{
  CapaApp *app = g_new0(CapaApp, 1);

  app->params = capa_params_new();

  return app;
}

void capa_app_free(CapaApp *app)
{
  if (!app)
    return;

  capa_params_free(app->params);

  g_free(app);
}

CapaParams *capa_app_params(CapaApp *app)
{
  return app->params;
}

CapaCameraList *capa_app_detect_cameras(CapaApp *app)
{
  CameraList *cams = NULL;
  CapaCameraList *ret = NULL;

  fprintf(stderr, "Detecting\n");
  capa_params_refresh(app->params);

  if (gp_list_new(&cams) != GP_OK)
    return NULL;

  gp_abilities_list_detect(app->params->caps, app->params->ports, cams, app->params->ctx);

  ret = capa_camera_list_new();
  for (int i = 0 ; i < gp_list_count(cams) ; i++) {
    const char *model, *path;

    gp_list_get_name(cams, i, &model);
    gp_list_get_value(cams, i, &path);

    if (strcmp(path, "usb:") != 0)
      capa_camera_list_add(ret,
			   capa_camera_new(model, path));
  }
  gp_list_unref(cams);

  return ret;

}

