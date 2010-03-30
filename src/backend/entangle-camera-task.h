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

#ifndef __ENTANGLE_CAMERA_TASK_H__
#define __ENTANGLE_CAMERA_TASK_H__

#include <glib-object.h>

#include "entangle-camera.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_TASK            (entangle_camera_task_get_type ())
#define ENTANGLE_CAMERA_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_TASK, EntangleCameraTask))
#define ENTANGLE_CAMERA_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_TASK, EntangleCameraTaskClass))
#define ENTANGLE_IS_CAMERA_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_TASK))
#define ENTANGLE_IS_CAMERA_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_TASK))
#define ENTANGLE_CAMERA_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_TASK, EntangleCameraTaskClass))


typedef struct _EntangleCameraTask EntangleCameraTask;
typedef struct _EntangleCameraTaskPrivate EntangleCameraTaskPrivate;
typedef struct _EntangleCameraTaskClass EntangleCameraTaskClass;

struct _EntangleCameraTask
{
    GObject parent;

    EntangleCameraTaskPrivate *priv;
};

struct _EntangleCameraTaskClass
{
    GObjectClass parent_class;

    gboolean (*execute)(EntangleCameraTask *task, EntangleCamera *cam);
};

GType entangle_camera_task_get_type(void) G_GNUC_CONST;

EntangleCameraTask *entangle_camera_task_new(const char *name,
                                     const char *label);

const char *entangle_camera_task_get_name(EntangleCameraTask *task);
const char *entangle_camera_task_get_label(EntangleCameraTask *task);

gboolean entangle_camera_task_execute(EntangleCameraTask *task,
                                  EntangleCamera *camera);

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
