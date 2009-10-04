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

#ifndef __CAPA_CAMERA_MANAGER_H__
#define __CAPA_CAMERA_MANAGER_H__

#include <glib-object.h>

#include "camera.h"
#include "preferences.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_MANAGER            (capa_camera_manager_get_type ())
#define CAPA_CAMERA_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManager))
#define CAPA_CAMERA_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerClass))
#define CAPA_IS_CAMERA_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_MANAGER))
#define CAPA_IS_CAMERA_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_MANAGER))
#define CAPA_CAMERA_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerClass))


typedef struct _CapaCameraManager CapaCameraManager;
typedef struct _CapaCameraManagerPrivate CapaCameraManagerPrivate;
typedef struct _CapaCameraManagerClass CapaCameraManagerClass;

struct _CapaCameraManager
{
  GObject parent;

  CapaCameraManagerPrivate *priv;
};

struct _CapaCameraManagerClass
{
  GObjectClass parent_class;

  void (*manager_connect)(CapaCameraManager *manager);
  void (*manager_disconnect)(CapaCameraManager *manager);
};


GType capa_camera_manager_get_type(void) G_GNUC_CONST;
CapaCameraManager* capa_camera_manager_new(CapaPreferences *prefs);

void capa_camera_manager_show(CapaCameraManager *manager);
void capa_camera_manager_hide(CapaCameraManager *manager);

gboolean capa_camera_manager_visible(CapaCameraManager *manager);

G_END_DECLS

#endif /* __CAPA_CAMERA_MANAGER_H__ */

