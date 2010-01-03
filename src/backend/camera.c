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

#include "internal.h"
#include "camera.h"
#include "params.h"
#include "progress.h"
#include "control-button.h"
#include "control-choice.h"
#include "control-date.h"
#include "control-group.h"
#include "control-range.h"
#include "control-text.h"
#include "control-toggle.h"

#define CAPA_CAMERA_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA, CapaCameraPrivate))

struct _CapaCameraPrivate {
    CapaParams *params;
    Camera *cam;

    CameraWidget *widgets;
    CapaControlGroup *controls;

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

static void (*threads_enter_impl)(void) = NULL;
static void (*threads_leave_impl)(void) = NULL;

static void capa_camera_threads_enter(void)
{
    if (threads_enter_impl)
        (threads_enter_impl)();
}

static void capa_camera_threads_leave(void)
{
    if (threads_leave_impl)
        (threads_leave_impl)();
}

void capa_camera_set_thread_funcs(void (*threads_enter)(void),
                                  void (*threads_leave)(void))
{
    threads_enter_impl = threads_enter;
    threads_leave_impl = threads_leave;
}

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
            if (priv->session)
                g_object_ref(priv->session);
            break;

        case PROP_PROGRESS:
            if (priv->progress)
                g_object_unref(G_OBJECT(priv->progress));
            priv->progress = g_value_get_object(value);
            if (priv->progress)
                g_object_ref(priv->progress);
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
    if (priv->session)
        g_object_unref(priv->session);
    if (priv->cam) {
        gp_camera_exit(priv->cam, priv->params->ctx);
        gp_camera_free(priv->cam);
    }
    if (priv->widgets)
        gp_widget_unref(priv->widgets);
    if (priv->controls)
        g_object_unref(priv->controls);
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

int capa_camera_connect(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;
    int i;
    GPPortInfo port;
    CameraAbilities cap;

    CAPA_DEBUG("Conencting to cam");

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
        CAPA_DEBUG("failed");
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

    CAPA_DEBUG("ok");
    return 0;
}

int capa_camera_disconnect(CapaCamera *cam)
{
    CapaCameraPrivate *priv = cam->priv;

    CAPA_DEBUG("Disconnecting from cam");

    if (priv->cam == NULL)
        return 0;

    gp_camera_exit(priv->cam, priv->params->ctx);

    if (priv->widgets) {
        gp_widget_unref(priv->widgets);
        priv->widgets = NULL;
    }
    if (priv->controls) {
        g_object_unref(priv->controls);
        priv->controls = NULL;
    }

    capa_params_free(priv->params);
    priv->params = NULL;

    gp_camera_unref(priv->cam);
    priv->cam = NULL;

    priv->hasCapture = priv->hasPreview = priv->hasSettings = FALSE;

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

    capa_camera_threads_enter();
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-begin");
    capa_camera_threads_leave();

    CAPA_DEBUG("Starting capture");
    if (gp_camera_capture(priv->cam, GP_CAPTURE_IMAGE, &camerapath, priv->params->ctx) != GP_OK) {
        CAPA_DEBUG("Failed capture");
        goto error;
    }

    CAPA_DEBUG("captured '%s' '%s'", camerapath.folder, camerapath.name);

    gp_file_new(&datafile);

    CAPA_DEBUG("Getting file");
    if (gp_camera_file_get(priv->cam, camerapath.folder, camerapath.name,
                           GP_FILE_TYPE_NORMAL, datafile, priv->params->ctx) != GP_OK)
        goto error_delete;


    localpath = capa_session_next_filename(priv->session);

    CAPA_DEBUG("Saving local file '%s'", localpath);
    if (gp_file_save(datafile, localpath) != GP_OK)
        goto error_delete;

    gp_file_unref(datafile);

    CAPA_DEBUG("Deleting camera file");
    if (gp_camera_file_delete(priv->cam, camerapath.folder, camerapath.name, priv->params->ctx) != GP_OK)
        goto error;

    CAPA_DEBUG("Done");

    image = capa_image_new(localpath);

    capa_camera_threads_enter();
    capa_session_add(priv->session, image);
    g_signal_emit_by_name(G_OBJECT(cam), "camera-image", image);

    g_object_unref(image);

    priv->operation = NULL;
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
    capa_camera_threads_leave();

    return NULL;

 error_delete:
    CAPA_DEBUG("Error, try delete camera file");
    gp_camera_file_delete(priv->cam, camerapath.folder, camerapath.name, priv->params->ctx);

 error:
    CAPA_DEBUG("Error");
    if (datafile)
        gp_file_unref(datafile);
    priv->operation = NULL;
    capa_camera_threads_enter();
    g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to capture");
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
    capa_camera_threads_leave();
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

    capa_camera_threads_enter();
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-begin");
    capa_camera_threads_leave();
    gp_file_new(&datafile);

    CAPA_DEBUG("Starting preview");
    if (gp_camera_capture_preview(priv->cam, datafile, priv->params->ctx) != GP_OK) {
        CAPA_DEBUG("Failed capture");
        goto error;
    }

    localpath = capa_session_temp_filename(priv->session);

    CAPA_DEBUG("Saving file '%s'", localpath);
    if (gp_file_save(datafile, localpath) != GP_OK)
        goto error;

    gp_file_unref(datafile);

    image = capa_image_new(localpath);

    capa_camera_threads_enter();
    g_signal_emit_by_name(G_OBJECT(cam), "camera-image", image);

    g_object_unref(image);
    unlink(localpath);

    CAPA_DEBUG("Done");
    priv->operation = NULL;
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
    capa_camera_threads_leave();
    return NULL;

 error:
    CAPA_DEBUG("Error");
    if (datafile)
        gp_file_unref(datafile);
    priv->operation = NULL;
    capa_camera_threads_enter();
    g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to preview");
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
    capa_camera_threads_leave();
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

    CAPA_DEBUG("captured '%s' '%s'", camerapath->folder, camerapath->name);

    gp_file_new(&datafile);

    CAPA_DEBUG("Getting file");
    if (gp_camera_file_get(priv->cam, camerapath->folder, camerapath->name,
                           GP_FILE_TYPE_NORMAL, datafile, priv->params->ctx) != GP_OK)
        goto error_delete;

    localpath = capa_session_next_filename(priv->session);

    CAPA_DEBUG("Saving local file '%s'", localpath);
    if (gp_file_save(datafile, localpath) != GP_OK)
        goto error_delete;

    gp_file_unref(datafile);

    CAPA_DEBUG("Deleting camera file");
    /* XXX should we really do this TBD ? */
    if (gp_camera_file_delete(priv->cam, camerapath->folder, camerapath->name, priv->params->ctx) != GP_OK)
        goto error;

    CAPA_DEBUG("Done");

    image = capa_image_new(localpath);

    capa_camera_threads_enter();
    capa_session_add(priv->session, image);
    g_signal_emit_by_name(G_OBJECT(cam), "camera-image", image);
    capa_camera_threads_leave();

    g_object_unref(image);

    return 0;

 error_delete:
    CAPA_DEBUG("Error, try delete camera file");
    gp_camera_file_delete(priv->cam, camerapath->folder, camerapath->name, priv->params->ctx);

 error:
    CAPA_DEBUG("Error");
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

    capa_camera_threads_enter();
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-begin");
    capa_camera_threads_leave();

    CAPA_DEBUG("Starting monitor");
    while (priv->progress && !capa_progress_cancelled(priv->progress)) {
        if (gp_camera_wait_for_event(priv->cam, 500, &eventType, &eventData, priv->params->ctx) != GP_OK) {
            CAPA_DEBUG("Failed capture");
            capa_camera_threads_enter();
            g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to wait for events");
            capa_camera_threads_leave();
            goto cleanup;
        }
        CAPA_DEBUG("Foo %d '%s'", eventType, (char *)eventData);
        switch (eventType) {
        case GP_EVENT_TIMEOUT:
            /* We just use timeouts to check progress cancellation */
            break;

        case GP_EVENT_FOLDER_ADDED:
            /* Don't care about this */
            break;

        case GP_EVENT_FILE_ADDED:
            if (do_camera_file_added(cam, eventData) < 0) {
                capa_camera_threads_enter();
                g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unable to process file");
                capa_camera_threads_leave();
                goto cleanup;
            }
            break;

        case GP_EVENT_UNKNOWN:
        default:
            capa_camera_threads_enter();
            g_signal_emit_by_name(G_OBJECT(cam), "camera-error", "Unexpected/unknown camera event");
            capa_camera_threads_leave();
            CAPA_DEBUG("Unknown event type %d", eventType);
            goto cleanup;
        }
    }

 cleanup:
    priv->operation = NULL;
    capa_camera_threads_enter();
    g_signal_emit_by_name(G_OBJECT(cam), "camera-op-end");
    capa_camera_threads_leave();

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


CapaControlGroup *capa_camera_controls(CapaCamera *cam)
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

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
