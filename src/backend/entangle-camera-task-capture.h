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

#ifndef __ENTANGLE_CAMERA_TASK_CAPTURE_H__
#define __ENTANGLE_CAMERA_TASK_CAPTURE_H__

#include "entangle-camera-task.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_TASK_CAPTURE            (entangle_camera_task_capture_get_type ())
#define ENTANGLE_CAMERA_TASK_CAPTURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_TASK_CAPTURE, EntangleCameraTaskCapture))
#define ENTANGLE_CAMERA_TASK_CAPTURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_TASK_CAPTURE, EntangleCameraTaskCaptureClass))
#define ENTANGLE_IS_CAMERA_TASK_CAPTURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_TASK_CAPTURE))
#define ENTANGLE_IS_CAMERA_TASK_CAPTURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_TASK_CAPTURE))
#define ENTANGLE_CAMERA_TASK_CAPTURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_TASK_CAPTURE, EntangleCameraTaskCaptureClass))


typedef struct _EntangleCameraTaskCapture EntangleCameraTaskCapture;
typedef struct _EntangleCameraTaskCapturePrivate EntangleCameraTaskCapturePrivate;
typedef struct _EntangleCameraTaskCaptureClass EntangleCameraTaskCaptureClass;

struct _EntangleCameraTaskCapture
{
    EntangleCameraTask parent;

    EntangleCameraTaskCapturePrivate *priv;
};

struct _EntangleCameraTaskCaptureClass
{
    EntangleCameraTaskClass parent_class;

};

GType entangle_camera_task_capture_get_type(void) G_GNUC_CONST;

EntangleCameraTaskCapture *entangle_camera_task_capture_new(void);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_TASK_CAPTURE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
