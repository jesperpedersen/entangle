
#include <glib.h>
#include <stdio.h>

#include "camera.h"

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


int capa_camera_connect(CapaCamera *cam, CapaParams *params)
{
  int i;
  GPPortInfo port;
  CameraAbilities cap;

  fprintf(stderr, "Conencting to cam\n");

  if (cam->cam != NULL)
    return 0;


  i = gp_port_info_list_lookup_path(params->ports, cam->port);
  gp_port_info_list_get_info(params->ports, i, &port);

  i = gp_abilities_list_lookup_model(params->caps, cam->model);
  gp_abilities_list_get_abilities(params->caps, i, &cap);

  gp_camera_new(&cam->cam);
  gp_camera_set_abilities(cam->cam, cap);
  gp_camera_set_port_info(cam->cam, port);

  if (gp_camera_init(cam->cam, params->ctx) != GP_OK) {
    gp_camera_unref(cam->cam);
    cam->cam = NULL;
    fprintf(stderr, "failed\n");
    return -1;
  }

  cam->params = params;

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
