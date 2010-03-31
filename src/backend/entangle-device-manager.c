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

#include <config.h>

#include <string.h>
#include <stdio.h>

#if WITH_GUDEV
#define G_UDEV_API_IS_SUBJECT_TO_CHANGE
#include <gudev/gudev.h>
#endif
#if WITH_HAL
#include <libhal.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

#include "entangle-debug.h"
#include "entangle-device-manager.h"

#define ENTANGLE_DEVICE_MANAGER_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_DEVICE_MANAGER, EntangleDeviceManagerPrivate))

struct _EntangleDeviceManagerPrivate {
#if WITH_GUDEV
    GUdevClient *ctx;
#endif
#if WITH_HAL
    LibHalContext *ctx;
    GHashTable *ports; /* UDI -> portname */
#endif
};

G_DEFINE_TYPE(EntangleDeviceManager, entangle_device_manager, G_TYPE_OBJECT);


static void entangle_device_manager_finalize (GObject *object)
{
    EntangleDeviceManager *manager = ENTANGLE_DEVICE_MANAGER(object);
    EntangleDeviceManagerPrivate *priv = manager->priv;
    ENTANGLE_DEBUG("Finalize manager");

#if WITH_GUDEV
    if (priv->ctx)
        g_object_unref(priv->ctx);
#endif
#if WITH_HAL
    if (priv->ctx) {
        libhal_ctx_shutdown(priv->ctx, NULL);
        libhal_ctx_free(priv->ctx);
    }
    g_hash_table_unref(priv->ports);
#endif

    G_OBJECT_CLASS (entangle_device_manager_parent_class)->finalize (object);
}


static void entangle_device_manager_class_init(EntangleDeviceManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_device_manager_finalize;

    g_signal_new("device-added",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleDeviceManagerClass, device_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__STRING,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_STRING);

    g_signal_new("device-removed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleDeviceManagerClass, device_removed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__STRING,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_STRING);


    g_type_class_add_private(klass, sizeof(EntangleDeviceManagerPrivate));
}

#if WITH_GUDEV
static void do_udev_event(GUdevClient *client G_GNUC_UNUSED,
                          const char *action,
                          GUdevDevice *dev,
                          gpointer opaque)
{
    EntangleDeviceManager *manager = opaque;
    const gchar *sysfs;
    const gchar *usbbus, *usbdev;
    const gchar *devtype;
    gchar *port;

    if (strcmp(action, "add") != 0 &&
        strcmp(action, "remove") != 0)
        return;

    devtype = g_udev_device_get_devtype(dev);
    if (strcmp(devtype, "usb_device") != 0)
        return;

    sysfs = g_udev_device_get_sysfs_path(dev);

    usbbus = g_udev_device_get_property(dev, "BUSNUM");
    usbdev = g_udev_device_get_property(dev, "DEVNUM");
    port = g_strdup_printf("usb:%s,%s", usbbus, usbdev);

    ENTANGLE_DEBUG("%s device '%s' '%s'", action, sysfs, port);

    if (strcmp(action, "add") == 0) {
        g_signal_emit_by_name(manager, "device-added", port);
    } else {
        g_signal_emit_by_name(manager, "device-removed", port);
    }
    g_free(port);
}

#endif

#if WITH_HAL
static void do_device_added(LibHalContext *ctx, const char *udi)
{
    EntangleDeviceManager *manager = libhal_ctx_get_user_data(ctx);
    EntangleDeviceManagerPrivate *priv = manager->priv;
    char *type = NULL;
    char *port = NULL;
    int bus;
    int dev;

    ENTANGLE_DEBUG("Add UDI %s", udi);

    if (!(type = libhal_device_get_property_string(ctx, udi, "info.bus", NULL)))
        goto cleanup;
    ENTANGLE_DEBUG("type %s", type);
    if (strcmp(type, "usb_device") != 0) {
        ENTANGLE_DEBUG("Dont want");
        goto cleanup;
    }

    if (!(bus = libhal_device_get_property_int(ctx, udi, "usb_device.bus_number", NULL)))
        goto cleanup;
    ENTANGLE_DEBUG("bus %d", bus);
    if (!(dev = libhal_device_get_property_int(ctx, udi, "usb_device.linux.device_number", NULL)))
        goto cleanup;
    ENTANGLE_DEBUG("dev %d", dev);

    port = g_strdup_printf("usb:%03d,%03d", bus, dev);

    ENTANGLE_DEBUG("Add device '%s' '%s'", udi, port);

    g_hash_table_insert(priv->ports, g_strdup(udi), port);

    g_signal_emit_by_name(manager, "device-added", port);

 cleanup:
    g_free(type);
}

static void do_device_removed(LibHalContext *ctx, const char *udi)
{
    EntangleDeviceManager *manager = libhal_ctx_get_user_data(ctx);
    EntangleDeviceManagerPrivate *priv = manager->priv;
    char *port;

    ENTANGLE_DEBUG("Remove UDI %s", udi);

    port = g_hash_table_lookup(priv->ports, udi);

    if (port) {
        ENTANGLE_DEBUG("Remove device '%s' '%s'", udi, port);
        g_signal_emit_by_name(manager, "device-added", port);
        g_hash_table_remove(priv->ports, udi);
    }
}
#endif

EntangleDeviceManager *entangle_device_manager_new(void)
{
    return ENTANGLE_DEVICE_MANAGER(g_object_new(ENTANGLE_TYPE_DEVICE_MANAGER, NULL));
}


#if WITH_GUDEV
static void entangle_device_manager_init_devices(EntangleDeviceManager *manager)
{
    EntangleDeviceManagerPrivate *priv = manager->priv;
    GList *devs, *tmp;
    const gchar *const subsys[] = {
        "usb/usb_device", NULL,
    };

    ENTANGLE_DEBUG("Init udev");

    priv->ctx = g_udev_client_new(subsys);

    g_signal_connect(priv->ctx, "uevent", G_CALLBACK(do_udev_event), manager);

    devs = g_udev_client_query_by_subsystem(priv->ctx, "usb");

    tmp = devs;
    while (tmp) {
        GUdevDevice *dev = tmp->data;

        do_udev_event(priv->ctx, "add", dev, manager);

        g_object_unref(dev);
        tmp = tmp->next;
    }

    g_list_free(devs);
}
#endif


#if WITH_HAL
static void entangle_device_manager_init_devices(EntangleDeviceManager *manager)
{
    EntangleDeviceManagerPrivate *priv = manager->priv;
    DBusConnection *conn;
    int num_devs;
    char **udis;

    priv->ports = g_hash_table_new_full(g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        g_free);
    priv->ctx = libhal_ctx_new();

    if (!priv->ctx)
        return;

    libhal_ctx_set_device_added(priv->ctx, do_device_added);
    libhal_ctx_set_device_removed(priv->ctx, do_device_removed);

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
    libhal_ctx_set_dbus_connection(priv->ctx, conn);
    libhal_ctx_set_user_data(priv->ctx, manager);

    libhal_ctx_init(priv->ctx, NULL);

    dbus_connection_setup_with_g_main(conn, g_main_context_default());
    ENTANGLE_DEBUG("Listing for HAL events");

    udis = libhal_manager_find_device_string_match(priv->ctx,
                                                   "info.bus",
                                                   "usb_device",
                                                   &num_devs,
                                                   NULL);

    if (udis) {
        int i;
        for (i = 0 ; i < num_devs ; i++) {
            do_device_added(priv->ctx, udis[i]);
            g_free(udis[i]);
        }
        g_free(udis);
    }
}
#endif

static void entangle_device_manager_init(EntangleDeviceManager *manager)
{
    EntangleDeviceManagerPrivate *priv;

    priv = manager->priv = ENTANGLE_DEVICE_MANAGER_GET_PRIVATE(manager);

    entangle_device_manager_init_devices(manager);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
