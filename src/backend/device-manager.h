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

#ifndef __CAPA_DEVICE_MANAGER_H__
#define __CAPA_DEVICE_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_DEVICE_MANAGER            (capa_device_manager_get_type ())
#define CAPA_DEVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_DEVICE_MANAGER, CapaDeviceManager))
#define CAPA_DEVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_DEVICE_MANAGER, CapaDeviceManagerClass))
#define CAPA_IS_DEVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_DEVICE_MANAGER))
#define CAPA_IS_DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_DEVICE_MANAGER))
#define CAPA_DEVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_DEVICE_MANAGER, CapaDeviceManagerClass))


typedef struct _CapaDeviceManager CapaDeviceManager;
typedef struct _CapaDeviceManagerPrivate CapaDeviceManagerPrivate;
typedef struct _CapaDeviceManagerClass CapaDeviceManagerClass;

struct _CapaDeviceManager
{
  GObject parent;

  CapaDeviceManagerPrivate *priv;
};

struct _CapaDeviceManagerClass
{
  GObjectClass parent_class;

  void (*device_added)(CapaDeviceManager *manager, const char *port);
  void (*device_removed)(CapaDeviceManager *manager, const char *port);
};


GType capa_device_manager_get_type(void) G_GNUC_CONST;
CapaDeviceManager* capa_device_manager_new(void);

gboolean capa_device_manager_has_port(CapaDeviceManager *manager, const char *port);

char *capa_device_manager_port_serial(CapaDeviceManager *manager, const char *port);

G_END_DECLS

#endif /* __CAPA_DEVICE_MANAGER_H__ */

