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

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <libhal.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "device-manager.h"

#define CAPA_DEVICE_MANAGER_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_DEVICE_MANAGER, CapaDeviceManagerPrivate))

struct _CapaDeviceManagerPrivate {
  LibHalContext *ctx;
  GHashTable *ports; /* UDI -> portname */
};

G_DEFINE_TYPE(CapaDeviceManager, capa_device_manager, G_TYPE_OBJECT);


static void capa_device_manager_finalize (GObject *object)
{
  CapaDeviceManager *manager = CAPA_DEVICE_MANAGER(object);
  CapaDeviceManagerPrivate *priv = manager->priv;
  fprintf(stderr, "Finalize manager\n");

  if (priv->ctx) {
    libhal_ctx_shutdown(priv->ctx, NULL);
    libhal_ctx_free(priv->ctx);
  }

  g_hash_table_unref(priv->ports);

  G_OBJECT_CLASS (capa_device_manager_parent_class)->finalize (object);
}


static void capa_device_manager_class_init(CapaDeviceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_device_manager_finalize;

  g_signal_new("device-added",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaDeviceManagerClass, device_added),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__STRING,
	       G_TYPE_NONE,
	       1,
	       G_TYPE_STRING);

  g_signal_new("device-removed",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaDeviceManagerClass, device_removed),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__STRING,
	       G_TYPE_NONE,
	       1,
	       G_TYPE_STRING);


  g_type_class_add_private(klass, sizeof(CapaDeviceManagerPrivate));
}


static void do_device_added(LibHalContext *ctx, const char *udi)
{
  CapaDeviceManager *manager = libhal_ctx_get_user_data(ctx);
  CapaDeviceManagerPrivate *priv = manager->priv;
  char *type = NULL;
  char *port = NULL;
  int bus;
  int dev;

  fprintf(stderr, "Add UDI %s\n", udi);

  if (!(type = libhal_device_get_property_string(ctx, udi, "info.bus", NULL)))
    goto cleanup;
  fprintf(stderr, "type %s\n", type);
  if (strcmp(type, "usb_device") != 0) {
    fprintf(stderr, "Dont want\n");
    goto cleanup;
  }

  if (!(bus = libhal_device_get_property_int(ctx, udi, "usb_device.bus_number", NULL)))
    goto cleanup;
  fprintf(stderr, "bus %d\n", bus);
  if (!(dev = libhal_device_get_property_int(ctx, udi, "usb_device.linux.device_number", NULL)))
    goto cleanup;
  fprintf(stderr, "dev %d\n", dev);

  port = g_strdup_printf("usb:%03d,%03d", bus, dev);

  fprintf(stderr, "Add device '%s' '%s'\n", udi, port);

  g_hash_table_insert(priv->ports, g_strdup(udi), port);

  g_signal_emit_by_name(G_OBJECT(manager), "device-added", port);

 cleanup:
  g_free(port);
  g_free(type);
}

static void do_device_removed(LibHalContext *ctx, const char *udi)
{
  CapaDeviceManager *manager = libhal_ctx_get_user_data(ctx);
  CapaDeviceManagerPrivate *priv = manager->priv;
  char *port;

  fprintf(stderr, "Remove UDI %s\n", udi);

  port = g_hash_table_lookup(priv->ports, udi);

  if (port) {
    fprintf(stderr, "Remove device '%s' '%s'\n", udi, port);
    g_signal_emit_by_name(G_OBJECT(manager), "device-added", port);
    g_hash_table_remove(priv->ports, udi);
  }
}

CapaDeviceManager *capa_device_manager_new(void)
{
  return CAPA_DEVICE_MANAGER(g_object_new(CAPA_TYPE_DEVICE_MANAGER, NULL));
}


static void capa_device_manager_init(CapaDeviceManager *manager)
{
  CapaDeviceManagerPrivate *priv;
  DBusConnection *conn;
  int num_devs;
  char **udis;

  priv = manager->priv = CAPA_DEVICE_MANAGER_GET_PRIVATE(manager);

  priv->ctx = libhal_ctx_new();
  priv->ports = g_hash_table_new_full(g_str_hash,
				      g_str_equal,
				      g_free,
				      g_free);

  if (!priv->ctx)
    return;

  libhal_ctx_set_device_added(priv->ctx, do_device_added);
  libhal_ctx_set_device_removed(priv->ctx, do_device_removed);

  conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
  libhal_ctx_set_dbus_connection(priv->ctx, conn);
  libhal_ctx_set_user_data(priv->ctx, manager);

  libhal_ctx_init(priv->ctx, NULL);

  dbus_connection_setup_with_g_main(conn, g_main_context_default());
  fprintf(stderr, "Listing for HAL events\n");

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

