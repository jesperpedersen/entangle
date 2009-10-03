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
#include <unistd.h>

#include "camera.h"
#include "params.h"
#include "progress.h"

#define CAPA_CAMERA_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA, CapaCameraPrivate))

struct _CapaCameraPrivate {
  CapaParams *params;
  Camera *cam;

  CapaSession *session;

  CapaProgress *progress;

  char *model;
  char *port;

  GThread *operation;

  gboolean hasCapture;
  gboolean hasPreview;
  gboolean hasSettings;
};

G_DEFINE_TYPE(CapaCamera, capa_camera, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_MODEL,
  PROP_PORT,
  PROP_SESSION,
  PROP_PROGRESS,
  PROP_HAS_CAPTURE,
  PROP_HAS_PREVIEW,
  PROP_HAS_SETTINGS,
};

static void capa_camera_get_property(GObject *object,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec)
{
  CapaCamera *picker = CAPA_CAMERA(object);
  CapaCameraPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_string(value, priv->model);
      break;

    case PROP_PORT:
      g_value_set_string(value, priv->port);
      break;

    case PROP_SESSION:
      g_value_set_object(value, priv->session);
      break;

    case PROP_PROGRESS:
      g_value_set_object(value, priv->progress);
      break;

    case PROP_HAS_CAPTURE:
      g_value_set_boolean(value, priv->hasCapture);
      break;

    case PROP_HAS_PREVIEW:
      g_value_set_boolean(value, priv->hasPreview);
      break;

    case PROP_HAS_SETTINGS:
      g_value_set_boolean(value, priv->hasSettings);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_camera_set_property(GObject *object,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
  CapaCamera *picker = CAPA_CAMERA(object);
  CapaCameraPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_MODEL:
      g_free(priv->model);
      priv->model = g_value_dup_string(value);
      break;

    case PROP_PORT:
      g_free(priv->port);
      priv->port = g_value_dup_string(value);
      break;

    case PROP_SESSION:
      if (priv->session)
	g_object_unref(G_OBJECT(priv->session));
      priv->session = g_value_get_object(value);
      g_object_ref(priv->session);
      break;

    case PROP_PROGRESS:
      if (priv->progress)
	g_object_unref(G_OBJECT(priv->progress));
      priv->progress = g_value_get_object(value);
      g_object_ref(priv->progress);
      break;

    case PROP_HAS_CAPTURE:
      priv->hasCapture = g_value_get_boolean(value);
      fprintf(stderr, "Set has capture %d\n", priv->hasCapture);
      break;

    case PROP_HAS_PREVIEW:
      priv->hasPreview = g_value_get_boolean(value);
      fprintf(stderr, "Set has preview %d\n", priv->hasPreview);
      break;

    case PROP_HAS_SETTINGS:
      priv->hasSettings = g_value_get_boolean(value);
      fprintf(stderr, "Set has settings %d\n", priv->hasSettings);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void capa_camera_finalize(GObject *object)
{
  CapaCamera *camera = CAPA_CAMERA(object);
  CapaCameraPrivate *priv = camera->priv;

  fprintf(stderr, "Finalize camera %p\n", object);

  if (priv->progress)
    g_object_unref(priv->progress);
  if (priv->session)
    g_object_unref(priv->session);
  if (priv->cam) {
    gp_camera_exit(priv->cam, priv->params->ctx);
    gp_camera_free(priv->cam);
  }
  capa_params_free(priv->params);
  g_free(priv->model);
  g_free(priv->port);

  G_OBJECT_CLASS (capa_camera_parent_class)->finalize (object);
}


static void capa_camera_class_init(CapaCameraClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_camera_finalize;
  object_class->get_property = capa_camera_get_property;
  object_class->set_property = capa_camera_set_property;

  g_signal_new("camera-image",
               G_TYPE_FROM_CLASS(klass),
               G_SIGNAL_RUN_FIRST,
               G_STRUCT_OFFSET(CapaCameraClass, camera_image),
               NULL, NULL,
               g_cclosure_marshal_VOID__OBJECT,
               G_TYPE_NONE,
               1,
               CAPA_TYPE_IMAGE);

  g_signal_new("camera-error",
               G_TYPE_FROM_CLASS(klass),
               G_SIGNAL_RUN_FIRST,
               G_STRUCT_OFFSET(CapaCameraClass, camera_error),
               NULL, NULL,
               g_cclosure_marshal_VOID__STRING,
               G_TYPE_NONE,
               1,
	       G_TYPE_STRING);


  g_signal_new("camera-op-begin",
               G_TYPE_FROM_CLASS(klass),
               G_SIGNAL_RUN_FIRST,
               G_STRUCT_OFFSET(CapaCameraClass, camera_op_end),
               NULL, NULL,
               g_cclosure_marshal_VOID__VOID,
               G_TYPE_NONE,
               0);


  g_signal_new("camera-op-end",
               G_TYPE_FROM_CLASS(klass),
               G_SIGNAL_RUN_FIRST,
               G_STRUCT_OFFSET(CapaCameraClass, camera_op_end),
               NULL, NULL,
               g_cclosure_marshal_VOID__VOID,
               G_TYPE_NONE,
               0);


  g_object_class_install_property(object_class,
                                  PROP_MODEL,
                                  g_param_spec_string("model",
                                                      "Camera model",
                                                      "Model name of the camera",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property(object_class,
                                  PROP_PORT,
                                  g_param_spec_string("port",
                                                      "Camera port",
                                                      "Device port of the camera",
                                                      NULL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property(object_class,
				  PROP_SESSION,
				  g_param_spec_object("session",
						      "Active session",
						      "Active session for image capture",
						      CAPA_TYPE_SESSION,
						      G_PARAM_READWRITE |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_object_class_install_property(object_class,
				  PROP_PROGRESS,
				  g_param_spec_object("progress",
						      "Progress updater",
						      "Operation progress updater",
						      CAPA_PROGRESS_TYPE,
						      G_PARAM_READWRITE |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_object_class_install_property(object_class,
				  PROP_HAS_CAPTURE,
				  g_param_spec_boolean("has-capture",
						       "Capture supported",
						       "Whether image capture is supported",
						       FALSE,
						       G_PARAM_READWRITE |
						       G_PARAM_CONSTRUCT_ONLY |
						       G_PARAM_STATIC_NAME |
						       G_PARAM_STATIC_NICK |
						       G_PARAM_STATIC_BLURB));
  g_object_class_install_property(object_class,
				  PROP_HAS_PREVIEW,
				  g_param_spec_boolean("has-preview",
						       "Preview supported",
						       "Whether image preview is supported",
						       FALSE,
						       G_PARAM_READWRITE |
						       G_PARAM_CONSTRUCT_ONLY |
						       G_PARAM_STATIC_NAME |
						       G_PARAM_STATIC_NICK |
						       G_PARAM_STATIC_BLURB));
  g_object_class_install_property(object_class,
				  PROP_HAS_SETTINGS,
				  g_param_spec_boolean("has-settings",
						       "Settings supported",
						       "Whether camera settings configuration is supported",
						       FALSE,
						       G_PARAM_READWRITE |
						       G_PARAM_CONSTRUCT_ONLY |
						       G_PARAM_STATIC_NAME |
						       G_PARAM_STATIC_NICK |
						       G_PARAM_STATIC_BLURB));
  fprintf(stderr, "install prog done\n");

  g_type_class_add_private(klass, sizeof(CapaCameraPrivate));
}


CapaCamera *capa_camera_new(const char *model,
			    const char *port,
			    gboolean hasCapture,
			    gboolean hasPreview,
			    gboolean hasSettings)
{
  return CAPA_CAMERA(g_object_new(CAPA_TYPE_CAMERA,
				  "model", model,
				  "port", port,
				  "has-capture", hasCapture,
				  "has-preview", hasPreview,
				  "has-settings", hasSettings,
				  NULL));
}


static void capa_camera_init(CapaCamera *picker)
{
  CapaCameraPrivate *priv;

  priv = picker->priv = CAPA_CAMERA_GET_PRIVATE(picker);
}


const char *capa_camera_model(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;
  return priv->model;
}

const char *capa_camera_port(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;
  return priv->port;
}


static unsigned int do_capa_camera_progress_start(GPContext *ctx G_GNUC_UNUSED,
						  float target,
						  const char *format,
						  va_list args,
						  void *data)
{
  CapaCamera *cam = data;
  CapaCameraPrivate *priv = cam->priv;

  if (priv->progress)
    capa_progress_start(priv->progress, target, format, args);

  return 0; /* XXX what is this actually useful for ? */
}

static void do_capa_camera_progress_update(GPContext *ctx G_GNUC_UNUSED,
					   unsigned int id G_GNUC_UNUSED,
					   float current,
					   void *data)
{
  CapaCamera *cam = data;
  CapaCameraPrivate *priv = cam->priv;

  if (priv->progress)
    capa_progress_update(priv->progress, current);
}

static void do_capa_camera_progress_stop(GPContext *ctx G_GNUC_UNUSED,
					 unsigned int id G_GNUC_UNUSED,
					 void *data)
{
  CapaCamera *cam = data;
  CapaCameraPrivate *priv = cam->priv;

  if (priv->progress)
    capa_progress_stop(priv->progress);
}

static GPContextFeedback do_capa_camera_progress_cancelled(GPContext *ctx G_GNUC_UNUSED,
							   void *data)
{
  CapaCamera *cam = data;
  CapaCameraPrivate *priv = cam->priv;

  fprintf(stderr, "Cancel check\n");

  if (priv->progress &&
      capa_progress_cancelled(priv->progress)) {
    fprintf(stderr, "yes\n");
    return GP_CONTEXT_FEEDBACK_CANCEL;
  }

  fprintf(stderr, "no\n");
  return GP_CONTEXT_FEEDBACK_OK;
}

int capa_camera_connect(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;
  int i;
  GPPortInfo port;
  CameraAbilities cap;

  fprintf(stderr, "Conencting to cam\n");

  if (priv->cam != NULL)
    return 0;

  priv->params = capa_params_new();

  gp_context_set_progress_funcs(priv->params->ctx,
				do_capa_camera_progress_start,
				do_capa_camera_progress_update,
				do_capa_camera_progress_stop,
				cam);

  gp_context_set_cancel_func(priv->params->ctx,
			     do_capa_camera_progress_cancelled,
			     cam);

  i = gp_port_info_list_lookup_path(priv->params->ports, priv->port);
  gp_port_info_list_get_info(priv->params->ports, i, &port);

  i = gp_abilities_list_lookup_model(priv->params->caps, priv->model);
  gp_abilities_list_get_abilities(priv->params->caps, i, &cap);

  gp_camera_new(&priv->cam);
  gp_camera_set_abilities(priv->cam, cap);
  gp_camera_set_port_info(priv->cam, port);

  if (gp_camera_init(priv->cam, priv->params->ctx) != GP_OK) {
    gp_camera_unref(priv->cam);
    priv->cam = NULL;
    fprintf(stderr, "failed\n");
    return -1;
  }

  /* Update capabilities as a sanity-check against orignal constructor */
  priv->hasCapture = priv->hasPreview = priv->hasSettings = FALSE;
  if (cap.operations & GP_OPERATION_CAPTURE_IMAGE)
    priv->hasCapture = TRUE;
  if (cap.operations & GP_OPERATION_CAPTURE_PREVIEW)
    priv->hasPreview = TRUE;
  if (cap.operations & GP_OPERATION_CONFIG)
    priv->hasSettings = TRUE;

  fprintf(stderr, "ok\n");
  return 0;
}

char *capa_camera_summary(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;
  CameraText txt;
  if (priv->cam == NULL)
    return g_strdup("");

  gp_camera_get_summary(priv->cam, &txt, priv->params->ctx);

  return g_strdup(txt.text);
}

char *capa_camera_manual(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;
  CameraText txt;
  if (priv->cam == NULL)
    return g_strdup("");

  gp_camera_get_manual(priv->cam, &txt, priv->params->ctx);

  return g_strdup(txt.text);
}

char *capa_camera_driver(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;
  CameraText txt;
  if (priv->cam == NULL)
    return g_strdup("");

  gp_camera_get_about(priv->cam, &txt, priv->params->ctx);

  return g_strdup(txt.text);
}


static void *do_camera_capture_thread(void *data)
{
  CapaCamera *cam = data;
  CapaCameraPrivate *priv = cam->priv;
  CameraFilePath camerapath;
  CameraFile *datafile = NULL;
  const char *localpath;
  CapaImage *image;

  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-begin");

  fprintf(stderr, "Starting capture\n");
  if (gp_camera_capture(priv->cam, GP_CAPTURE_IMAGE, &camerapath, priv->params->ctx) != GP_OK) {
    fprintf(stderr, "Failed capture\n");
    goto error;
  }

  fprintf(stderr, "captured '%s' '%s'\n", camerapath.folder, camerapath.name);

  gp_file_new(&datafile);

  fprintf(stderr, "Getting file\n");
  if (gp_camera_file_get(priv->cam, camerapath.folder, camerapath.name,
			 GP_FILE_TYPE_NORMAL, datafile, priv->params->ctx) != GP_OK)
    goto error_delete;


  localpath = capa_session_next_filename(priv->session);

  fprintf(stderr, "Saving local file '%s'\n", localpath);
  if (gp_file_save(datafile, localpath) != GP_OK)
    goto error_delete;

  gp_file_unref(datafile);

  fprintf(stderr, "Deleting camera file\n");
  if (gp_camera_file_delete(priv->cam, camerapath.folder, camerapath.name, priv->params->ctx) != GP_OK)
    goto error;

  fprintf(stderr, "Done\n");

  image = capa_image_new(localpath);
  /* XXX don't do this here in future */
  capa_image_load(image);
  capa_session_add(priv->session, image);

  g_signal_emit_by_name(G_OBJECT(cam), "camera-image", image);

  g_object_unref(image);

  priv->operation = NULL;
  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");

  return NULL;

 error_delete:
  fprintf(stderr, "Error, try delete camera file\n");
  gp_camera_file_delete(priv->cam, camerapath.folder, camerapath.name, priv->params->ctx);

 error:
  fprintf(stderr, "Error\n");
  if (datafile)
    gp_file_unref(datafile);
  priv->operation = NULL;
  g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to capture");
  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
  return NULL;
}

int capa_camera_capture(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;

  if (priv->cam == NULL)
    return -1;

  if (priv->operation != NULL)
    return -1;

  if (priv->session == NULL)
    return -1;

  priv->operation = g_thread_create(do_camera_capture_thread, cam, FALSE, NULL);

  return 0;
}

static void *do_camera_preview_thread(void *data)
{
  CapaCamera *cam = data;
  CapaCameraPrivate *priv = cam->priv;
  CameraFile *datafile = NULL;
  const char *localpath;
  CapaImage *image;

  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-begin");
  gp_file_new(&datafile);

  fprintf(stderr, "Starting preview\n");
  if (gp_camera_capture_preview(priv->cam, datafile, priv->params->ctx) != GP_OK) {
    fprintf(stderr, "Failed capture\n");
    goto error;
  }

  localpath = capa_session_temp_filename(priv->session),

  fprintf(stderr, "Saving file '%s'\n", localpath);
  if (gp_file_save(datafile, localpath) != GP_OK)
    goto error;

  gp_file_unref(datafile);

  image = capa_image_new(localpath);

  g_signal_emit_by_name(G_OBJECT(cam), "camera-image", image);

  g_object_unref(image);
  unlink(localpath);

  fprintf(stderr, "Done\n");
  priv->operation = NULL;
  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
  return NULL;

 error:
  fprintf(stderr, "Error\n");
  if (datafile)
    gp_file_unref(datafile);
  priv->operation = NULL;
  g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to preview");
  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
  return NULL;
}


int capa_camera_preview(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;

  if (priv->cam == NULL)
    return -1;

  if (priv->operation != NULL)
    return -1;

  if (priv->session == NULL)
    return -1;

  priv->operation = g_thread_create(do_camera_preview_thread, cam, FALSE, NULL);

  return 0;
}


static int do_camera_file_added(CapaCamera *cam,
				CameraFilePath *camerapath)
{
  CapaCameraPrivate *priv = cam->priv;
  CameraFile *datafile = NULL;
  const char *localpath;
  CapaImage *image;

  fprintf(stderr, "captured '%s' '%s'\n", camerapath->folder, camerapath->name);

  gp_file_new(&datafile);

  fprintf(stderr, "Getting file\n");
  if (gp_camera_file_get(priv->cam, camerapath->folder, camerapath->name,
			 GP_FILE_TYPE_NORMAL, datafile, priv->params->ctx) != GP_OK)
    goto error_delete;

  localpath = capa_session_next_filename(priv->session);

  fprintf(stderr, "Saving local file '%s'\n", localpath);
  if (gp_file_save(datafile, localpath) != GP_OK)
    goto error_delete;

  gp_file_unref(datafile);

  fprintf(stderr, "Deleting camera file\n");
  /* XXX should we really do this TBD ? */
  if (gp_camera_file_delete(priv->cam, camerapath->folder, camerapath->name, priv->params->ctx) != GP_OK)
    goto error;

  fprintf(stderr, "Done\n");

  image = capa_image_new(localpath);
  capa_session_add(priv->session, image);

  g_signal_emit_by_name(G_OBJECT(cam), "camera-image", image);

  g_object_unref(image);

  return 0;

 error_delete:
  fprintf(stderr, "Error, try delete camera file\n");
  gp_camera_file_delete(priv->cam, camerapath->folder, camerapath->name, priv->params->ctx);

 error:
  fprintf(stderr, "Error\n");
  if (datafile)
    gp_file_unref(datafile);
  return -1;
}

static void *do_camera_monitor_thread(void *data)
{
  CapaCamera *cam = data;
  CapaCameraPrivate *priv = cam->priv;
  CameraEventType eventType;
  void *eventData;

  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-begin");

  fprintf(stderr, "Starting monitor\n");
  while (!capa_progress_cancelled(priv->progress)) {
    if (gp_camera_wait_for_event(priv->cam, 500, &eventType, &eventData, priv->params->ctx) != GP_OK) {
      fprintf(stderr, "Failed capture\n");
      g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to wait for events");
      goto cleanup;
    }

    switch (eventType) {
    case GP_EVENT_TIMEOUT:
      /* We just use timeouts to check progress cancellation */
      break;

    case GP_EVENT_FOLDER_ADDED:
      /* Don't care about this */
      break;

    case GP_EVENT_FILE_ADDED:
      if (do_camera_file_added(cam, eventData) < 0) {
	g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to process file");
	goto cleanup;
      }
      break;

    case GP_EVENT_UNKNOWN:
    default:
      g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unexpected/unknown camera event");
      fprintf(stderr, "Unknown event type %d\n", eventType);
      goto cleanup;
    }
  }

 cleanup:
  priv->operation = NULL;
  g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");

  return NULL;
}


int capa_camera_monitor(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;

  if (priv->cam == NULL)
    return -1;

  if (priv->operation != NULL)
    return -1;

  if (priv->session == NULL)
    return -1;

  /* We need a progress widget in order todo cancellation */
  if (priv->progress == NULL)
    return -1;

  priv->operation = g_thread_create(do_camera_monitor_thread, cam, FALSE, NULL);

  return 0;
}

static int do_process_control(CapaControlGroup *grp,
			      const char *path,
			      CameraWidget *widget)
{
  CameraWidgetType type;
  const char *name;
  char *fullpath;
  int id;
  const char *label;

  if (gp_widget_get_type(widget, &type) != GP_OK)
    return -1;

  if (gp_widget_get_name(widget, &name) != GP_OK)
    return -1;

  gp_widget_get_id(widget, &id);
  gp_widget_get_label(widget, &label);

  fullpath = g_strdup_printf("%s/%s", path, name);

  switch (type) {
  case GP_WIDGET_WINDOW:
    {
      fprintf(stderr, "Toplevel %s\n", fullpath);
      for (int i = 0 ; i < gp_widget_count_children(widget) ; i++) {
	CameraWidget *child;
	if (gp_widget_get_child(widget, i, &child) != GP_OK)
	  goto error;
	if (do_process_control(grp, fullpath, child) < 0)
	  goto error;
      }
    } break;

  case GP_WIDGET_SECTION:
    {
      CapaControlGroup *subgrp;
      subgrp = capa_control_group_new(fullpath, id, label);

      fprintf(stderr, "Recurse %s\n", fullpath);
      for (int i = 0 ; i < gp_widget_count_children(widget) ; i++) {
	CameraWidget *child;
	if (gp_widget_get_child(widget, i, &child) != GP_OK) {
	  g_object_unref(G_OBJECT(subgrp));
	  goto error;
	}
	if (do_process_control(subgrp, fullpath, child) < 0) {
	  g_object_unref(G_OBJECT(subgrp));
	  goto error;
	}
      }

      capa_control_group_add(grp, CAPA_CONTROL(subgrp));
    } break;

  default:
    {
      CapaControl *control;

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
  CapaCameraPrivate *priv = cam->priv;
  CameraWidget *win = NULL;
  CapaControlGroup *group;

  if (priv->cam == NULL)
    return NULL;

  if (gp_camera_get_config(priv->cam, &win, priv->params->ctx) != GP_OK)
    return NULL;

  group = capa_control_group_new("/", 0, "Settings");

  if (do_process_control(group, "", win) < 0) {
    g_object_unref(group);
    group = NULL;
  }

  gp_widget_unref(win);

  return group;
}

gboolean capa_camera_has_capture(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;

  return priv->hasCapture;
}

gboolean capa_camera_has_preview(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;

  return priv->hasPreview;
}

gboolean capa_camera_has_settings(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;

  return priv->hasSettings;
}


CapaSession *capa_camera_session(CapaCamera *cam)
{
  CapaCameraPrivate *priv = cam->priv;

  return priv->session;
}
