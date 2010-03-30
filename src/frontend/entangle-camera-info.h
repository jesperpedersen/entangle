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

#ifndef __ENTANGLE_CAMERA_INFO_H__
#define __ENTANGLE_CAMERA_INFO_H__

#include <glib-object.h>

#include "entangle-camera.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_INFO            (entangle_camera_info_get_type ())
#define ENTANGLE_TYPE_CAMERA_INFO_DATA       (entangle_camera_info_data_get_type())
#define ENTANGLE_CAMERA_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_INFO, EntangleCameraInfo))
#define ENTANGLE_CAMERA_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_INFO, EntangleCameraInfoClass))
#define ENTANGLE_IS_CAMERA_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_INFO))
#define ENTANGLE_IS_CAMERA_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_INFO))
#define ENTANGLE_CAMERA_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_INFO, EntangleCameraInfoClass))


typedef struct _EntangleCameraInfo EntangleCameraInfo;
typedef struct _EntangleCameraInfoPrivate EntangleCameraInfoPrivate;
typedef struct _EntangleCameraInfoClass EntangleCameraInfoClass;

struct _EntangleCameraInfo
{
    GObject parent;

    EntangleCameraInfoPrivate *priv;
};

struct _EntangleCameraInfoClass
{
    GObjectClass parent_class;

    void (*info_close)(EntangleCameraInfo *info);
};

typedef enum {
    ENTANGLE_CAMERA_INFO_DATA_SUMMARY,
    ENTANGLE_CAMERA_INFO_DATA_MANUAL,
    ENTANGLE_CAMERA_INFO_DATA_DRIVER,
    ENTANGLE_CAMERA_INFO_DATA_SUPPORTED,
} EntangleCameraInfoData;

GType entangle_camera_info_get_type(void) G_GNUC_CONST;
GType entangle_camera_info_data_get_type(void) G_GNUC_CONST;

EntangleCameraInfo* entangle_camera_info_new(EntangleCamera *camera,
                                     EntangleCameraInfoData data);

void entangle_camera_info_show(EntangleCameraInfo *info);
void entangle_camera_info_hide(EntangleCameraInfo *info);


void entangle_camera_info_set_data(EntangleCameraInfo *info,
                               EntangleCameraInfoData data);

EntangleCameraInfoData entangle_camera_info_get_data(EntangleCameraInfo *info);

void entangle_camera_info_set_camera(EntangleCameraInfo *info,
                                 EntangleCamera *camera);

EntangleCamera *entangle_camera_info_get_camera(EntangleCameraInfo *info);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_INFO_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
