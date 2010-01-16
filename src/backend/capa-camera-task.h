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

#ifndef __CAPA_CAMERA_TASK_H__
#define __CAPA_CAMERA_TASK_H__

#include <glib-object.h>

#include "capa-camera.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_TASK            (capa_camera_task_get_type ())
#define CAPA_CAMERA_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_TASK, CapaCameraTask))
#define CAPA_CAMERA_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_TASK, CapaCameraTaskClass))
#define CAPA_IS_CAMERA_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_TASK))
#define CAPA_IS_CAMERA_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_TASK))
#define CAPA_CAMERA_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_TASK, CapaCameraTaskClass))


typedef struct _CapaCameraTask CapaCameraTask;
typedef struct _CapaCameraTaskPrivate CapaCameraTaskPrivate;
typedef struct _CapaCameraTaskClass CapaCameraTaskClass;

struct _CapaCameraTask
{
    GObject parent;

    CapaCameraTaskPrivate *priv;
};

struct _CapaCameraTaskClass
{
    GObjectClass parent_class;

    gboolean (*execute)(CapaCameraTask *task, CapaCamera *cam);
};

GType capa_camera_task_get_type(void) G_GNUC_CONST;

CapaCameraTask *capa_camera_task_new(const char *name,
                                     const char *label);

const char *capa_camera_task_get_name(CapaCameraTask *task);
const char *capa_camera_task_get_label(CapaCameraTask *task);

gboolean capa_camera_task_execute(CapaCameraTask *task,
                                  CapaCamera *camera);

G_END_DECLS

#endif /* __CAPA_CAMERA_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
