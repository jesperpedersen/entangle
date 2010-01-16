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

#ifndef __CAPA_CAMERA_TASK_PREVIEW_H__
#define __CAPA_CAMERA_TASK_PREVIEW_H__

#include "capa-camera-task.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_TASK_PREVIEW            (capa_camera_task_preview_get_type ())
#define CAPA_CAMERA_TASK_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_TASK_PREVIEW, CapaCameraTaskPreview))
#define CAPA_CAMERA_TASK_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_TASK_PREVIEW, CapaCameraTaskPreviewClass))
#define CAPA_IS_CAMERA_TASK_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_TASK_PREVIEW))
#define CAPA_IS_CAMERA_TASK_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_TASK_PREVIEW))
#define CAPA_CAMERA_TASK_PREVIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_TASK_PREVIEW, CapaCameraTaskPreviewClass))


typedef struct _CapaCameraTaskPreview CapaCameraTaskPreview;
typedef struct _CapaCameraTaskPreviewPrivate CapaCameraTaskPreviewPrivate;
typedef struct _CapaCameraTaskPreviewClass CapaCameraTaskPreviewClass;

struct _CapaCameraTaskPreview
{
    CapaCameraTask parent;

    CapaCameraTaskPreviewPrivate *priv;
};

struct _CapaCameraTaskPreviewClass
{
    CapaCameraTaskClass parent_class;

};

GType capa_camera_task_preview_get_type(void) G_GNUC_CONST;

CapaCameraTaskPreview *capa_camera_task_preview_new(void);

G_END_DECLS

#endif /* __CAPA_CAMERA_TASK_PREVIEW_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
