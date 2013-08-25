/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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

#ifndef __ENTANGLE_CAMERA_MANAGER_H__
#define __ENTANGLE_CAMERA_MANAGER_H__

#include <gtk/gtk.h>

#include "entangle-camera.h"
#include "entangle-application.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_MANAGER            (entangle_camera_manager_get_type ())
#define ENTANGLE_CAMERA_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_MANAGER, EntangleCameraManager))
#define ENTANGLE_CAMERA_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_MANAGER, EntangleCameraManagerClass))
#define ENTANGLE_IS_CAMERA_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_MANAGER))
#define ENTANGLE_IS_CAMERA_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_MANAGER))
#define ENTANGLE_CAMERA_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_MANAGER, EntangleCameraManagerClass))


typedef struct _EntangleCameraManager EntangleCameraManager;
typedef struct _EntangleCameraManagerPrivate EntangleCameraManagerPrivate;
typedef struct _EntangleCameraManagerClass EntangleCameraManagerClass;

struct _EntangleCameraManager
{
    GtkWindow parent;

    EntangleCameraManagerPrivate *priv;
};

struct _EntangleCameraManagerClass
{
    GtkWindowClass parent_class;

    void (*manager_connect)(EntangleCameraManager *manager);
    void (*manager_disconnect)(EntangleCameraManager *manager);
};


GType entangle_camera_manager_get_type(void) G_GNUC_CONST;
EntangleCameraManager* entangle_camera_manager_new(void);

void entangle_camera_manager_capture(EntangleCameraManager *manager);
void entangle_camera_manager_preview_begin(EntangleCameraManager *manager);
void entangle_camera_manager_preview_cancel(EntangleCameraManager *manager);

void entangle_camera_manager_set_camera(EntangleCameraManager *manager,
                                        EntangleCamera *cam);
EntangleCamera *entangle_camera_manager_get_camera(EntangleCameraManager *manager);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_MANAGER_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
