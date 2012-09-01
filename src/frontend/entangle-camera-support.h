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

#ifndef __ENTANGLE_CAMERA_SUPPORT_H__
#define __ENTANGLE_CAMERA_SUPPORT_H__

#include <gtk/gtk.h>

#include "entangle-camera-list.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_SUPPORT            (entangle_camera_support_get_type ())
#define ENTANGLE_TYPE_CAMERA_SUPPORT_DATA       (entangle_camera_support_data_get_type())
#define ENTANGLE_CAMERA_SUPPORT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_SUPPORT, EntangleCameraSupport))
#define ENTANGLE_CAMERA_SUPPORT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_SUPPORT, EntangleCameraSupportClass))
#define ENTANGLE_IS_CAMERA_SUPPORT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_SUPPORT))
#define ENTANGLE_IS_CAMERA_SUPPORT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_SUPPORT))
#define ENTANGLE_CAMERA_SUPPORT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_SUPPORT, EntangleCameraSupportClass))


typedef struct _EntangleCameraSupport EntangleCameraSupport;
typedef struct _EntangleCameraSupportPrivate EntangleCameraSupportPrivate;
typedef struct _EntangleCameraSupportClass EntangleCameraSupportClass;

struct _EntangleCameraSupport
{
    GObject parent;

    EntangleCameraSupportPrivate *priv;
};

struct _EntangleCameraSupportClass
{
    GObjectClass parent_class;

    void (*support_close)(EntangleCameraSupport *support);
};

GType entangle_camera_support_get_type(void) G_GNUC_CONST;

EntangleCameraSupport* entangle_camera_support_new(EntangleCameraList *list);

GtkWindow *entangle_camera_support_get_window(EntangleCameraSupport *support);

void entangle_camera_support_show(EntangleCameraSupport *support);
void entangle_camera_support_hide(EntangleCameraSupport *support);

void entangle_camera_support_set_camera_list(EntangleCameraSupport *support,
					       EntangleCameraList *list);

EntangleCameraList *entangle_camera_support_get_camera_list(EntangleCameraSupport *support);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_SUPPORT_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
