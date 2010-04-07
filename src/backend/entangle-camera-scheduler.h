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

#ifndef __ENTANGLE_CAMERA_SCHEDULER_H__
#define __ENTANGLE_CAMERA_SCHEDULER_H__

#include <glib-object.h>

#include "entangle-camera-task.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_SCHEDULER            (entangle_camera_scheduler_get_type ())
#define ENTANGLE_CAMERA_SCHEDULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_SCHEDULER, EntangleCameraScheduler))
#define ENTANGLE_CAMERA_SCHEDULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_SCHEDULER, EntangleCameraSchedulerClass))
#define ENTANGLE_IS_CAMERA_SCHEDULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_SCHEDULER))
#define ENTANGLE_IS_CAMERA_SCHEDULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_SCHEDULER))
#define ENTANGLE_CAMERA_SCHEDULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_SCHEDULER, EntangleCameraSchedulerClass))


typedef struct _EntangleCameraScheduler EntangleCameraScheduler;
typedef struct _EntangleCameraSchedulerPrivate EntangleCameraSchedulerPrivate;
typedef struct _EntangleCameraSchedulerClass EntangleCameraSchedulerClass;

struct _EntangleCameraScheduler
{
    GObject parent;

    EntangleCameraSchedulerPrivate *priv;
};

struct _EntangleCameraSchedulerClass
{
    GObjectClass parent_class;

    void (*camera_scheduler_task_begin)(EntangleCameraScheduler *sched,
                                        EntangleCameraTask *task);
    void (*camera_scheduler_task_end)(EntangleCameraScheduler *sched,
                                      EntangleCameraTask *task);
};

GType entangle_camera_scheduler_get_type(void) G_GNUC_CONST;

EntangleCameraScheduler *entangle_camera_scheduler_new(EntangleCamera *camera);

gboolean entangle_camera_scheduler_start(EntangleCameraScheduler *scheduler);
gboolean entangle_camera_scheduler_end(EntangleCameraScheduler *scheduler);

gboolean entangle_camera_scheduler_pause(EntangleCameraScheduler *scheduler);
gboolean entangle_camera_scheduler_resume(EntangleCameraScheduler *scheduler);

gboolean entangle_camera_scheduler_queue(EntangleCameraScheduler *scheduler,
                                         EntangleCameraTask *task);

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
