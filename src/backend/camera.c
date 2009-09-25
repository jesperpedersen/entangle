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
#include <stdio.h>

#include "camera.h"
#include "params.h"

struct _CapaCamera {
  CapaParams *params;
  Camera *cam;

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
  if (cam->cam)
    gp_camera_free(cam->cam);
  capa_params_free(cam->params);
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


int capa_camera_connect(CapaCamera *cam)
{
  int i;
  GPPortInfo port;
  CameraAbilities cap;

  fprintf(stderr, "Conencting to cam\n");

  if (cam->cam != NULL)
    return 0;

  cam->params = capa_params_new();

  i = gp_port_info_list_lookup_path(cam->params->ports, cam->port);
  gp_port_info_list_get_info(cam->params->ports, i, &port);

  i = gp_abilities_list_lookup_model(cam->params->caps, cam->model);
  gp_abilities_list_get_abilities(cam->params->caps, i, &cap);

  gp_camera_new(&cam->cam);
  gp_camera_set_abilities(cam->cam, cap);
  gp_camera_set_port_info(cam->cam, port);

  if (gp_camera_init(cam->cam, cam->params->ctx) != GP_OK) {
    gp_camera_unref(cam->cam);
    cam->cam = NULL;
    fprintf(stderr, "failed\n");
    return -1;
  }

  fprintf(stderr, "ok\n");
  return 0;
}

char *capa_camera_summary(CapaCamera *cam)
{
  CameraText txt;
  if (cam->cam == NULL)
    return g_strdup("");

  gp_camera_get_summary(cam->cam, &txt, cam->params->ctx);

  return g_strdup(txt.text);
}

char *capa_camera_manual(CapaCamera *cam)
{
  CameraText txt;
  if (cam->cam == NULL)
    return g_strdup("");

  gp_camera_get_manual(cam->cam, &txt, cam->params->ctx);

  return g_strdup(txt.text);
}

char *capa_camera_driver(CapaCamera *cam)
{
  CameraText txt;
  if (cam->cam == NULL)
    return g_strdup("");

  gp_camera_get_about(cam->cam, &txt, cam->params->ctx);

  return g_strdup(txt.text);
}


int capa_camera_capture(CapaCamera *cam, const char *localpath)
{
  CameraFilePath path;
  CameraFile *file;

  if (cam->cam == NULL)
    return -1;

  fprintf(stderr, "Starting capture\n");
  if (gp_camera_capture(cam->cam, GP_CAPTURE_IMAGE, &path, cam->params->ctx) != GP_OK) {
    fprintf(stderr, "Failed capture\n");
    return -1;
  }

  fprintf(stderr, "captured '%s' '%s'\n", path.folder, path.name);

  gp_file_new(&file);

  fprintf(stderr, "Getting file\n");
  if (gp_camera_file_get(cam->cam, path.folder, path.name, GP_FILE_TYPE_NORMAL, file, cam->params->ctx) != GP_OK)
    goto error;

  fprintf(stderr, "Saveing file\n");
  if (gp_file_save(file, localpath) != GP_OK)
    goto error_delete;

  fprintf(stderr, "Deleting file\n");
  if (gp_camera_file_delete(cam->cam, path.folder, path.name, cam->params->ctx) != GP_OK)
    goto error;

  gp_file_unref(file);

  fprintf(stderr, "Done\n");
  return 0;

 error_delete:
  fprintf(stderr, "Error, try delete\n");
  gp_camera_file_delete(cam->cam, path.folder, path.name, cam->params->ctx);

 error:
  fprintf(stderr, "Error\n");
  gp_file_unref(file);
  return -1;
}


static int do_process_control(CapaControlGroup *grp,
			      const char *path,
			      CameraWidget *widget)
{
  CameraWidgetType type;
  CapaControl *control;
  const char *name;
  char *fullpath;

  if (gp_widget_get_type(widget, &type) != GP_OK)
    return -1;

  if (gp_widget_get_name(widget, &name) != GP_OK)
    return -1;

  fullpath = g_strdup_printf("%s/%s", path, name);

  switch (type) {
  case GP_WIDGET_WINDOW:
  case GP_WIDGET_SECTION:
    {
      fprintf(stderr, "Recurse %s\n", fullpath);
      for (int i = 0 ; i < gp_widget_count_children(widget) ; i++) {
	CameraWidget *child;
	if (gp_widget_get_child(widget, i, &child) != GP_OK)
	  goto error;
	if (do_process_control(grp, fullpath, child) < 0)
	  goto error;
      }
    } break;

  default:
    {
      int id;
      const char *label;
      gp_widget_get_id(widget, &id);
      gp_widget_get_label(widget, &label);

      fprintf(stderr, "Add %s %d %s\n", fullpath, id, label);
      control = capa_control_new(fullpath, id, label);
      capa_control_group_add(grp, control);
    } break;
  }

  return 0;

  error:
  g_free(fullpath);
  return -1;
}

CapaControlGroup *capa_camera_controls(CapaCamera *cam)
{
  CameraWidget *win = NULL;
  CapaControlGroup *grp;

  if (cam->cam == NULL)
    return NULL;

  if (gp_camera_get_config(cam->cam, &win, cam->params->ctx) != GP_OK)
    return NULL;

  grp = capa_control_group_new();

  if (do_process_control(grp, "", win) < 0) {
    g_object_unref(grp);
    grp = NULL;
  }

  gp_widget_unref(win);

  return grp;
}
