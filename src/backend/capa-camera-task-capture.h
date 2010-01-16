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

#ifndef __CAPA_CAMERA_TASK_CAPTURE_H__
#define __CAPA_CAMERA_TASK_CAPTURE_H__

#include "capa-camera-task.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_TASK_CAPTURE            (capa_camera_task_capture_get_type ())
#define CAPA_CAMERA_TASK_CAPTURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_TASK_CAPTURE, CapaCameraTaskCapture))
#define CAPA_CAMERA_TASK_CAPTURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_TASK_CAPTURE, CapaCameraTaskCaptureClass))
#define CAPA_IS_CAMERA_TASK_CAPTURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_TASK_CAPTURE))
#define CAPA_IS_CAMERA_TASK_CAPTURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_TASK_CAPTURE))
#define CAPA_CAMERA_TASK_CAPTURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_TASK_CAPTURE, CapaCameraTaskCaptureClass))


typedef struct _CapaCameraTaskCapture CapaCameraTaskCapture;
typedef struct _CapaCameraTaskCapturePrivate CapaCameraTaskCapturePrivate;
typedef struct _CapaCameraTaskCaptureClass CapaCameraTaskCaptureClass;

struct _CapaCameraTaskCapture
{
    CapaCameraTask parent;

    CapaCameraTaskCapturePrivate *priv;
};

struct _CapaCameraTaskCaptureClass
{
    CapaCameraTaskClass parent_class;

};

GType capa_camera_task_capture_get_type(void) G_GNUC_CONST;

CapaCameraTaskCapture *capa_camera_task_capture_new(void);

G_END_DECLS

#endif /* __CAPA_CAMERA_TASK_CAPTURE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
