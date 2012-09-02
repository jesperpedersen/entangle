/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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
#include <unistd.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "entangle-debug.h"
#include "entangle-camera-support.h"

#define ENTANGLE_CAMERA_SUPPORT_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_SUPPORT, EntangleCameraSupportPrivate))

gboolean do_support_close(GtkButton *src,
                          EntangleCameraSupport *support);
gboolean do_support_delete(GtkWidget *src,
                           GdkEvent *ev,
                           EntangleCameraSupport *support);

struct _EntangleCameraSupportPrivate {
    EntangleCameraList *cameraList;

    GtkBuilder *builder;
};

G_DEFINE_TYPE(EntangleCameraSupport, entangle_camera_support, G_TYPE_OBJECT);

enum {
    PROP_O,
    PROP_CAMERA_LIST,
};


static void entangle_camera_support_get_property(GObject *object,
                                                 guint prop_id,
                                                 GValue *value,
                                                 GParamSpec *pspec)
{
    EntangleCameraSupport *support = ENTANGLE_CAMERA_SUPPORT(object);
    EntangleCameraSupportPrivate *priv = support->priv;

    switch (prop_id)
        {
        case PROP_CAMERA_LIST:
            g_value_set_object(value, priv->cameraList);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_support_set_property(GObject *object,
                                                 guint prop_id,
                                                 const GValue *value,
                                                 GParamSpec *pspec)
{
    EntangleCameraSupport *support = ENTANGLE_CAMERA_SUPPORT(object);

    ENTANGLE_DEBUG("Set prop %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA_LIST:
            entangle_camera_support_set_camera_list(support, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_support_finalize(GObject *object)
{
    EntangleCameraSupport *support = ENTANGLE_CAMERA_SUPPORT(object);
    EntangleCameraSupportPrivate *priv = support->priv;

    if (priv->cameraList)
        g_object_unref(priv->cameraList);
    g_object_unref(priv->builder);

    G_OBJECT_CLASS (entangle_camera_support_parent_class)->finalize (object);
}


static void entangle_camera_support_class_init(EntangleCameraSupportClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_camera_support_finalize;
    object_class->get_property = entangle_camera_support_get_property;
    object_class->set_property = entangle_camera_support_set_property;

    g_signal_new("support-close",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraSupportClass, support_close),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_object_class_install_property(object_class,
                                    PROP_CAMERA_LIST,
                                    g_param_spec_object("camera-list",
                                                        "Camera List",
                                                        "Camera list to query",
                                                        ENTANGLE_TYPE_CAMERA_LIST,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleCameraSupportPrivate));
}


EntangleCameraSupport *entangle_camera_support_new(EntangleCameraList *cameraList)
{
    return ENTANGLE_CAMERA_SUPPORT(g_object_new(ENTANGLE_TYPE_CAMERA_SUPPORT,
                                                "camera-list", cameraList,
                                                NULL));
}


gboolean do_support_close(GtkButton *src G_GNUC_UNUSED,
                          EntangleCameraSupport *support)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support), TRUE);

    EntangleCameraSupportPrivate *priv = support->priv;
    ENTANGLE_DEBUG("support close");
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-support"));

    gtk_widget_hide(win);
    return TRUE;
}


gboolean do_support_delete(GtkWidget *src G_GNUC_UNUSED,
                           GdkEvent *ev G_GNUC_UNUSED,
                           EntangleCameraSupport *support)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support), FALSE);

    EntangleCameraSupportPrivate *priv = support->priv;
    ENTANGLE_DEBUG("support delete");
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-support"));

    gtk_widget_hide(win);
    return FALSE;
}


static void entangle_camera_support_init(EntangleCameraSupport *support)
{
    EntangleCameraSupportPrivate *priv;
    GError *error = NULL;

    priv = support->priv = ENTANGLE_CAMERA_SUPPORT_GET_PRIVATE(support);

    priv->builder = gtk_builder_new();

    if (access("./entangle", R_OK) == 0)
        gtk_builder_add_from_file(priv->builder, "frontend/entangle-camera-support.xml", &error);
    else
        gtk_builder_add_from_file(priv->builder, PKGDATADIR "/entangle-camera-support.xml", &error);

    if (error)
        g_error(_("Could not load user interface definition file: %s"), error->message);

    gtk_builder_connect_signals(priv->builder, support);
}


GtkWindow *entangle_camera_support_get_window(EntangleCameraSupport *support)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support), NULL);

    EntangleCameraSupportPrivate *priv = support->priv;
    return GTK_WINDOW(gtk_builder_get_object(priv->builder, "camera-support"));
}


void entangle_camera_support_show(EntangleCameraSupport *support)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support));

    EntangleCameraSupportPrivate *priv = support->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-support"));

    gtk_widget_show(win);
    gtk_window_present(GTK_WINDOW(win));
}


void entangle_camera_support_hide(EntangleCameraSupport *support)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support));

    EntangleCameraSupportPrivate *priv = support->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-support"));

    gtk_widget_hide(win);
}


static void do_support_refresh(EntangleCameraSupport *support)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support));

    EntangleCameraSupportPrivate *priv = support->priv;
    GtkWidget *text = GTK_WIDGET(gtk_builder_get_object(priv->builder, "info-text"));

    if (priv->cameraList) {
        gchar **cameras = entangle_camera_list_get_supported(priv->cameraList);
        gchar *tmp = g_strjoinv("\n", cameras);
        gtk_label_set_text(GTK_LABEL(text), tmp);
        g_free(tmp);
    } else {
        gtk_label_set_text(GTK_LABEL(text), "");
    }
}


void entangle_camera_support_set_camera_list(EntangleCameraSupport *support,
                                             EntangleCameraList *cameraList)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support));
    g_return_if_fail(ENTANGLE_IS_CAMERA_LIST(cameraList));

    EntangleCameraSupportPrivate *priv = support->priv;

    if (priv->cameraList)
        g_object_unref(priv->cameraList);
    priv->cameraList = cameraList;
    if (priv->cameraList)
        g_object_ref(priv->cameraList);

    do_support_refresh(support);
}


EntangleCameraList *entangle_camera_support_get_camera_list(EntangleCameraSupport *support)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_SUPPORT(support), NULL);

    EntangleCameraSupportPrivate *priv = support->priv;

    return priv->cameraList;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
