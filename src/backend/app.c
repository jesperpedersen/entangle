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
#include "device-manager.h"

struct _CapaApp {
  CapaParams *params;

  CapaDeviceManager *devManager;
  CapaCameraList *cameras;

  CapaPreferences *prefs;
};

static void do_refresh_cameras(CapaApp *app)
{
  CameraList *cams = NULL;
  GHashTable *toRemove;
  GHashTableIter iter;
  gpointer key, value;

  capa_params_refresh(app->params);

  fprintf(stderr, "Detecting1\n");

  if (gp_list_new(&cams) != GP_OK)
    return;

  gp_abilities_list_detect(app->params->caps, app->params->ports, cams, app->params->ctx);

  for (int i = 0 ; i < gp_list_count(cams) ; i++) {
    const char *model, *port;
    int n;
    CapaCamera *cam;
    CameraAbilities cap;

    gp_list_get_name(cams, i, &model);
    gp_list_get_value(cams, i, &port);

    cam = capa_camera_list_find(app->cameras, port);

    if (cam)
      continue;

    n = gp_abilities_list_lookup_model(app->params->caps, model);
    gp_abilities_list_get_abilities(app->params->caps, n, &cap);

    /* For back compat, libgphoto2 always adds a default
     * USB camera called 'usb:'. We ignore that, since we
     * can go for the exact camera entries
     */
    if (strcmp(port, "usb:") == 0)
      continue;

    fprintf(stderr, "New camera '%s' '%s' %d\n", model, port, cap.operations);
    cam = capa_camera_new(model, port,
			  cap.operations & GP_OPERATION_CAPTURE_IMAGE ? TRUE : FALSE,
			  cap.operations & GP_OPERATION_CAPTURE_PREVIEW ? TRUE : FALSE,
			  cap.operations & GP_OPERATION_CONFIG ? TRUE : FALSE);
    capa_camera_list_add(app->cameras, cam);
    g_object_unref(G_OBJECT(cam));
  }

  toRemove = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  for (int i = 0 ; i < capa_camera_list_count(app->cameras) ; i++) {
    gboolean found = FALSE;
    CapaCamera *cam = capa_camera_list_get(app->cameras, i);

    fprintf(stderr, "Checking if %s exists\n", capa_camera_port(cam));

    for (int j = 0 ; j < gp_list_count(cams) ; j++) {
      const char *port;
      gp_list_get_value(cams, j, &port);

      if (strcmp(port, capa_camera_port(cam)) == 0) {
	found = TRUE;
	break;
      }
    }
    if (!found)
      g_hash_table_insert(toRemove, g_strdup(capa_camera_port(cam)), cam);
  }

  gp_list_unref(cams);

  g_hash_table_iter_init(&iter, toRemove);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    CapaCamera *cam = value;

    capa_camera_list_remove(app->cameras, cam);
  }
  g_hash_table_unref(toRemove);
}

static void do_device_addremove(CapaDeviceManager *manager G_GNUC_UNUSED,
				char *port G_GNUC_UNUSED,
				CapaApp *app)
{
  do_refresh_cameras(app);
}

CapaApp *capa_app_new(void)
{
  CapaApp *app = g_new0(CapaApp, 1);

  app->params = capa_params_new();
  app->cameras = capa_camera_list_new();
  app->devManager = capa_device_manager_new();

  g_signal_connect(app->devManager, "device-added", G_CALLBACK(do_device_addremove), app);
  g_signal_connect(app->devManager, "device-removed", G_CALLBACK(do_device_addremove), app);

  do_refresh_cameras(app);

  return app;
}

void capa_app_free(CapaApp *app)
{
  if (!app)
    return;

  capa_params_free(app->params);

  g_object_unref(G_OBJECT(app->cameras));

  g_object_unref(G_OBJECT(app->devManager));

  fprintf(stderr, "Free app\n");
  g_free(app);
}

void capa_app_refresh(CapaApp *app)
{
  do_refresh_cameras(app);
}

CapaCameraList *capa_app_cameras(CapaApp *app)
{
  return app->cameras;
}

CapaPreferences *capa_app_preferences(CapaApp *app)
{
  return app->prefs;
}
