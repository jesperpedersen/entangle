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

#ifndef __ENTANGLE_CAMERA_H__
#define __ENTANGLE_CAMERA_H__

#include <glib-object.h>
#include <gio/gio.h>

#include "entangle-control-group.h"
#include "entangle-camera-file.h"
#include "entangle-progress.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA            (entangle_camera_get_type ())
#define ENTANGLE_CAMERA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA, EntangleCamera))
#define ENTANGLE_CAMERA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA, EntangleCameraClass))
#define ENTANGLE_IS_CAMERA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA))
#define ENTANGLE_IS_CAMERA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA))
#define ENTANGLE_CAMERA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA, EntangleCameraClass))


typedef struct _EntangleCamera EntangleCamera;
typedef struct _EntangleCameraPrivate EntangleCameraPrivate;
typedef struct _EntangleCameraClass EntangleCameraClass;

struct _EntangleCamera
{
    GObject parent;

    EntangleCameraPrivate *priv;
};

struct _EntangleCameraClass
{
    GObjectClass parent_class;

    void (*camera_file_added)(EntangleCamera *cam, EntangleCameraFile *file);
    void (*camera_file_captured)(EntangleCamera *cam, EntangleCameraFile *file);
    void (*camera_file_previewed)(EntangleCamera *cam, EntangleCameraFile *file);
    void (*camera_file_downloaded)(EntangleCamera *cam, EntangleCameraFile *file);
    void (*camera_file_deleted)(EntangleCamera *cam, EntangleCameraFile *file);
};


GType entangle_camera_get_type(void) G_GNUC_CONST;

EntangleCamera *entangle_camera_new(const char *model,
                                    const char *port,
                                    gboolean hasCapture,
                                    gboolean hasPreview,
                                    gboolean hasSettings);

const char *entangle_camera_get_model(EntangleCamera *cam);
const char *entangle_camera_get_port(EntangleCamera *cam);

gboolean entangle_camera_connect(EntangleCamera *cam,
                                 GError **error);
gboolean entangle_camera_disconnect(EntangleCamera *cam);
gboolean entangle_camera_get_connected(EntangleCamera *cam);

char *entangle_camera_get_summary(EntangleCamera *cam);
char *entangle_camera_get_manual(EntangleCamera *cam);
char *entangle_camera_get_driver(EntangleCamera *cam);

EntangleCameraFile *entangle_camera_capture_image(EntangleCamera *cam,
                                                  GError **error);

EntangleCameraFile *entangle_camera_preview_image(EntangleCamera *cam,
                                                  GError **error);

gboolean entangle_camera_download_file(EntangleCamera *cam,
                                       EntangleCameraFile *file,
                                       GError **error);
void entangle_camera_download_file_async(EntangleCamera *cam,
                                         EntangleCameraFile *file,
                                         GCancellable *cancellable,
                                         GAsyncReadyCallback callback,
                                         gpointer user_data);
gboolean entangle_camera_download_file_finish(EntangleCamera *cam,
                                              GAsyncResult *result,
                                              GError **err);

gboolean entangle_camera_delete_file(EntangleCamera *cam,
                                     EntangleCameraFile *file,
                                     GError **error);
void entangle_camera_delete_file_async(EntangleCamera *cam,
                                       EntangleCameraFile *file,
                                       GCancellable *cancellable,
                                       GAsyncReadyCallback callback,
                                       gpointer user_data);
gboolean entangle_camera_delete_file_finish(EntangleCamera *cam,
                                            GAsyncResult *result,
                                            GError **error);

gboolean entangle_camera_event_flush(EntangleCamera *cam,
                                     GError **error);
gboolean entangle_camera_event_wait(EntangleCamera *cam,
                                    guint64 waitms,
                                    GError **error);

gboolean entangle_camera_get_has_capture(EntangleCamera *cam);
gboolean entangle_camera_get_has_preview(EntangleCamera *cam);
gboolean entangle_camera_get_has_settings(EntangleCamera *cam);

EntangleControlGroup *entangle_camera_get_controls(EntangleCamera *cam);

void entangle_camera_set_progress(EntangleCamera *cam, EntangleProgress *prog);
EntangleProgress *entangle_camera_get_progress(EntangleCamera *cam);

gboolean entangle_camera_is_mounted(EntangleCamera *cam);

void entangle_camera_mount_async(EntangleCamera *cam,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data);
gboolean entangle_camera_mount_finish(EntangleCamera *cam,
                                      GAsyncResult *result,
                                      GError **err);

void entangle_camera_unmount_async(EntangleCamera *cam,
                                   GCancellable *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data);
gboolean entangle_camera_unmount_finish(EntangleCamera *cam,
                                        GAsyncResult *result,
                                        GError **err);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
