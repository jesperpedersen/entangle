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

#ifndef __ENTANGLE_DEVICE_MANAGER_H__
#define __ENTANGLE_DEVICE_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_DEVICE_MANAGER            (entangle_device_manager_get_type ())
#define ENTANGLE_DEVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_DEVICE_MANAGER, EntangleDeviceManager))
#define ENTANGLE_DEVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_DEVICE_MANAGER, EntangleDeviceManagerClass))
#define ENTANGLE_IS_DEVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_DEVICE_MANAGER))
#define ENTANGLE_IS_DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_DEVICE_MANAGER))
#define ENTANGLE_DEVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_DEVICE_MANAGER, EntangleDeviceManagerClass))


typedef struct _EntangleDeviceManager EntangleDeviceManager;
typedef struct _EntangleDeviceManagerPrivate EntangleDeviceManagerPrivate;
typedef struct _EntangleDeviceManagerClass EntangleDeviceManagerClass;

struct _EntangleDeviceManager
{
    GObject parent;

    EntangleDeviceManagerPrivate *priv;
};

struct _EntangleDeviceManagerClass
{
    GObjectClass parent_class;

    void (*device_added)(EntangleDeviceManager *manager, const char *port);
    void (*device_removed)(EntangleDeviceManager *manager, const char *port);
};


GType entangle_device_manager_get_type(void) G_GNUC_CONST;
EntangleDeviceManager* entangle_device_manager_new(void);

gboolean entangle_device_manager_has_port(EntangleDeviceManager *manager, const char *port);

char *entangle_device_manager_port_serial(EntangleDeviceManager *manager, const char *port);

G_END_DECLS

#endif /* __ENTANGLE_DEVICE_MANAGER_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
