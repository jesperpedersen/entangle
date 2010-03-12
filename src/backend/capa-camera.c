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

#include <config.h>

#include <glib.h>
#include <stdio.h>
#include <unistd.h>

#include "capa-debug.h"
#include "capa-camera.h"
#include "capa-params.h"
#include "capa-control-button.h"
#include "capa-control-choice.h"
#include "capa-control-date.h"
#include "capa-control-group.h"
#include "capa-control-range.h"
#include "capa-control-text.h"
#include "capa-control-toggle.h"

#define CAPA_CAMERA_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA, CapaCameraPrivate))

struct _CapaCameraPrivate {
    CapaParams *params;
    Camera *cam;

    CameraWidget *widgets;
    CapaControlGroup *controls;

    CapaProgress *progress;

    char *model;
    char *port;

    char *manual;
    char *summary;
    char *driver;

    gboolean hasCapture;
    gboolean hasPreview;
    gboolean hasSettings;
};

G_DEFINE_TYPE(CapaCamera, capa_camera, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_MODEL,
    PROP_PORT,
    PROP_MANUAL,
    PROP_SUMMARY,
    PROP_DRIVER,
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
    CapaCamera *cam = CAPA_CAMERA(object);
    CapaCameraPrivate *priv = cam->priv;

    switch (prop_id)
        {
        case PROP_MODEL:
            g_value_set_string(value, priv->model);
            break;

        case PROP_PORT:
            g_value_set_string(value, priv->port);
            break;

        case PROP_MANUAL:
            g_value_set_string(value, priv->manual);
            break;

        case PROP_SUMMARY:
            g_value_set_string(value, priv->summary);
            break;

        case PROP_DRIVER:
            g_value_set_string(value, priv->driver);
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
    CapaCamera *cam = CAPA_CAMERA(object);
    CapaCameraPrivate *priv = cam->priv;

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

        case PROP_PROGRESS:
            capa_camera_set_progress(cam, g_value_get_object(value));
            break;

        case PROP_HAS_CAPTURE:
            priv->hasCapture = g_value_get_boolean(value);
            CAPA_DEBUG("Set has capture %d", priv->hasCapture);
            break;

        case PROP_HAS_PREVIEW:
            priv->hasPreview = g_value_get_boolean(value);
            CAPA_DEBUG("Set has preview %d", priv->hasPreview);
            break;

        case PROP_HAS_SETTINGS:
            priv->hasSettings = g_value_get_boolean(value);
            CAPA_DEBUG("Set has settings %d", priv->hasSettings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void capa_camera_finalize(GObject *object)
{
    CapaCamera *camera = CAPA_CAMERA(object);
    CapaCameraPrivate *priv = camera->priv;

    CAPA_DEBUG("Finalize camera %p", object);

    if (priv->progress)
        g_object_unref(priv->progress);
    if (priv->cam) {
        gp_camera_exit(priv->cam, priv->params->ctx);
        gp_camera_free(priv->cam);
    }
    if (priv->widgets)
        gp_widget_unref(priv->widgets);
    if (priv->controls)
        g_object_unref(priv->controls);
    capa_params_free(priv->params);
    g_free(priv->driver);
    g_free(priv->summary);
    g_free(priv->manual);
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


    g_signal_new("camera-file-added",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraClass, camera_file_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 CAPA_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-captured",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraClass, camera_file_captured),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 CAPA_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-previewed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraClass, camera_file_previewed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 CAPA_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-downloaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraClass, camera_file_downloaded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 CAPA_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-deleted",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraClass, camera_file_deleted),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 CAPA_TYPE_CAMERA_FILE);


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
                                    PROP_SUMMARY,
                                    g_param_spec_string("summary",
                                                        "Camera summary",
                                                        "Camera summary",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_MANUAL,
                                    g_param_spec_string("manual",
                                                        "Camera manual",
                                                        "Camera manual",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_DRIVER,
                                    g_param_spec_string("driver",
                                                        "Camera driver info",
                                                        "Camera driver information",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PROGRESS,
                                    g_param_spec_object("progress",
                                                        "Progress updater",
                                                        "Operation progress updater",
                                                        CAPA_TYPE_PROGRESS,
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
    CAPA_DEBUG("install prog done");

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


const char *capa_camera_get_model(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;
    return priv->model;
}

const char *capa_camera_get_port(CapaCamera *cam)
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

    CAPA_DEBUG("Cancel check");

    if (!priv->progress)
        return GP_CONTEXT_FEEDBACK_CANCEL;

    if (capa_progress_cancelled(priv->progress)) {
        CAPA_DEBUG("yes");
        return GP_CONTEXT_FEEDBACK_CANCEL;
    }

    CAPA_DEBUG("no");
    return GP_CONTEXT_FEEDBACK_OK;
}


gboolean capa_camera_connect(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;
    int i;
    GPPortInfo port;
    CameraAbilities cap;
    CameraText txt;

    CAPA_DEBUG("Conencting to cam");

    if (priv->cam != NULL)
        return TRUE;

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
        CAPA_DEBUG("failed");
        return FALSE;
    }

    /* Update capabilities as a sanity-check against orignal constructor */
    priv->hasCapture = priv->hasPreview = priv->hasSettings = FALSE;
    if (cap.operations & GP_OPERATION_CAPTURE_IMAGE)
        priv->hasCapture = TRUE;
    if (cap.operations & GP_OPERATION_CAPTURE_PREVIEW)
        priv->hasPreview = TRUE;
    if (cap.operations & GP_OPERATION_CONFIG)
        priv->hasSettings = TRUE;

    gp_camera_get_summary(priv->cam, &txt, priv->params->ctx);
    priv->summary = g_strdup(txt.text);

    gp_camera_get_manual(priv->cam, &txt, priv->params->ctx);
    priv->manual = g_strdup(txt.text);

    gp_camera_get_about(priv->cam, &txt, priv->params->ctx);
    priv->driver = g_strdup(txt.text);

    CAPA_DEBUG("ok");
    return TRUE;
}

gboolean capa_camera_disconnect(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    CAPA_DEBUG("Disconnecting from cam");

    if (priv->cam == NULL)
        return TRUE;

    gp_camera_exit(priv->cam, priv->params->ctx);

    if (priv->widgets) {
        gp_widget_unref(priv->widgets);
        priv->widgets = NULL;
    }
    if (priv->controls) {
        g_object_unref(priv->controls);
        priv->controls = NULL;
    }

    g_free(priv->driver);
    g_free(priv->manual);
    g_free(priv->summary);
    priv->driver = priv->manual = priv->summary = NULL;

    capa_params_free(priv->params);
    priv->params = NULL;

    gp_camera_unref(priv->cam);
    priv->cam = NULL;

    priv->hasCapture = priv->hasPreview = priv->hasSettings = FALSE;

    return TRUE;
}

const char *capa_camera_get_summary(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    return priv->summary;
}

const char *capa_camera_get_manual(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    return priv->manual;
}

const char *capa_camera_get_driver(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    return priv->driver;
}


CapaCameraFile *capa_camera_capture_image(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;
    CameraFilePath camerapath;
    CapaCameraFile *file;

    CAPA_DEBUG("Starting capture");
    if (gp_camera_capture(priv->cam,
                          GP_CAPTURE_IMAGE,
                          &camerapath,
                          priv->params->ctx) != GP_OK)
        return NULL;

    file = capa_camera_file_new(camerapath.folder,
                                camerapath.name);

    g_signal_emit_by_name(cam, "camera-file-captured", file);

    return file;
}


CapaCameraFile *capa_camera_preview_image(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;
    CapaCameraFile *file;
    CameraFile *datafile = NULL;
    const char *mimetype = NULL;
    GByteArray *data = NULL;
    const char *rawdata;
    unsigned long int rawdatalen;
    const char *name;

    gp_file_new(&datafile);

    CAPA_DEBUG("Starting preview");
    if (gp_camera_capture_preview(priv->cam,
                                  datafile,
                                  priv->params->ctx) != GP_OK) {
        CAPA_DEBUG("Failed capture");
        goto error;
    }

    if (gp_file_get_data_and_size(datafile, &rawdata, &rawdatalen) != GP_OK)
        goto error;

    if (gp_file_get_name(datafile, &name) != GP_OK)
        goto error;

    file = capa_camera_file_new("/", name);

    if (gp_file_get_mime_type(datafile, &mimetype) == GP_OK)
        g_object_set(file, "mimetype", mimetype, NULL);

    data = g_byte_array_new();
    g_byte_array_append(data, (const guint8 *)rawdata, rawdatalen);

    g_object_set(file, "data", data, NULL);
    g_byte_array_unref(data);

    gp_file_unref(datafile);

    g_signal_emit_by_name(cam, "camera-file-previewed", file);

    return file;

 error:
    if (datafile)
        gp_file_unref(datafile);
    return NULL;
}


gboolean capa_camera_download_file(CapaCamera *cam,
                                   CapaCameraFile *file)
{
    CapaCameraPrivate *priv = cam->priv;
    CameraFile *datafile = NULL;
    const char *data;
    unsigned long int datalen;
    GByteArray *filedata;

    CAPA_DEBUG("Downloading '%s' from '%s'",
               capa_camera_file_get_name(file),
               capa_camera_file_get_folder(file));

    gp_file_new(&datafile);

    CAPA_DEBUG("Getting file data");
    if (gp_camera_file_get(priv->cam,
                           capa_camera_file_get_folder(file),
                           capa_camera_file_get_name(file),
                           GP_FILE_TYPE_NORMAL,
                           datafile,
                           priv->params->ctx) != GP_OK)
        goto error;

    CAPA_DEBUG("Fetching data");
    if (gp_file_get_data_and_size(datafile, &data, &datalen) != GP_OK)
        goto error;

    filedata = g_byte_array_new();
    g_byte_array_append(filedata, (const guint8*)data, datalen);
    gp_file_unref(datafile);


    g_object_set(file, "data", filedata, NULL);
    g_byte_array_unref(filedata);

    g_signal_emit_by_name(cam, "camera-file-downloaded", file);

    return TRUE;

 error:
    CAPA_DEBUG("Error");
    if (datafile)
        gp_file_unref(datafile);
    return FALSE;
}


gboolean capa_camera_delete_file(CapaCamera *cam,
                                 CapaCameraFile *file)
{
    CapaCameraPrivate *priv = cam->priv;

    CAPA_DEBUG("Deleting '%s' from '%s'",
               capa_camera_file_get_name(file),
               capa_camera_file_get_folder(file));

    if (gp_camera_file_delete(priv->cam,
                              capa_camera_file_get_folder(file),
                              capa_camera_file_get_name(file),
                              priv->params->ctx) != GP_OK)
        return FALSE;

    g_signal_emit_by_name(cam, "camera-file-deleted", file);

    return TRUE;
}


gboolean capa_camera_event_flush(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;
    CameraEventType eventType;
    void *eventData;

    CAPA_DEBUG("Flushing events");

    do {
        if (gp_camera_wait_for_event(priv->cam, 10, &eventType, &eventData, priv->params->ctx) != GP_OK) {
            CAPA_DEBUG("Failed event wait");
            return FALSE;
        }

    } while (eventType != GP_EVENT_TIMEOUT);

    return TRUE;
}


gboolean capa_camera_event_wait(CapaCamera *cam,
                                int waitms)
{
    CapaCameraPrivate *priv = cam->priv;
    CameraEventType eventType;
    void *eventData;
    gboolean ret = TRUE;

    CAPA_DEBUG("Waiting for events");

    do {
        if (gp_camera_wait_for_event(priv->cam, waitms, &eventType, &eventData, priv->params->ctx) != GP_OK) {
            CAPA_DEBUG("Failed event wait");
            return FALSE;
        }

        switch (eventType) {
        case GP_EVENT_UNKNOWN:
            CAPA_DEBUG("Unknown event '%s'", (char *)eventData);
            break;

        case GP_EVENT_TIMEOUT:
            break;

        case GP_EVENT_FILE_ADDED: {
            CameraFilePath *camerapath = eventData;
            CapaCameraFile *file;

            CAPA_DEBUG("File added '%s' in '%s'", camerapath->name, camerapath->folder);

            file = capa_camera_file_new(camerapath->folder,
                                        camerapath->name);

            g_signal_emit_by_name(cam, "camera-file-added", file);

            g_object_unref(file);
        }   break;

        case GP_EVENT_FOLDER_ADDED: {
            CameraFilePath *camerapath = eventData;

            CAPA_DEBUG("Folder added '%s' in '%s'", camerapath->name, camerapath->folder);
        }   break;

        default:
            CAPA_DEBUG("Unexpected event received %d", eventType);
            ret = FALSE;
            break;
        }
    } while (ret && eventType != GP_EVENT_TIMEOUT);

    return ret;
}


static void do_update_control_text(GObject *object,
                                   GParamSpec *param G_GNUC_UNUSED,
                                   void *data)
{
    CapaCamera *cam = data;
    CapaCameraPrivate *priv = cam->priv;
    char *text;
    char *path;
    int id;
    CameraWidget *widget;

    g_object_get(object, "path", &path, "id", &id, "value", &text, NULL);
    CAPA_DEBUG("update of widget %s", path);

    if (gp_widget_get_child_by_id(priv->widgets, id, &widget) != GP_OK) {
        CAPA_DEBUG("cannot get widget id %d", id);
        return;
    }

    if (gp_widget_set_value(widget, text) != GP_OK) {
        CAPA_DEBUG("cannot set widget id %d to %s", id, text);
    }

    if (gp_camera_set_config(priv->cam, priv->widgets, priv->params->ctx) != GP_OK)
        CAPA_DEBUG("cannot set config");

}

static void do_update_control_float(GObject *object,
                                    GParamSpec *param G_GNUC_UNUSED,
                                    void *data)
{
    CapaCamera *cam = data;
    CapaCameraPrivate *priv = cam->priv;
    float value;
    char *path;
    int id;
    CameraWidget *widget;

    g_object_get(object, "path", &path, "id", &id, "value", &value, NULL);
    CAPA_DEBUG("update of widget %s", path);

    if (gp_widget_get_child_by_id(priv->widgets, id, &widget) != GP_OK) {
        CAPA_DEBUG("cannot get widget id %d", id);
        return;
    }

    if (gp_widget_set_value(widget, &value) != GP_OK) {
        CAPA_DEBUG("cannot set widget id %d to %f", id, value);
    }

    if (gp_camera_set_config(priv->cam, priv->widgets, priv->params->ctx) != GP_OK)
        CAPA_DEBUG("cannot set config");

}

static void do_update_control_boolean(GObject *object,
                                      GParamSpec *param G_GNUC_UNUSED,
                                      void *data)
{
    CapaCamera *cam = data;
    CapaCameraPrivate *priv = cam->priv;
    gboolean value;
    char *path;
    int id;
    CameraWidget *widget;

    g_object_get(object, "path", &path, "id", &id, "value", &value, NULL);
    CAPA_DEBUG("update of widget %s", path);

    if (gp_widget_get_child_by_id(priv->widgets, id, &widget) != GP_OK) {
        CAPA_DEBUG("cannot get widget id %d", id);
        return;
    }

    if (gp_widget_set_value(widget, &value) != GP_OK) {
        CAPA_DEBUG("cannot set widget id %d to %d", id, value);
    }

    if (gp_camera_set_config(priv->cam, priv->widgets, priv->params->ctx) != GP_OK)
        CAPA_DEBUG("cannot set config");

}

static CapaControl *do_build_controls(CapaCamera *cam,
                                      const char *path,
                                      CameraWidget *widget)
{
    CameraWidgetType type;
    CapaControl *ret = NULL;
    const char *name;
    char *fullpath;
    int id;
    const char *label;
    const char *info;

    if (gp_widget_get_type(widget, &type) != GP_OK)
        return NULL;

    if (gp_widget_get_name(widget, &name) != GP_OK)
        return NULL;

    gp_widget_get_id(widget, &id);
    gp_widget_get_label(widget, &label);
    gp_widget_get_info(widget, &info);
    if (info == NULL)
        info = label;

    fullpath = g_strdup_printf("%s/%s", path, name);

    switch (type) {
        /* We treat both window and section as just groups */
    case GP_WIDGET_WINDOW:
    case GP_WIDGET_SECTION:
        {
            CapaControlGroup *grp;
            CAPA_DEBUG("Add group %s %d %s", fullpath, id, label);
            grp = capa_control_group_new(fullpath, id, label, info);
            for (int i = 0 ; i < gp_widget_count_children(widget) ; i++) {
                CameraWidget *child;
                CapaControl *subctl;
                if (gp_widget_get_child(widget, i, &child) != GP_OK) {
                    g_object_unref(G_OBJECT(grp));
                    goto error;
                }
                if (!(subctl = do_build_controls(cam, fullpath, child))) {
                    g_object_unref(G_OBJECT(grp));
                    goto error;
                }

                capa_control_group_add(grp, subctl);
            }

            ret = CAPA_CONTROL(grp);
        } break;

    case GP_WIDGET_BUTTON:
        {
            CAPA_DEBUG("Add button %s %d %s", fullpath, id, label);
            ret = CAPA_CONTROL(capa_control_button_new(fullpath, id, label, info));
        } break;

        /* Unclear why these two are the same in libgphoto */
    case GP_WIDGET_RADIO:
    case GP_WIDGET_MENU:
        {
            char *value = NULL;
            CAPA_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = CAPA_CONTROL(capa_control_choice_new(fullpath, id, label, info));

            for (int i = 0 ; i < gp_widget_count_choices(widget) ; i++) {
                const char *choice;
                gp_widget_get_choice(widget, i, &choice);
                capa_control_choice_add_entry(CAPA_CONTROL_CHOICE(ret), choice);
            }

            gp_widget_get_value(widget, &value);
            g_object_set(G_OBJECT(ret), "value", value, NULL);
            g_signal_connect(G_OBJECT(ret), "notify::value",
                             G_CALLBACK(do_update_control_text), cam);
        } break;

    case GP_WIDGET_DATE:
        {
            int value = 0;
            CAPA_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = CAPA_CONTROL(capa_control_date_new(fullpath, id, label, info));
            g_object_set(G_OBJECT(ret), "value", value, NULL);
        } break;

    case GP_WIDGET_RANGE:
        {
            float min, max, step;
            float value = 0.0;
            gp_widget_get_range(widget, &min, &max, &step);
            CAPA_DEBUG("Add range %s %d %s %f %f %f", fullpath, id, label, min, max, step);
            ret = CAPA_CONTROL(capa_control_range_new(fullpath, id, label, info,
                                                      min, max, step));

            gp_widget_get_value(widget, &value);
            g_object_set(G_OBJECT(ret), "value", value, NULL);
            g_signal_connect(G_OBJECT(ret), "notify::value",
                             G_CALLBACK(do_update_control_float), cam);
        } break;

    case GP_WIDGET_TEXT:
        {
            char *value = NULL;
            CAPA_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = CAPA_CONTROL(capa_control_text_new(fullpath, id, label, info));

            gp_widget_get_value(widget, &value);
            g_object_set(G_OBJECT(ret), "value", value, NULL);
            g_signal_connect(G_OBJECT(ret), "notify::value",
                             G_CALLBACK(do_update_control_text), cam);
        } break;

    case GP_WIDGET_TOGGLE:
        {
            int value = 0;
            CAPA_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = CAPA_CONTROL(capa_control_toggle_new(fullpath, id, label, info));

            gp_widget_get_value(widget, &value);
            g_object_set(G_OBJECT(ret), "value", (gboolean)value, NULL);
            g_signal_connect(G_OBJECT(ret), "notify::value",
                             G_CALLBACK(do_update_control_boolean), cam);
        } break;
    }

 error:
    g_free(fullpath);
    return ret;
}


CapaControlGroup *capa_camera_get_controls(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    if (priv->cam == NULL)
        return NULL;

    if (priv->controls == NULL) {
        if (gp_camera_get_config(priv->cam, &priv->widgets, priv->params->ctx) != GP_OK)
            return NULL;

        priv->controls = CAPA_CONTROL_GROUP(do_build_controls(cam, "", priv->widgets));
    }

    return priv->controls;
}

gboolean capa_camera_get_has_capture(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    return priv->hasCapture;
}

gboolean capa_camera_get_has_preview(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    return priv->hasPreview;
}

gboolean capa_camera_get_has_settings(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    return priv->hasSettings;
}


void capa_camera_set_progress(CapaCamera *cam, CapaProgress *prog)
{
    CapaCameraPrivate *priv = cam->priv;

    if (priv->progress)
        g_object_unref(priv->progress);
    priv->progress = prog;
    if (priv->progress)
        g_object_ref(priv->progress);
}


CapaProgress *capa_camera_get_progress(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    return priv->progress;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
