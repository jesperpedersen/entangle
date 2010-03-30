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


#ifndef __ENTANGLE_CAMERA_LIST_H__
#define __ENTANGLE_CAMERA_LIST_H__

#include "entangle-camera.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_LIST            (entangle_camera_list_get_type ())
#define ENTANGLE_CAMERA_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_LIST, EntangleCameraList))
#define ENTANGLE_CAMERA_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_LIST, EntangleCameraListClass))
#define ENTANGLE_IS_CAMERA_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_LIST))
#define ENTANGLE_IS_CAMERA_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_LIST))
#define ENTANGLE_CAMERA_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_LIST, EntangleCameraListClass))


typedef struct _EntangleCameraList EntangleCameraList;
typedef struct _EntangleCameraListPrivate EntangleCameraListPrivate;
typedef struct _EntangleCameraListClass EntangleCameraListClass;

struct _EntangleCameraList
{
    GObject parent;

    EntangleCameraListPrivate *priv;
};

struct _EntangleCameraListClass
{
    GObjectClass parent_class;

    void (*camera_added)(EntangleCameraList *list, EntangleCamera *cam);
    void (*camera_removed)(EntangleCameraList *list, EntangleCamera *cam);
};


GType entangle_camera_list_get_type(void) G_GNUC_CONST;
EntangleCameraList* entangle_camera_list_new(void);

int entangle_camera_list_count(EntangleCameraList *list);

void entangle_camera_list_add(EntangleCameraList *list,
                          EntangleCamera *cam);

void entangle_camera_list_remove(EntangleCameraList *list,
                             EntangleCamera *cam);

EntangleCamera *entangle_camera_list_get(EntangleCameraList *list,
                                 int entry);

EntangleCamera *entangle_camera_list_find(EntangleCameraList *list,
                                  const char *port);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_LIST_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
