/*
 *  Entangle: Entangle Assists Photograph Aquisition
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
#include <glib/gthread.h>
#include <stdio.h>
#include <unistd.h>
#include <gphoto2.h>

#include "entangle-debug.h"
#include "entangle-camera.h"
#include "entangle-control-button.h"
#include "entangle-control-choice.h"
#include "entangle-control-date.h"
#include "entangle-control-group.h"
#include "entangle-control-range.h"
#include "entangle-control-text.h"
#include "entangle-control-toggle.h"

#define ENTANGLE_CAMERA_GET_PRIVATE(obj)                                    \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA, EntangleCameraPrivate))

#define ENTANGLE_ERROR(err, msg...)                                     \
    g_set_error((err),                                                  \
                g_quark_from_string("entangle-camera"),                 \
                0,                                                      \
                msg)

struct _EntangleCameraPrivate {
    GMutex *lock;
    gboolean inJob;
    GCond *cond;

    GPContext *ctx;
    CameraAbilitiesList *caps;
    GPPortInfoList *ports;
    Camera *cam;
    

    CameraWidget *widgets;
    EntangleControlGroup *controls;

    EntangleProgress *progress;

    char *lastError;

    char *model;  /* R/O */
    char *port;   /* R/O */

    char *manual;
    char *summary;
    char *driver;

    gboolean hasCapture;
    gboolean hasPreview;
    gboolean hasSettings;
};

G_DEFINE_TYPE(EntangleCamera, entangle_camera, G_TYPE_OBJECT);

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


static void entangle_camera_begin_job(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;

    g_object_ref(cam);

    while (priv->inJob)
        g_cond_wait(priv->cond, priv->lock);

    priv->inJob = TRUE;
    g_mutex_unlock(priv->lock);
}


static void entangle_camera_end_job(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;

    g_mutex_lock(priv->lock);
    priv->inJob = FALSE;
    g_cond_signal(priv->cond);
    g_object_unref(cam);
}


static void entangle_camera_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
    EntangleCamera *cam = ENTANGLE_CAMERA(object);
    EntangleCameraPrivate *priv = cam->priv;

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

static void entangle_camera_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
    EntangleCamera *cam = ENTANGLE_CAMERA(object);
    EntangleCameraPrivate *priv = cam->priv;

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
            entangle_camera_set_progress(cam, g_value_get_object(value));
            break;

        case PROP_HAS_CAPTURE:
            priv->hasCapture = g_value_get_boolean(value);
            ENTANGLE_DEBUG("Set has capture %d", priv->hasCapture);
            break;

        case PROP_HAS_PREVIEW:
            priv->hasPreview = g_value_get_boolean(value);
            ENTANGLE_DEBUG("Set has preview %d", priv->hasPreview);
            break;

        case PROP_HAS_SETTINGS:
            priv->hasSettings = g_value_get_boolean(value);
            ENTANGLE_DEBUG("Set has settings %d", priv->hasSettings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_finalize(GObject *object)
{
    EntangleCamera *camera = ENTANGLE_CAMERA(object);
    EntangleCameraPrivate *priv = camera->priv;

    ENTANGLE_DEBUG("Finalize camera %p", object);

    if (priv->progress)
        g_object_unref(priv->progress);
    if (priv->cam) {
        gp_camera_exit(priv->cam, priv->ctx);
        gp_camera_free(priv->cam);
    }
    if (priv->widgets)
        gp_widget_unref(priv->widgets);
    if (priv->controls)
        g_object_unref(priv->controls);
    if (priv->ports)
        gp_port_info_list_free(priv->ports);
    if (priv->caps)
        gp_abilities_list_free(priv->caps);
    gp_context_unref(priv->ctx);
    g_free(priv->driver);
    g_free(priv->summary);
    g_free(priv->manual);
    g_free(priv->model);
    g_free(priv->port);
    g_free(priv->lastError);
    g_mutex_free(priv->lock);
    g_cond_free(priv->cond);

    G_OBJECT_CLASS (entangle_camera_parent_class)->finalize (object);
}


static void entangle_camera_class_init(EntangleCameraClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_camera_finalize;
    object_class->get_property = entangle_camera_get_property;
    object_class->set_property = entangle_camera_set_property;


    g_signal_new("camera-file-added",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraClass, camera_file_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-captured",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraClass, camera_file_captured),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-previewed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraClass, camera_file_previewed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-downloaded",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraClass, camera_file_downloaded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA_FILE);

    g_signal_new("camera-file-deleted",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraClass, camera_file_deleted),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA_FILE);


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
                                                        ENTANGLE_TYPE_PROGRESS,
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
    ENTANGLE_DEBUG("install prog done");

    g_type_class_add_private(klass, sizeof(EntangleCameraPrivate));
}


EntangleCamera *entangle_camera_new(const char *model,
                                    const char *port,
                                    gboolean hasCapture,
                                    gboolean hasPreview,
                                    gboolean hasSettings)
{
    return ENTANGLE_CAMERA(g_object_new(ENTANGLE_TYPE_CAMERA,
                                    "model", model,
                                    "port", port,
                                    "has-capture", hasCapture,
                                    "has-preview", hasPreview,
                                    "has-settings", hasSettings,
                                    NULL));
}


static void entangle_camera_init(EntangleCamera *cam)
{
    cam->priv = ENTANGLE_CAMERA_GET_PRIVATE(cam);
    cam->priv->lock = g_mutex_new();
    cam->priv->cond = g_cond_new();
}


const char *entangle_camera_get_model(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    return priv->model;
}

const char *entangle_camera_get_port(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    return priv->port;
}


static unsigned int do_entangle_camera_progress_start(GPContext *ctx G_GNUC_UNUSED,
                                                      float target,
                                                      const char *format,
                                                      va_list args,
                                                      void *data)
{
    EntangleCamera *cam = data;
    EntangleCameraPrivate *priv = cam->priv;

    if (priv->progress)
        entangle_progress_start(priv->progress, target, format, args);

    return 0; /* XXX what is this actually useful for ? */
}

static void do_entangle_camera_progress_update(GPContext *ctx G_GNUC_UNUSED,
                                               unsigned int id G_GNUC_UNUSED,
                                               float current,
                                               void *data)
{
    EntangleCamera *cam = data;
    EntangleCameraPrivate *priv = cam->priv;

    if (priv->progress)
        entangle_progress_update(priv->progress, current);
}

static void do_entangle_camera_progress_stop(GPContext *ctx G_GNUC_UNUSED,
                                             unsigned int id G_GNUC_UNUSED,
                                             void *data)
{
    EntangleCamera *cam = data;
    EntangleCameraPrivate *priv = cam->priv;

    if (priv->progress)
        entangle_progress_stop(priv->progress);
}

static void entangle_camera_reset_last_error(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;

    g_free(priv->lastError);
    priv->lastError = NULL;
}

static void do_entangle_camera_error(GPContext *ctx G_GNUC_UNUSED,
                                     const char *fmt,
                                     va_list args,
                                     void *data)
{
    EntangleCamera *cam = data;
    EntangleCameraPrivate *priv = cam->priv;

    entangle_camera_reset_last_error(cam);
    priv->lastError = g_strdup_vprintf(fmt, args);
    ENTANGLE_DEBUG("Got error %s", priv->lastError);
}


gboolean entangle_camera_connect(EntangleCamera *cam,
                                 GError **error)
{
    EntangleCameraPrivate *priv = cam->priv;
    int i;
    GPPortInfo port;
    CameraAbilities cap;
    CameraText txt;
    int err;
    gboolean ret = FALSE;

    ENTANGLE_DEBUG("Conencting to cam");

    g_mutex_lock(priv->lock);

    if (priv->cam != NULL) {
        ret = TRUE;
        goto cleanup;
    }

    priv->ctx = gp_context_new();

    if (gp_abilities_list_new(&priv->caps) != GP_OK) {
        ENTANGLE_ERROR(error, "Cannot initialize gphoto2 abilities");
        goto cleanup;
    }

    if (gp_abilities_list_load(priv->caps, priv->ctx) != GP_OK) {
        ENTANGLE_ERROR(error, "Cannot load gphoto2 abilities");
        goto cleanup;
    }

    if (gp_port_info_list_new(&priv->ports) != GP_OK) {
        ENTANGLE_ERROR(error, "Cannot initialize gphoto2 ports");
        goto cleanup;
    }

    if (gp_port_info_list_load(priv->ports) != GP_OK) {
        ENTANGLE_ERROR(error, "Cannot load gphoto2 ports");
        goto cleanup;
    }

    gp_context_set_error_func(priv->ctx,
                              do_entangle_camera_error,
                              cam);
    gp_context_set_progress_funcs(priv->ctx,
                                  do_entangle_camera_progress_start,
                                  do_entangle_camera_progress_update,
                                  do_entangle_camera_progress_stop,
                                  cam);

    i = gp_port_info_list_lookup_path(priv->ports, priv->port);
    gp_port_info_list_get_info(priv->ports, i, &port);

    i = gp_abilities_list_lookup_model(priv->caps, priv->model);
    gp_abilities_list_get_abilities(priv->caps, i, &cap);

    gp_camera_new(&priv->cam);
    gp_camera_set_abilities(priv->cam, cap);
    gp_camera_set_port_info(priv->cam, port);

    entangle_camera_begin_job(cam);
    err = gp_camera_init(priv->cam, priv->ctx);
    entangle_camera_end_job(cam);

    if (err != GP_OK) {
        gp_camera_unref(priv->cam);
        priv->cam = NULL;
        ENTANGLE_ERROR(error, "Unable to initialize camera");
        goto cleanup;
    }

    /* Update entanglebilities as a sanity-check against orignal constructor */
    priv->hasCapture = priv->hasPreview = priv->hasSettings = FALSE;
    if (cap.operations & GP_OPERATION_CAPTURE_IMAGE)
        priv->hasCapture = TRUE;
    if (cap.operations & GP_OPERATION_CAPTURE_PREVIEW)
        priv->hasPreview = TRUE;
    if (cap.operations & GP_OPERATION_CONFIG)
        priv->hasSettings = TRUE;

    gp_camera_get_summary(priv->cam, &txt, priv->ctx);
    priv->summary = g_strdup(txt.text);

    gp_camera_get_manual(priv->cam, &txt, priv->ctx);
    priv->manual = g_strdup(txt.text);

    gp_camera_get_about(priv->cam, &txt, priv->ctx);
    priv->driver = g_strdup(txt.text);

    g_mutex_unlock(priv->lock);
    ENTANGLE_DEBUG("ok");
    ret = TRUE;

 cleanup:
    return ret;
}

gboolean entangle_camera_disconnect(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    gboolean ret = FALSE;

    ENTANGLE_DEBUG("Disconnecting from cam");

    g_mutex_lock(priv->lock);

    if (priv->cam == NULL) {
        ret = TRUE;
        goto cleanup;
    }

    entangle_camera_begin_job(cam);
    gp_camera_exit(priv->cam, priv->ctx);
    entangle_camera_end_job(cam);

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

    if (priv->ports)
        gp_port_info_list_free(priv->ports);
    if (priv->caps)
        gp_abilities_list_free(priv->caps);
    gp_context_unref(priv->ctx);

    gp_camera_unref(priv->cam);
    priv->cam = NULL;

    priv->hasCapture = priv->hasPreview = priv->hasSettings = FALSE;

    ret = TRUE;
 cleanup:
    g_mutex_unlock(priv->lock);
    return ret;
}

gboolean entangle_camera_get_connected(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;

    return priv->cam != NULL ? TRUE : FALSE;
}


char *entangle_camera_get_summary(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    char *ret;

    g_mutex_lock(priv->lock);
    ret = g_strdup(priv->summary);
    g_mutex_unlock(priv->lock);

    return ret;
}


char *entangle_camera_get_manual(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    char *ret;

    g_mutex_lock(priv->lock);
    ret = g_strdup(priv->manual);
    g_mutex_unlock(priv->lock);

    return ret;
}


char *entangle_camera_get_driver(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    char *ret;

    g_mutex_lock(priv->lock);
    ret = g_strdup(priv->driver);
    g_mutex_unlock(priv->lock);

    return ret;
}

struct EntangleCameraEventData {
    EntangleCamera *cam;
    EntangleCameraFile *file;
    char *signame;
};

static gboolean entangle_camera_emit_idle(gpointer opaque)
{
    struct EntangleCameraEventData *data = opaque;

    g_signal_emit_by_name(data->cam, data->signame, data->file);

    g_free(data->signame);
    g_object_unref(data->cam);
    g_object_unref(data->file);
    g_free(data);
    return FALSE;
}

static void entangle_camera_emit_deferred(EntangleCamera *cam,
                                          const char *signame,
                                          EntangleCameraFile *file)
{
    struct EntangleCameraEventData *data = g_new0(struct EntangleCameraEventData, 1);
    data->cam = cam;
    data->file = file;
    data->signame = g_strdup(signame);
    g_object_ref(cam);
    g_object_ref(file);

    g_idle_add(entangle_camera_emit_idle, data);
}

EntangleCameraFile *entangle_camera_capture_image(EntangleCamera *cam,
                                                  GError **error)
{
    EntangleCameraPrivate *priv = cam->priv;
    CameraFilePath camerapath;
    EntangleCameraFile *file = NULL;
    int err;

    g_mutex_lock(priv->lock);

    if (!priv->cam) {
        ENTANGLE_ERROR(error, "Cannot capture image while not connected");
        goto cleanup;
    }

    ENTANGLE_DEBUG("Starting capture");
    entangle_camera_reset_last_error(cam);
    entangle_camera_begin_job(cam);
    err = gp_camera_capture(priv->cam,
                            GP_CAPTURE_IMAGE,
                            &camerapath,
                            priv->ctx);
    entangle_camera_end_job(cam);
    if (err!= GP_OK) {
        ENTANGLE_ERROR(error, "Unable to capture image: %s", priv->lastError);
        goto cleanup;
    }

    file = entangle_camera_file_new(camerapath.folder,
                                    camerapath.name);

    entangle_camera_emit_deferred(cam, "camera-file-captured", file);

 cleanup:
    g_mutex_unlock(priv->lock);
    return file;
}


EntangleCameraFile *entangle_camera_preview_image(EntangleCamera *cam,
                                                  GError **error)
{
    EntangleCameraPrivate *priv = cam->priv;
    EntangleCameraFile *file = NULL;
    CameraFile *datafile = NULL;
    const char *mimetype = NULL;
    GByteArray *data = NULL;
    const char *rawdata;
    unsigned long int rawdatalen;
    const char *name;
    int err;

    g_mutex_lock(priv->lock);

    if (!priv->cam) {
        ENTANGLE_ERROR(error, "Cannot preview image while not connected");
        goto cleanup;
    }

    gp_file_new(&datafile);

    ENTANGLE_DEBUG("Starting preview");
    entangle_camera_reset_last_error(cam);
    entangle_camera_begin_job(cam);
    err = gp_camera_capture_preview(priv->cam,
                                    datafile,
                                    priv->ctx);
    entangle_camera_end_job(cam);

    if (err != GP_OK) {
        ENTANGLE_ERROR(error, "Unable to capture preview: %s", priv->lastError);
        goto cleanup;
    }


    if (gp_file_get_data_and_size(datafile, &rawdata, &rawdatalen) != GP_OK) {
        ENTANGLE_ERROR(error, "Unable to get file data: %s", priv->lastError);
        goto cleanup;
    }

    if (gp_file_get_name(datafile, &name) != GP_OK) {
        ENTANGLE_ERROR(error, "Unable to get filename: %s", priv->lastError);
        goto cleanup;
    }

    file = entangle_camera_file_new("/", name);

    if (gp_file_get_mime_type(datafile, &mimetype) == GP_OK)
        entangle_camera_file_set_mimetype(file, mimetype);

    data = g_byte_array_new();
    g_byte_array_append(data, (const guint8 *)rawdata, rawdatalen);

    entangle_camera_file_set_data(file, data);
    g_byte_array_unref(data);

    entangle_camera_emit_deferred(cam, "camera-file-previewed", file);

 cleanup:
    if (datafile)
        gp_file_unref(datafile);
    g_mutex_unlock(priv->lock);
    return file;
}


gboolean entangle_camera_download_file(EntangleCamera *cam,
                                       EntangleCameraFile *file,
                                       GError **error)
{
    EntangleCameraPrivate *priv = cam->priv;
    CameraFile *datafile = NULL;
    const char *data;
    unsigned long int datalen;
    GByteArray *filedata;
    gboolean ret = FALSE;
    int err;

    g_mutex_lock(priv->lock);

    if (!priv->cam) {
        ENTANGLE_ERROR(error, "Cannot download file while not connected");
        goto cleanup;
    }

    ENTANGLE_DEBUG("Downloading '%s' from '%s'",
               entangle_camera_file_get_name(file),
               entangle_camera_file_get_folder(file));

    gp_file_new(&datafile);

    ENTANGLE_DEBUG("Getting file data");
    entangle_camera_reset_last_error(cam);
    entangle_camera_begin_job(cam);
    err = gp_camera_file_get(priv->cam,
                             entangle_camera_file_get_folder(file),
                             entangle_camera_file_get_name(file),
                             GP_FILE_TYPE_NORMAL,
                             datafile,
                             priv->ctx);
    entangle_camera_end_job(cam);

    if (err != GP_OK) {
        ENTANGLE_ERROR(error, "Unable to get camera file: %s", priv->lastError);
        goto cleanup;
    }

    ENTANGLE_DEBUG("Fetching data");
    if (gp_file_get_data_and_size(datafile, &data, &datalen) != GP_OK) {
        ENTANGLE_ERROR(error, "Unable to get file data: %s", priv->lastError);
        goto cleanup;
    }

    filedata = g_byte_array_new();
    g_byte_array_append(filedata, (const guint8*)data, datalen);

    entangle_camera_file_set_data(file, filedata);
    g_byte_array_unref(filedata);

    entangle_camera_emit_deferred(cam, "camera-file-downloaded", file);

    ret = TRUE;

 cleanup:
    ENTANGLE_DEBUG("Error");
    if (datafile)
        gp_file_unref(datafile);
    g_mutex_unlock(priv->lock);
    return ret;
}


gboolean entangle_camera_delete_file(EntangleCamera *cam,
                                     EntangleCameraFile *file,
                                     GError **error)
{
    EntangleCameraPrivate *priv = cam->priv;
    gboolean ret = FALSE;
    int err;

    g_mutex_lock(priv->lock);

    if (!priv->cam) {
        ENTANGLE_ERROR(error, "Cannot delete file while not connected");
        goto cleanup;
    }

    ENTANGLE_DEBUG("Deleting '%s' from '%s'",
               entangle_camera_file_get_name(file),
               entangle_camera_file_get_folder(file));

    entangle_camera_reset_last_error(cam);
    entangle_camera_begin_job(cam);
    err = gp_camera_file_delete(priv->cam,
                                entangle_camera_file_get_folder(file),
                                entangle_camera_file_get_name(file),
                                priv->ctx);
    entangle_camera_end_job(cam);

    if (err != GP_OK) {
        ENTANGLE_ERROR(error, "Unable to delete file: %s", priv->lastError);
        goto cleanup;
    }

    entangle_camera_emit_deferred(cam, "camera-file-deleted", file);

    ret = TRUE;

 cleanup:
    g_mutex_unlock(priv->lock);
    return ret;
}


gboolean entangle_camera_event_flush(EntangleCamera *cam,
                                     GError **error)
{
    EntangleCameraPrivate *priv = cam->priv;
    CameraEventType eventType;
    void *eventData;
    int err;
    gboolean ret = FALSE;

    g_mutex_lock(priv->lock);

    if (!priv->cam) {
        ENTANGLE_ERROR(error, "Cannot flush events while not connected");
        goto cleanup;
    }

    ENTANGLE_DEBUG("Flushing events");

    entangle_camera_reset_last_error(cam);
    do {
        entangle_camera_begin_job(cam);
        err = gp_camera_wait_for_event(priv->cam, 10, &eventType, &eventData, priv->ctx);
        entangle_camera_end_job(cam);

        if (err != GP_OK) {
            /* Some drivers (eg canon native) can't do events */
            if (err == GP_ERROR_NOT_SUPPORTED) {
                ENTANGLE_DEBUG("Event wait not supported, nothing to flush");
                ret = TRUE;
                goto cleanup;
            }

            ENTANGLE_ERROR(error, "Unable to wait for events: %s", priv->lastError);
            goto cleanup;
        }

    } while (eventType != GP_EVENT_TIMEOUT);

    ENTANGLE_DEBUG("Done flushing events");
    ret = TRUE;

 cleanup:
    g_mutex_unlock(priv->lock);
    return ret;
}


gboolean entangle_camera_event_wait(EntangleCamera *cam,
                                    guint64 waitms,
                                    GError **error)
{
    EntangleCameraPrivate *priv = cam->priv;
    CameraEventType eventType = 0;
    void *eventData;
    GTimeVal tv;
    guint64 startms, endms, donems;
    gboolean ret = FALSE;
    int err;

    g_mutex_lock(priv->lock);

    if (!priv->cam) {
        ENTANGLE_ERROR(error, "Cannot wait for events while not connected");
        goto cleanup;
    }

    g_get_current_time(&tv);
    startms = (tv.tv_sec * 1000ll) + (tv.tv_usec / 1000ll);

    ENTANGLE_DEBUG("Waiting for events %llu", (unsigned long long)startms);

    entangle_camera_reset_last_error(cam);
    donems = 0;
    do {
        entangle_camera_begin_job(cam);
        fprintf(stderr, "Wait %llu\n", (unsigned long long)waitms - donems);
        err = gp_camera_wait_for_event(priv->cam, waitms - donems, &eventType, &eventData, priv->ctx);
        fprintf(stderr, "Wait done %llu\n", (unsigned long long)waitms - donems);
        entangle_camera_end_job(cam);

        if (err != GP_OK) { 
            /* Some drivers (eg canon native) can't do events, so just do a sleep */
            if (err == GP_ERROR_NOT_SUPPORTED) {
                ENTANGLE_DEBUG("Event wait not supported, using usleep");
                g_usleep((waitms-donems)*1000ll);
                ret = TRUE;
                goto cleanup;
            }
            ENTANGLE_ERROR(error, "Unable to wait for events: %s", priv->lastError);
            goto cleanup;
        }

        switch (eventType) {
        case GP_EVENT_UNKNOWN:
            ENTANGLE_DEBUG("Unknown event '%s'", (char *)eventData);
            break;

        case GP_EVENT_TIMEOUT:
            ENTANGLE_DEBUG("Wait timed out");
            break;

        case GP_EVENT_FILE_ADDED: {
            CameraFilePath *camerapath = eventData;
            EntangleCameraFile *file;

            ENTANGLE_DEBUG("File added '%s' in '%s'", camerapath->name, camerapath->folder);

            file = entangle_camera_file_new(camerapath->folder,
                                        camerapath->name);

            entangle_camera_emit_deferred(cam, "camera-file-added", file);

            g_object_unref(file);
        }   break;

        case GP_EVENT_FOLDER_ADDED: {
            CameraFilePath *camerapath = eventData;

            ENTANGLE_DEBUG("Folder added '%s' in '%s'", camerapath->name, camerapath->folder);
        }   break;

        case GP_EVENT_CAPTURE_COMPLETE:
            ENTANGLE_DEBUG("Capture is complete");
            break;

        default:
            ENTANGLE_DEBUG("Unexpected event received %d", eventType);
            break;
        }

        g_get_current_time(&tv);
        endms = (tv.tv_sec * 1000ll) + (tv.tv_usec / 1000ll);
        donems = endms - startms;
    } while (eventType != GP_EVENT_TIMEOUT &&
             donems < waitms);

    ENTANGLE_DEBUG("Done waiting for events %llu", (unsigned long long)donems);

    ret = TRUE;

 cleanup:
    g_mutex_unlock(priv->lock);
    return ret;
}


static void do_update_control_text(GObject *object,
                                   GParamSpec *param G_GNUC_UNUSED,
                                   void *data)
{
    EntangleCamera *cam = data;
    EntangleCameraPrivate *priv = cam->priv;
    char *text;
    char *path;
    int id;
    CameraWidget *widget;
    int ret;

    g_object_get(object, "path", &path, "id", &id, "value", &text, NULL);
    ENTANGLE_DEBUG("update of widget %s", path);

    if (gp_widget_get_child_by_id(priv->widgets, id, &widget) != GP_OK) {
        ENTANGLE_DEBUG("cannot get widget id %d", id);
        return;
    }

    if (gp_widget_set_value(widget, text) != GP_OK) {
        ENTANGLE_DEBUG("cannot set widget id %d to %s", id, text);
    }

    if ((ret = gp_camera_set_config(priv->cam, priv->widgets, priv->ctx)) != GP_OK)
        ENTANGLE_DEBUG("cannot set config %s", gp_result_as_string(ret));

}

static void do_update_control_float(GObject *object,
                                    GParamSpec *param G_GNUC_UNUSED,
                                    void *data)
{
    EntangleCamera *cam = data;
    EntangleCameraPrivate *priv = cam->priv;
    float value;
    char *path;
    int id;
    CameraWidget *widget;

    g_object_get(object, "path", &path, "id", &id, "value", &value, NULL);
    ENTANGLE_DEBUG("update of widget %s", path);

    if (gp_widget_get_child_by_id(priv->widgets, id, &widget) != GP_OK) {
        ENTANGLE_DEBUG("cannot get widget id %d", id);
        return;
    }

    if (gp_widget_set_value(widget, &value) != GP_OK) {
        ENTANGLE_DEBUG("cannot set widget id %d to %f", id, value);
    }

    if (gp_camera_set_config(priv->cam, priv->widgets, priv->ctx) != GP_OK)
        ENTANGLE_DEBUG("cannot set config");

}

static void do_update_control_boolean(GObject *object,
                                      GParamSpec *param G_GNUC_UNUSED,
                                      void *data)
{
    EntangleCamera *cam = data;
    EntangleCameraPrivate *priv = cam->priv;
    gboolean value;
    char *path;
    int id;
    CameraWidget *widget;

    g_object_get(object, "path", &path, "id", &id, "value", &value, NULL);
    ENTANGLE_DEBUG("update of widget %s", path);

    if (gp_widget_get_child_by_id(priv->widgets, id, &widget) != GP_OK) {
        ENTANGLE_DEBUG("cannot get widget id %d", id);
        return;
    }

    if (gp_widget_set_value(widget, &value) != GP_OK) {
        ENTANGLE_DEBUG("cannot set widget id %d to %d", id, value);
    }

    if (gp_camera_set_config(priv->cam, priv->widgets, priv->ctx) != GP_OK)
        ENTANGLE_DEBUG("cannot set config");

}

static EntangleControl *do_build_controls(EntangleCamera *cam,
                                          const char *path,
                                          CameraWidget *widget)
{
    CameraWidgetType type;
    EntangleControl *ret = NULL;
    const char *name;
    char *fullpath;
    int id;
    const char *label;
    const char *info;
    int ro;

    if (gp_widget_get_type(widget, &type) != GP_OK)
        return NULL;

    if (gp_widget_get_name(widget, &name) != GP_OK)
        return NULL;

    gp_widget_get_id(widget, &id);
    gp_widget_get_label(widget, &label);
    gp_widget_get_info(widget, &info);
    gp_widget_get_readonly(widget, &ro);
    if (info == NULL)
        info = label;

    fullpath = g_strdup_printf("%s/%s", path, name);

    switch (type) {
        /* We treat both window and section as just groups */
    case GP_WIDGET_WINDOW:
    case GP_WIDGET_SECTION:
        {
            EntangleControlGroup *grp;
            ENTANGLE_DEBUG("Add group %s %d %s", fullpath, id, label);
            grp = entangle_control_group_new(fullpath, id, label, info, ro);
            for (int i = 0 ; i < gp_widget_count_children(widget) ; i++) {
                CameraWidget *child;
                EntangleControl *subctl;
                if (gp_widget_get_child(widget, i, &child) != GP_OK) {
                    g_object_unref(grp);
                    goto error;
                }
                if (!(subctl = do_build_controls(cam, fullpath, child))) {
                    g_object_unref(grp);
                    goto error;
                }

                entangle_control_group_add(grp, subctl);
            }

            ret = ENTANGLE_CONTROL(grp);
        } break;

    case GP_WIDGET_BUTTON:
        {
            ENTANGLE_DEBUG("Add button %s %d %s", fullpath, id, label);
            ret = ENTANGLE_CONTROL(entangle_control_button_new(fullpath, id, label, info, ro));
        } break;

        /* Unclear why these two are the same in libgphoto */
    case GP_WIDGET_RADIO:
    case GP_WIDGET_MENU:
        {
            char *value = NULL;
            ENTANGLE_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = ENTANGLE_CONTROL(entangle_control_choice_new(fullpath, id, label, info, ro));

            for (int i = 0 ; i < gp_widget_count_choices(widget) ; i++) {
                const char *choice;
                gp_widget_get_choice(widget, i, &choice);
                entangle_control_choice_add_entry(ENTANGLE_CONTROL_CHOICE(ret), choice);
            }

            gp_widget_get_value(widget, &value);
            g_object_set(ret, "value", value, NULL);
            g_signal_connect(ret, "notify::value",
                             G_CALLBACK(do_update_control_text), cam);
        } break;

    case GP_WIDGET_DATE:
        {
            int value = 0;
            ENTANGLE_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = ENTANGLE_CONTROL(entangle_control_date_new(fullpath, id, label, info, ro));
            g_object_set(ret, "value", value, NULL);
        } break;

    case GP_WIDGET_RANGE:
        {
            float min, max, step;
            float value = 0.0;
            gp_widget_get_range(widget, &min, &max, &step);
            ENTANGLE_DEBUG("Add range %s %d %s %f %f %f", fullpath, id, label, min, max, step);
            ret = ENTANGLE_CONTROL(entangle_control_range_new(fullpath, id, label, info, ro,
                                                              min, max, step));

            gp_widget_get_value(widget, &value);
            g_object_set(ret, "value", value, NULL);
            g_signal_connect(ret, "notify::value",
                             G_CALLBACK(do_update_control_float), cam);
        } break;

    case GP_WIDGET_TEXT:
        {
            char *value = NULL;
            ENTANGLE_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = ENTANGLE_CONTROL(entangle_control_text_new(fullpath, id, label, info, ro));

            gp_widget_get_value(widget, &value);
            g_object_set(ret, "value", value, NULL);
            g_signal_connect(ret, "notify::value",
                             G_CALLBACK(do_update_control_text), cam);
        } break;

    case GP_WIDGET_TOGGLE:
        {
            int value = 0;
            ENTANGLE_DEBUG("Add date %s %d %s", fullpath, id, label);
            ret = ENTANGLE_CONTROL(entangle_control_toggle_new(fullpath, id, label, info, ro));

            gp_widget_get_value(widget, &value);
            g_object_set(ret, "value", (gboolean)value, NULL);
            g_signal_connect(ret, "notify::value",
                             G_CALLBACK(do_update_control_boolean), cam);
        } break;
    }

 error:
    g_free(fullpath);
    return ret;
}


EntangleControlGroup *entangle_camera_get_controls(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    EntangleControlGroup *ret = NULL;

    g_mutex_lock(priv->lock);

    if (priv->cam == NULL)
        goto cleanup;

    if (priv->controls == NULL) {
        int err;
        entangle_camera_begin_job(cam);
        err = gp_camera_get_config(priv->cam, &priv->widgets, priv->ctx);
        entangle_camera_end_job(cam);

        if (err != GP_OK)
            goto cleanup;

        priv->controls = ENTANGLE_CONTROL_GROUP(do_build_controls(cam, "", priv->widgets));
    }

    ret = priv->controls;
    g_object_ref(ret);

 cleanup:
    g_mutex_unlock(priv->lock);
    return ret;
}


gboolean entangle_camera_get_has_capture(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    gboolean ret;

    g_mutex_lock(priv->lock);
    ret = priv->hasCapture;
    g_mutex_unlock(priv->lock);

    return ret;
}


gboolean entangle_camera_get_has_preview(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    gboolean ret;

    g_mutex_lock(priv->lock);
    ret = priv->hasPreview;
    g_mutex_unlock(priv->lock);

    return ret;
}


gboolean entangle_camera_get_has_settings(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    gboolean ret;

    g_mutex_lock(priv->lock);
    ret = priv->hasSettings;
    g_mutex_unlock(priv->lock);

    return ret;
}


void entangle_camera_set_progress(EntangleCamera *cam, EntangleProgress *prog)
{
    EntangleCameraPrivate *priv = cam->priv;

    g_mutex_lock(priv->lock);
    if (priv->progress)
        g_object_unref(priv->progress);
    priv->progress = prog;
    if (priv->progress)
        g_object_ref(priv->progress);
    g_mutex_unlock(priv->lock);
}


EntangleProgress *entangle_camera_get_progress(EntangleCamera *cam)
{
    EntangleCameraPrivate *priv = cam->priv;
    EntangleProgress *ret;

    g_mutex_unlock(priv->lock);
    ret = priv->progress;
    g_object_ref(ret);
    g_mutex_unlock(priv->lock);

    return ret;
}


static GMount *entangle_device_manager_find_mount(EntangleCamera *cam,
                                                  GVolumeMonitor *monitor)
{
    EntangleCameraPrivate *priv = cam->priv;
    gchar *uri;
    GList *mounts;
    GList *tmp;
    GMount *ret = NULL;

    g_mutex_lock(priv->lock);
    uri = g_strdup_printf("gphoto2://[%s]/", priv->port);
    g_mutex_unlock(priv->lock);

    mounts = g_volume_monitor_get_mounts(monitor);
    tmp = mounts;
    while (tmp) {
        GMount *mount = tmp->data;
        GFile *root = g_mount_get_root(mount);
        gchar *thisuri = g_file_get_uri(root);

        if (g_strcmp0(uri, thisuri) == 0)
            ret = mount;
        else
            g_object_unref(mount);

        tmp = tmp->next;
    }
    g_list_free(mounts);

    g_free(uri);

    return ret;
}


gboolean entangle_camera_is_mounted(EntangleCamera *cam)
{
    GVolumeMonitor *monitor = g_volume_monitor_get();
    GMount *mount = entangle_device_manager_find_mount(cam, monitor);
    gboolean ret;

    if (mount) {
        g_object_unref(mount);
        ret = TRUE;
    } else {
        ret = FALSE;
    }

    g_object_unref(monitor);
    return ret;
}
#if 0
void entangle_camera_mount_async(EntangleCamera *cam,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data);
gboolean entangle_camera_mount_finish(EntangleCamera *cam,
                                      GAsyncResult *result,
                                      GError **err);
#endif

struct UnmountData {
    GVolumeMonitor *monitor;
    GMount *mount;
    GAsyncReadyCallback callback;
    gpointer user_data;
};

static void entangle_camera_unmount_cleanup(GObject *object,
                                            GAsyncResult *result,
                                            gpointer user_data)
{
    struct UnmountData *data = user_data;

    data->callback(object, result, data->user_data);

    g_object_unref(data->monitor);
    if (data->mount)
        g_object_unref(data->mount);
    g_free(data);
}

static void entangle_camera_unmount_complete(GObject *object,
                                             GAsyncResult *result,
                                             gpointer user_data)
{
    GSimpleAsyncResult *camresult = user_data;
    GError *err = NULL;
    GMount *mount = G_MOUNT(object);

    g_mount_unmount_with_operation_finish(mount,
                                          result,
                                          &err);

    if (err)
        g_simple_async_result_set_from_error(camresult, err);

    g_simple_async_result_complete(camresult);
    g_object_unref(camresult);
}

void entangle_camera_unmount_async(EntangleCamera *cam,
                                   GCancellable *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data)
{
    struct UnmountData *data = g_new0(struct UnmountData, 1);

    data->monitor = g_volume_monitor_get();
    data->mount = entangle_device_manager_find_mount(cam, data->monitor);
    data->callback = callback;
    data->user_data = user_data;

    GSimpleAsyncResult *result = g_simple_async_result_new(G_OBJECT(cam),
                                                           entangle_camera_unmount_cleanup,
                                                           data,
                                                           entangle_camera_unmount_async);

    if (data->mount) {
        g_mount_unmount_with_operation(data->mount,
                                       0, NULL,
                                       cancellable,
                                       entangle_camera_unmount_complete,
                                       result);
    } else {
        g_simple_async_result_complete(result);
        g_object_unref(result);
    }
}

gboolean entangle_camera_unmount_finish(EntangleCamera *cam G_GNUC_UNUSED,
                                        GAsyncResult *result,
                                        GError **err)
{
    return g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(result),
                                                 err);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
