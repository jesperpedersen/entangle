/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2015 Daniel P. Berrange
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

#ifndef __ENTANGLE_CAMERA_PICKER_H__
#define __ENTANGLE_CAMERA_PICKER_H__

#include <gtk/gtk.h>

#include "entangle-camera.h"
#include "entangle-camera-list.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_PICKER            (entangle_camera_picker_get_type ())
#define ENTANGLE_CAMERA_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_PICKER, EntangleCameraPicker))
#define ENTANGLE_CAMERA_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_PICKER, EntangleCameraPickerClass))
#define ENTANGLE_IS_CAMERA_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_PICKER))
#define ENTANGLE_IS_CAMERA_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_PICKER))
#define ENTANGLE_CAMERA_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_PICKER, EntangleCameraPickerClass))


typedef struct _EntangleCameraPicker EntangleCameraPicker;
typedef struct _EntangleCameraPickerPrivate EntangleCameraPickerPrivate;
typedef struct _EntangleCameraPickerClass EntangleCameraPickerClass;

struct _EntangleCameraPicker
{
    GtkDialog parent;

    EntangleCameraPickerPrivate *priv;
};

struct _EntangleCameraPickerClass
{
    GtkDialogClass parent_class;

    void (*picker_connect)(EntangleCameraPicker *picker, EntangleCamera *cam);
    void (*picker_refresh)(EntangleCameraPicker *picker);
};


GType entangle_camera_picker_get_type(void) G_GNUC_CONST;
EntangleCameraPicker* entangle_camera_picker_new(void);

void entangle_camera_picker_set_camera_list(EntangleCameraPicker *picker,
                                            EntangleCameraList *cameras);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_PICKER_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
