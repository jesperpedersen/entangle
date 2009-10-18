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

#ifndef __CAPA_CAMERA_PICKER_H__
#define __CAPA_CAMERA_PICKER_H__

#include <glib-object.h>

#include "camera.h"
#include "camera-list.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_PICKER            (capa_camera_picker_get_type ())
#define CAPA_CAMERA_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_PICKER, CapaCameraPicker))
#define CAPA_CAMERA_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_PICKER, CapaCameraPickerClass))
#define CAPA_IS_CAMERA_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_PICKER))
#define CAPA_IS_CAMERA_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_PICKER))
#define CAPA_CAMERA_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_PICKER, CapaCameraPickerClass))


typedef struct _CapaCameraPicker CapaCameraPicker;
typedef struct _CapaCameraPickerPrivate CapaCameraPickerPrivate;
typedef struct _CapaCameraPickerClass CapaCameraPickerClass;

struct _CapaCameraPicker
{
    GObject parent;

    CapaCameraPickerPrivate *priv;
};

struct _CapaCameraPickerClass
{
    GObjectClass parent_class;

    void (*picker_connect)(CapaCameraPicker *picker, CapaCamera *cam);
    void (*picker_refresh)(CapaCameraPicker *picker);
    void (*picker_close)(CapaCameraPicker *picker);
};


GType capa_camera_picker_get_type(void) G_GNUC_CONST;
CapaCameraPicker* capa_camera_picker_new(CapaCameraList *cameras);

void capa_camera_picker_show(CapaCameraPicker *picker);
void capa_camera_picker_hide(CapaCameraPicker *picker);
gboolean capa_camera_picker_visible(CapaCameraPicker *picker);

G_END_DECLS

#endif /* __CAPA_CAMERA_PICKER_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
