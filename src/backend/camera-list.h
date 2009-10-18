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


#ifndef __CAPA_CAMERA_LIST_H__
#define __CAPA_CAMERA_LIST_H__

#include "camera.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_LIST            (capa_camera_list_get_type ())
#define CAPA_CAMERA_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_LIST, CapaCameraList))
#define CAPA_CAMERA_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_LIST, CapaCameraListClass))
#define CAPA_IS_CAMERA_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_LIST))
#define CAPA_IS_CAMERA_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_LIST))
#define CAPA_CAMERA_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_LIST, CapaCameraListClass))


typedef struct _CapaCameraList CapaCameraList;
typedef struct _CapaCameraListPrivate CapaCameraListPrivate;
typedef struct _CapaCameraListClass CapaCameraListClass;

struct _CapaCameraList
{
    GObject parent;

    CapaCameraListPrivate *priv;
};

struct _CapaCameraListClass
{
    GObjectClass parent_class;

    void (*camera_added)(CapaCameraList *list, CapaCamera *cam);
    void (*camera_removed)(CapaCameraList *list, CapaCamera *cam);
};


GType capa_camera_list_get_type(void) G_GNUC_CONST;
CapaCameraList* capa_camera_list_new(void);

int capa_camera_list_count(CapaCameraList *list);

void capa_camera_list_add(CapaCameraList *list,
                          CapaCamera *cam);

void capa_camera_list_remove(CapaCameraList *list,
                             CapaCamera *cam);

CapaCamera *capa_camera_list_get(CapaCameraList *list,
                                 int entry);

CapaCamera *capa_camera_list_find(CapaCameraList *list,
                                  const char *port);

G_END_DECLS

#endif /* __CAPA_CAMERA_LIST_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
