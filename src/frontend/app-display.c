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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "app-display.h"
#include "camera-picker.h"
#include "camera-manager.h"
#include "device-manager.h"

#define CAPA_APP_DISPLAY_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_APP_DISPLAY, CapaAppDisplayPrivate))

struct _CapaAppDisplayPrivate {
  CapaApp *app;

  CapaDeviceManager *devManager;
  CapaCameraPicker *picker;
  GHashTable *managers;
};

G_DEFINE_TYPE(CapaAppDisplay, capa_app_display, G_TYPE_OBJECT);


static void capa_app_display_finalize (GObject *object)
{
  CapaAppDisplay *display = CAPA_APP_DISPLAY(object);
  CapaAppDisplayPrivate *priv = display->priv;

  fprintf(stderr, "Finalize display\n");

  capa_app_free(priv->app);

  g_object_unref(G_OBJECT(priv->picker));

  g_hash_table_unref(priv->managers);

  G_OBJECT_CLASS (capa_app_display_parent_class)->finalize (object);
}

static void capa_app_display_class_init(CapaAppDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_app_display_finalize;

  g_signal_new("app-closed",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaAppDisplayClass, app_closed),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__VOID,
	       G_TYPE_NONE,
	       0);

  g_type_class_add_private(klass, sizeof(CapaAppDisplayPrivate));
}


CapaAppDisplay *capa_app_display_new(void)
{
  return CAPA_APP_DISPLAY(g_object_new(CAPA_TYPE_APP_DISPLAY, NULL));
}

static gboolean capa_app_display_visible(CapaAppDisplay *display)
{
  CapaAppDisplayPrivate *priv = display->priv;
  GHashTableIter iter;
  gpointer key, value;

  if (capa_camera_picker_visible(priv->picker))
    return TRUE;

  g_hash_table_iter_init(&iter, priv->managers);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    CapaCameraManager *man = value;
    if (capa_camera_manager_visible(man))
      return TRUE;
  }

  return FALSE;
}

static void do_picker_close(CapaCameraPicker *picker, CapaAppDisplay *display)
{
  capa_camera_picker_hide(picker);

  if (!capa_app_display_visible(display)) {
    fprintf(stderr, "emit closed\n");
    g_signal_emit_by_name(display, "app-closed", NULL);
  }
}

static void do_device_refresh(CapaAppDisplay *display)
{
  CapaAppDisplayPrivate *priv = display->priv;
  CapaCameraList *cameras = capa_app_detect_cameras(priv->app);
  GValue val;

  memset(&val, 0, sizeof(val));
  g_value_init(&val, G_TYPE_POINTER);
  g_value_set_pointer(&val, cameras);

  fprintf(stderr, "emit refresh\n");
  g_object_set_property(G_OBJECT(priv->picker), "cameras", &val);

  g_value_unset(&val);
}

static void do_picker_refresh(CapaCameraPicker *picker G_GNUC_UNUSED, CapaAppDisplay *display)
{
  do_device_refresh(display);
}

static void do_device_addremove(CapaDeviceManager *manager G_GNUC_UNUSED,
				char *port G_GNUC_UNUSED,
				CapaAppDisplay *display)
{
  do_device_refresh(display);
}

static void do_manager_connect(CapaCameraManager *manager G_GNUC_UNUSED,
			       CapaAppDisplay *display)
{
  CapaAppDisplayPrivate *priv = display->priv;
  capa_camera_picker_show(priv->picker);
}


static void do_manager_disconnect(CapaCameraManager *manager, CapaAppDisplay *display)
{
  capa_camera_manager_hide(manager);
  fprintf(stderr, "Doing disconnect\n");
  if (!capa_app_display_visible(display)) {
    fprintf(stderr, "emit closed\n");
    g_signal_emit_by_name(display, "app-closed", NULL);
  }
}

static void do_picker_connect(CapaCameraPicker *picker, CapaCamera *cam, CapaAppDisplay *display)
{
  CapaAppDisplayPrivate *priv = display->priv;
  fprintf(stderr, "emit connect %p %s\n", cam, capa_camera_model(cam));
  CapaCameraManager *man;

  while (capa_camera_connect(cam) < 0) {
    int response;
    GtkWidget *msg = gtk_message_dialog_new(NULL,
					    GTK_DIALOG_MODAL,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_NONE,
					    "%s",
					    "Unable to connect to camera");

    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(msg),
					       "%s",
					       "Check that the camera is not\n\n"
					       " - opened by another photo <b>application</b>\n"
					       " - mounted as a <b>filesystem</b> on the desktop\n"
					       " - <b>turned off</b> to save battery power\n");

    gtk_dialog_add_button(GTK_DIALOG(msg), "Cancel", GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(msg), "Retry", GTK_RESPONSE_ACCEPT);

    response = gtk_dialog_run(GTK_DIALOG(msg));

    gtk_widget_hide(msg);
    //g_object_unref(G_OBJECT(msg));

    if (response == GTK_RESPONSE_CANCEL)
      return;
  }

  man = g_hash_table_lookup(priv->managers, capa_camera_model(cam));
  if (!man) {
    GValue camval;
    memset(&camval, 0, sizeof camval);
    g_value_init(&camval, G_TYPE_OBJECT);
    g_value_set_object(&camval, cam);
    man = capa_camera_manager_new();

    g_signal_connect(G_OBJECT(man), "manager-disconnect", G_CALLBACK(do_manager_disconnect), display);
    g_signal_connect(G_OBJECT(man), "manager-connect", G_CALLBACK(do_manager_connect), display);

    g_object_set_property(G_OBJECT(man), "camera", &camval);
    g_hash_table_insert(priv->managers, g_strdup(capa_camera_model(cam)), man);
    g_value_unset(&camval);
  }
  capa_camera_manager_show(man);
  capa_camera_picker_hide(picker);
}

static void capa_app_display_init(CapaAppDisplay *display)
{
  CapaAppDisplayPrivate *priv;

  priv = display->priv = CAPA_APP_DISPLAY_GET_PRIVATE(display);

  priv->app = capa_app_new();
  priv->picker = capa_camera_picker_new();
  priv->managers = g_hash_table_new_full(g_str_hash, g_str_equal,
					 g_free, g_object_unref);
  priv->devManager = capa_device_manager_new();

  g_signal_connect(priv->picker, "picker-close", G_CALLBACK(do_picker_close), display);
  g_signal_connect(priv->picker, "picker-refresh", G_CALLBACK(do_picker_refresh), display);
  g_signal_connect(priv->picker, "picker-connect", G_CALLBACK(do_picker_connect), display);

  g_signal_connect(priv->devManager, "device-added", G_CALLBACK(do_device_addremove), display);
  g_signal_connect(priv->devManager, "device-removed", G_CALLBACK(do_device_addremove), display);
}


void capa_app_display_show(CapaAppDisplay *display)
{
  CapaAppDisplayPrivate *priv = display->priv;

  do_picker_refresh(priv->picker, display);

  capa_camera_picker_show(priv->picker);
}
