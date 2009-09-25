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

#include <gphoto2.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "params.h"

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
    CapaCamera *cam;

    gp_list_get_name(cams, i, &model);
    gp_list_get_value(cams, i, &path);

    /* For back compat, libgphoto2 always adds a default
     * USB camera called 'usb:'. We ignore that, since we
     * can go for the exact camera entries
     */
    if (strcmp(path, "usb:") == 0)
      continue;

    cam = capa_camera_new(model, path);
    capa_camera_list_add(ret, cam);
    fprintf(stderr, "unref list objet");
    g_object_unref(G_OBJECT(cam));
  }
  gp_list_unref(cams);

  return ret;

}

