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

#ifndef __ENTANGLE_CAMERA_TASK_MONITOR_H__
#define __ENTANGLE_CAMERA_TASK_MONITOR_H__

#include "entangle-camera-task.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_TASK_MONITOR            (entangle_camera_task_monitor_get_type ())
#define ENTANGLE_CAMERA_TASK_MONITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_TASK_MONITOR, EntangleCameraTaskMonitor))
#define ENTANGLE_CAMERA_TASK_MONITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_TASK_MONITOR, EntangleCameraTaskMonitorClass))
#define ENTANGLE_IS_CAMERA_TASK_MONITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_TASK_MONITOR))
#define ENTANGLE_IS_CAMERA_TASK_MONITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_TASK_MONITOR))
#define ENTANGLE_CAMERA_TASK_MONITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_TASK_MONITOR, EntangleCameraTaskMonitorClass))


typedef struct _EntangleCameraTaskMonitor EntangleCameraTaskMonitor;
typedef struct _EntangleCameraTaskMonitorPrivate EntangleCameraTaskMonitorPrivate;
typedef struct _EntangleCameraTaskMonitorClass EntangleCameraTaskMonitorClass;

struct _EntangleCameraTaskMonitor
{
    EntangleCameraTask parent;

    EntangleCameraTaskMonitorPrivate *priv;
};

struct _EntangleCameraTaskMonitorClass
{
    EntangleCameraTaskClass parent_class;

};

GType entangle_camera_task_monitor_get_type(void) G_GNUC_CONST;

EntangleCameraTaskMonitor *entangle_camera_task_monitor_new(void);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_TASK_MONITOR_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
