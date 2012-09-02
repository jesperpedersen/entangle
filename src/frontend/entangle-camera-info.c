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
#include "entangle-camera-info.h"
#include "entangle-camera.h"

#define ENTANGLE_CAMERA_INFO_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_INFO, EntangleCameraInfoPrivate))

gboolean do_info_close(GtkButton *src,
                       EntangleCameraInfo *info);
gboolean do_info_delete(GtkWidget *src,
                        GdkEvent *ev,
                        EntangleCameraInfo *info);

struct _EntangleCameraInfoPrivate {
    EntangleCamera *camera;
    int data;

    GtkBuilder *builder;
};

G_DEFINE_TYPE(EntangleCameraInfo, entangle_camera_info, G_TYPE_OBJECT);

enum {
    PROP_O,
    PROP_CAMERA,
    PROP_DATA,
};


static void entangle_camera_info_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    EntangleCameraInfo *info = ENTANGLE_CAMERA_INFO(object);
    EntangleCameraInfoPrivate *priv = info->priv;

    switch (prop_id)
        {
        case PROP_CAMERA:
            g_value_set_object(value, priv->camera);
            break;

        case PROP_DATA:
            g_value_set_enum(value, priv->data);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_info_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    EntangleCameraInfo *info = ENTANGLE_CAMERA_INFO(object);

    ENTANGLE_DEBUG("Set prop %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            entangle_camera_info_set_camera(info, g_value_get_object(value));
            break;

        case PROP_DATA:
            entangle_camera_info_set_data(info, g_value_get_enum(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_camera_info_finalize(GObject *object)
{
    EntangleCameraInfo *info = ENTANGLE_CAMERA_INFO(object);
    EntangleCameraInfoPrivate *priv = info->priv;

    if (priv->camera)
        g_object_unref(priv->camera);
    g_object_unref(priv->builder);

    G_OBJECT_CLASS (entangle_camera_info_parent_class)->finalize (object);
}


static void entangle_camera_info_class_init(EntangleCameraInfoClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_camera_info_finalize;
    object_class->get_property = entangle_camera_info_get_property;
    object_class->set_property = entangle_camera_info_set_property;

    g_signal_new("info-close",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraInfoClass, info_close),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to be managed",
                                                        ENTANGLE_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DATA,
                                    g_param_spec_enum("data",
                                                      "Data",
                                                      "Data type to display",
                                                      ENTANGLE_TYPE_CAMERA_INFO_DATA,
                                                      ENTANGLE_CAMERA_INFO_DATA_SUMMARY,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleCameraInfoPrivate));
}


GType entangle_camera_info_data_get_type(void)
{
    static GType etype = 0;

    if (etype == 0) {
        static const GEnumValue values[] = {
            { ENTANGLE_CAMERA_INFO_DATA_SUMMARY, "ENTANGLE_CAMERA_INFO_DATA_SUMMARY", "summary" },
            { ENTANGLE_CAMERA_INFO_DATA_MANUAL, "ENTANGLE_CAMERA_INFO_DATA_MANUAL", "manual" },
            { ENTANGLE_CAMERA_INFO_DATA_DRIVER, "ENTANGLE_CAMERA_INFO_DATA_DRIVER", "driver" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static ("EntangleCameraInfoData", values );
    }

    return etype;
}


EntangleCameraInfo *entangle_camera_info_new(EntangleCamera *camera,
                                             EntangleCameraInfoData data)
{
    return ENTANGLE_CAMERA_INFO(g_object_new(ENTANGLE_TYPE_CAMERA_INFO,
                                         "camera", camera,
                                         "data", data,
                                         NULL));
}


gboolean do_info_close(GtkButton *src G_GNUC_UNUSED,
                       EntangleCameraInfo *info)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_INFO(info), FALSE);

    EntangleCameraInfoPrivate *priv = info->priv;
    ENTANGLE_DEBUG("info close");
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-info"));

    gtk_widget_hide(win);
    return TRUE;
}


gboolean do_info_delete(GtkWidget *src G_GNUC_UNUSED,
                        GdkEvent *ev G_GNUC_UNUSED,
                        EntangleCameraInfo *info)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_INFO(info), FALSE);

    EntangleCameraInfoPrivate *priv = info->priv;
    ENTANGLE_DEBUG("info delete");
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-info"));

    gtk_widget_hide(win);
    return FALSE;
}


static void entangle_camera_info_init(EntangleCameraInfo *info)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_INFO(info));

    EntangleCameraInfoPrivate *priv;
    GError *error = NULL;

    priv = info->priv = ENTANGLE_CAMERA_INFO_GET_PRIVATE(info);

    priv->builder = gtk_builder_new();

    if (access("./entangle", R_OK) == 0)
        gtk_builder_add_from_file(priv->builder, "frontend/entangle-camera-info.xml", &error);
    else
        gtk_builder_add_from_file(priv->builder, PKGDATADIR "/entangle-camera-info.xml", &error);

    if (error)
        g_error(_("Could not load user interface definition file: %s"), error->message);

    gtk_builder_connect_signals(priv->builder, info);
}


GtkWindow *entangle_camera_info_get_window(EntangleCameraInfo *info)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_INFO(info), NULL);

    EntangleCameraInfoPrivate *priv = info->priv;
    return GTK_WINDOW(gtk_builder_get_object(priv->builder, "camera-info"));
}


void entangle_camera_info_show(EntangleCameraInfo *info)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_INFO(info));

    EntangleCameraInfoPrivate *priv = info->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-info"));

    gtk_widget_show(win);
    gtk_window_present(GTK_WINDOW(win));
}


void entangle_camera_info_hide(EntangleCameraInfo *info)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_INFO(info));

    EntangleCameraInfoPrivate *priv = info->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-info"));

    gtk_widget_hide(win);
}


static void do_info_refresh(EntangleCameraInfo *info)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_INFO(info));

    EntangleCameraInfoPrivate *priv = info->priv;
    GtkWidget *text = GTK_WIDGET(gtk_builder_get_object(priv->builder, "info-text"));

    if (priv->camera) {
        switch (priv->data) {
        case ENTANGLE_CAMERA_INFO_DATA_SUMMARY: {
            char *str = entangle_camera_get_summary(priv->camera);
            gtk_label_set_text(GTK_LABEL(text), str);
            g_free(str);
        }   break;
        case ENTANGLE_CAMERA_INFO_DATA_MANUAL: {
            char *str = entangle_camera_get_manual(priv->camera);
            gtk_label_set_text(GTK_LABEL(text), str);
            g_free(str);
        }   break;
        case ENTANGLE_CAMERA_INFO_DATA_DRIVER: {
            char *str = entangle_camera_get_driver(priv->camera);
            gtk_label_set_text(GTK_LABEL(text), str);
            g_free(str);
        }   break;
        default:
            gtk_label_set_text(GTK_LABEL(text), _("Unknown information"));
            break;
        }
    } else {
        gtk_label_set_text(GTK_LABEL(text), "");
    }
}


void entangle_camera_info_set_data(EntangleCameraInfo *info,
                                   EntangleCameraInfoData data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_INFO(info));

    EntangleCameraInfoPrivate *priv = info->priv;

    priv->data = data;

    do_info_refresh(info);
}


EntangleCameraInfoData entangle_camera_info_get_data(EntangleCameraInfo *info)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_INFO(info), 0);

    EntangleCameraInfoPrivate *priv = info->priv;

    return priv->data;
}


void entangle_camera_info_set_camera(EntangleCameraInfo *info,
                                     EntangleCamera *camera)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_INFO(info));
    g_return_if_fail(!camera || ENTANGLE_IS_CAMERA_INFO(camera));

    EntangleCameraInfoPrivate *priv = info->priv;
    char *title;
    GtkWidget *win;

    if (priv->camera)
        g_object_unref(priv->camera);
    priv->camera = camera;
    if (priv->camera) {
        g_object_ref(priv->camera);
        title = g_strdup_printf(_("%s Camera Info - Entangle"),
                                entangle_camera_get_model(priv->camera));
    } else {
        title = g_strdup(_("Camera Info - Entangle"));
    }

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-info"));
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);

    do_info_refresh(info);
}


EntangleCamera *entangle_camera_info_get_camera(EntangleCameraInfo *info)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_INFO(info), NULL);

    EntangleCameraInfoPrivate *priv = info->priv;

    return priv->camera;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
