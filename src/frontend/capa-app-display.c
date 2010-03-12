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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <unique/unique.h>

#include "capa-debug.h"
#include "capa-app-display.h"
#include "capa-camera-picker.h"
#include "capa-camera-manager.h"

#define CAPA_APP_DISPLAY_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_APP_DISPLAY, CapaAppDisplayPrivate))

struct _CapaAppDisplayPrivate {
    UniqueApp *unique;

    CapaCameraPicker *picker;
    CapaCameraManager *manager;
};

G_DEFINE_TYPE(CapaAppDisplay, capa_app_display, CAPA_TYPE_APP);


static void capa_app_display_finalize (GObject *object)
{
    CapaAppDisplay *display = CAPA_APP_DISPLAY(object);
    CapaAppDisplayPrivate *priv = display->priv;

    CAPA_DEBUG("Finalize display");

    g_object_unref(G_OBJECT(priv->unique));
    g_object_unref(G_OBJECT(priv->picker));

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


static void do_picker_close(CapaCameraPicker *picker, CapaAppDisplay *display G_GNUC_UNUSED)
{
    capa_camera_picker_hide(picker);
}


static void do_picker_refresh(CapaCameraPicker *picker G_GNUC_UNUSED, CapaAppDisplay *display)
{
    capa_app_refresh_cameras(CAPA_APP(display));
}

static void do_manager_connect(CapaCameraManager *manager G_GNUC_UNUSED,
                               CapaAppDisplay *display)
{
    capa_app_display_show(display);
}


static void do_picker_connect(CapaCameraPicker *picker, CapaCamera *cam, CapaAppDisplay *display)
{
    CapaAppDisplayPrivate *priv = display->priv;
    CAPA_DEBUG("emit connect %p %s", cam, capa_camera_get_model(cam));

    while (!capa_camera_connect(cam)) {
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
                                                   " - in <b>sleep mode</b> to save battery power\n");

        gtk_dialog_add_button(GTK_DIALOG(msg), "Cancel", GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button(GTK_DIALOG(msg), "Retry", GTK_RESPONSE_ACCEPT);

        response = gtk_dialog_run(GTK_DIALOG(msg));

        gtk_widget_hide(msg);
        //g_object_unref(G_OBJECT(msg));

        if (response == GTK_RESPONSE_CANCEL)
            return;
    }

    g_object_set(G_OBJECT(priv->manager), "camera", cam, NULL);
    capa_camera_picker_hide(picker);
}

static void do_set_icons(void)
{
    GList *icons = NULL;
    int iconSizes[] = { 16, 32, 48, 64, 128, 0 };

    for (int i = 0 ; iconSizes[i] != 0 ; i++) {
        char *local = g_strdup_printf("./capa-%dx%d.png", iconSizes[i], iconSizes[i]);
        if (access(local, R_OK) < 0) {
            g_free(local);
            local = g_strdup_printf("%s/capa-%dx%d.png", PKGDATADIR, iconSizes[i], iconSizes[i]);
        }
        GdkPixbuf *pix = gdk_pixbuf_new_from_file(local, NULL);
        if (pix)
            icons = g_list_append(icons, pix);
    }

    gtk_window_set_default_icon_list(icons);
}

static UniqueResponse
do_unique_message(UniqueApp *app G_GNUC_UNUSED,
                  UniqueCommand command,
                  UniqueMessageData *message_data G_GNUC_UNUSED,
                  guint time_ms G_GNUC_UNUSED,
                  gpointer data)
{
    CapaAppDisplay *display = CAPA_APP_DISPLAY(data);
    CapaAppDisplayPrivate *priv = display->priv;

    if (command == UNIQUE_ACTIVATE) {
        capa_camera_manager_show(priv->manager);
    }
    return UNIQUE_RESPONSE_OK;
}

static void do_camera_removed(CapaCameraList *cameras G_GNUC_UNUSED,
                              CapaCamera *camera,
                              gpointer data)
{
    CapaAppDisplay *display = CAPA_APP_DISPLAY(data);
    CapaAppDisplayPrivate *priv = display->priv;
    CapaCamera *current;

    g_object_get(priv->manager, "camera", &current, NULL);

    CAPA_DEBUG("Check removed camera '%s' %p, against '%s' %p",
               capa_camera_get_model(camera), camera,
               current ? capa_camera_get_model(current) : "<none>", current);

    if (current == camera)
        g_object_set(priv->manager, "camera", NULL, NULL);
}

static void capa_app_display_init(CapaAppDisplay *display)
{
    CapaAppDisplayPrivate *priv;
    CapaCameraList *cameras;

    priv = display->priv = CAPA_APP_DISPLAY_GET_PRIVATE(display);

    do_set_icons();

    priv->picker = capa_camera_picker_new(capa_app_get_cameras(CAPA_APP(display)));
    priv->manager = capa_camera_manager_new(capa_app_get_preferences(CAPA_APP(display)),
                                            capa_app_get_plugin_manager(CAPA_APP(display)));

    cameras = capa_app_get_cameras(CAPA_APP(display));

    g_signal_connect(priv->picker, "picker-close", G_CALLBACK(do_picker_close), display);
    g_signal_connect(priv->picker, "picker-refresh", G_CALLBACK(do_picker_refresh), display);
    g_signal_connect(priv->picker, "picker-connect", G_CALLBACK(do_picker_connect), display);

    g_signal_connect(G_OBJECT(cameras), "camera-removed", G_CALLBACK(do_camera_removed), display);
    g_signal_connect(G_OBJECT(priv->manager), "manager-connect",
                     G_CALLBACK(do_manager_connect), display);

    priv->unique = unique_app_new("org.capa_project.Display", NULL);
    g_signal_connect(priv->unique, "message-received",
                     G_CALLBACK(do_unique_message), display);
}


gboolean capa_app_display_show(CapaAppDisplay *display)
{
    CapaAppDisplayPrivate *priv = display->priv;
    CapaCameraList *cameras;
    gboolean choose = TRUE;

    if (unique_app_is_running(priv->unique)) {
        unique_app_send_message(priv->unique, UNIQUE_ACTIVATE, NULL);
        return FALSE;
    }

    cameras = capa_app_get_cameras(CAPA_APP(display));

    if (capa_camera_list_count(cameras) == 1) {
        CapaCamera *cam = capa_camera_list_get(cameras, 0);

        if (capa_camera_connect(cam)) {
            g_object_set(G_OBJECT(priv->manager), "camera", cam, NULL);
            choose = FALSE;
        }
    }

    capa_camera_manager_show(priv->manager);
    if (choose)
        capa_camera_picker_show(priv->picker);

    return TRUE;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
