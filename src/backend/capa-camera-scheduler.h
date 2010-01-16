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

#ifndef __CAPA_CAMERA_SCHEDULER_H__
#define __CAPA_CAMERA_SCHEDULER_H__

#include <glib-object.h>

#include "capa-camera-task.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_SCHEDULER            (capa_camera_scheduler_get_type ())
#define CAPA_CAMERA_SCHEDULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_SCHEDULER, CapaCameraScheduler))
#define CAPA_CAMERA_SCHEDULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_SCHEDULER, CapaCameraSchedulerClass))
#define CAPA_IS_CAMERA_SCHEDULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_SCHEDULER))
#define CAPA_IS_CAMERA_SCHEDULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_SCHEDULER))
#define CAPA_CAMERA_SCHEDULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_SCHEDULER, CapaCameraSchedulerClass))


typedef struct _CapaCameraScheduler CapaCameraScheduler;
typedef struct _CapaCameraSchedulerPrivate CapaCameraSchedulerPrivate;
typedef struct _CapaCameraSchedulerClass CapaCameraSchedulerClass;

struct _CapaCameraScheduler
{
    GObject parent;

    CapaCameraSchedulerPrivate *priv;
};

struct _CapaCameraSchedulerClass
{
    GObjectClass parent_class;

    void (*camera_scheduler_task_begin)(CapaCameraScheduler *sched,
                                        CapaCameraTask *task);
    void (*camera_scheduler_task_end)(CapaCameraScheduler *sched,
                                      CapaCameraTask *task);
};

GType capa_camera_scheduler_get_type(void) G_GNUC_CONST;

CapaCameraScheduler *capa_camera_scheduler_new(CapaCamera *camera);

gboolean capa_camera_scheduler_start(CapaCameraScheduler *scheduler);
gboolean capa_camera_scheduler_end(CapaCameraScheduler *scheduler);

gboolean capa_camera_scheduler_queue(CapaCameraScheduler *scheduler,
                                     CapaCameraTask *task);

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
