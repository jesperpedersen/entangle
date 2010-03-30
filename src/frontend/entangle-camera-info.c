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
#include <glade/glade.h>
#include <unistd.h>

#include "entangle-debug.h"
#include "entangle-camera-info.h"
#include "entangle-camera.h"

#define ENTANGLE_CAMERA_INFO_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_INFO, EntangleCameraInfoPrivate))

struct _EntangleCameraInfoPrivate {
    EntangleCamera *camera;
    int data;

    GladeXML *glade;
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


static void entangle_camera_info_finalize (GObject *object)
{
    EntangleCameraInfo *info = ENTANGLE_CAMERA_INFO(object);
    EntangleCameraInfoPrivate *priv = info->priv;

    if (priv->camera)
        g_object_unref(priv->camera);
    g_object_unref(priv->glade);

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
            { ENTANGLE_CAMERA_INFO_DATA_SUPPORTED, "ENTANGLE_CAMERA_INFO_DATA_SUPPORTED", "supported" },
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


static gboolean do_info_close(GtkButton *src G_GNUC_UNUSED,
                              EntangleCameraInfo *info)
{
    EntangleCameraInfoPrivate *priv = info->priv;
    ENTANGLE_DEBUG("info close");
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

    gtk_widget_hide(win);
    return TRUE;
}


static gboolean do_info_delete(GtkWidget *src G_GNUC_UNUSED,
                               GdkEvent *ev G_GNUC_UNUSED,
                               EntangleCameraInfo *info)
{
    EntangleCameraInfoPrivate *priv = info->priv;
    ENTANGLE_DEBUG("info delete");
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

    gtk_widget_hide(win);
    return FALSE;
}


static void entangle_camera_info_init(EntangleCameraInfo *info)
{
    EntangleCameraInfoPrivate *priv;
    GtkWidget *txt;

    priv = info->priv = ENTANGLE_CAMERA_INFO_GET_PRIVATE(info);

    if (access("./entangle.glade", R_OK) == 0)
        priv->glade = glade_xml_new("entangle.glade", "camera-info", "entangle");
    else
        priv->glade = glade_xml_new(PKGDATADIR "/entangle.glade", "camera-info", "entangle");

    glade_xml_signal_connect_data(priv->glade, "camera_info_close", G_CALLBACK(do_info_close), info);
    glade_xml_signal_connect_data(priv->glade, "camera_info_delete", G_CALLBACK(do_info_delete), info);

    txt = glade_xml_get_widget(priv->glade, "info-text");

    gtk_widget_set_sensitive(txt, FALSE);
}


void entangle_camera_info_show(EntangleCameraInfo *info)
{
    EntangleCameraInfoPrivate *priv = info->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

    gtk_widget_show(win);
    gtk_window_present(GTK_WINDOW(win));
}


void entangle_camera_info_hide(EntangleCameraInfo *info)
{
    EntangleCameraInfoPrivate *priv = info->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

    gtk_widget_hide(win);
}


static void do_info_refresh(EntangleCameraInfo *info)
{
    EntangleCameraInfoPrivate *priv = info->priv;
    GtkWidget *text = glade_xml_get_widget(priv->glade, "info-text");
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));


    if (priv->camera) {
        switch (priv->data) {
        case ENTANGLE_CAMERA_INFO_DATA_SUMMARY:
            gtk_text_buffer_set_text(buf, entangle_camera_get_summary(priv->camera), -1);
            break;
        case ENTANGLE_CAMERA_INFO_DATA_MANUAL:
            gtk_text_buffer_set_text(buf, entangle_camera_get_manual(priv->camera), -1);
            break;
        case ENTANGLE_CAMERA_INFO_DATA_DRIVER:
            gtk_text_buffer_set_text(buf, entangle_camera_get_driver(priv->camera), -1);
            break;
        case ENTANGLE_CAMERA_INFO_DATA_SUPPORTED:
            gtk_text_buffer_set_text(buf, "supported", -1);
            break;
        default:
            gtk_text_buffer_set_text(buf, "unknown", -1);
            break;
        }
    } else {
        gtk_text_buffer_set_text(buf, "", -1);
    }
}


void entangle_camera_info_set_data(EntangleCameraInfo *info,
                               EntangleCameraInfoData data)
{
    EntangleCameraInfoPrivate *priv = info->priv;

    priv->data = data;

    do_info_refresh(info);
}


EntangleCameraInfoData entangle_camera_info_get_data(EntangleCameraInfo *info)
{
    EntangleCameraInfoPrivate *priv = info->priv;

    return priv->data;
}


void entangle_camera_info_set_camera(EntangleCameraInfo *info,
                                 EntangleCamera *camera)
{
    EntangleCameraInfoPrivate *priv = info->priv;
    char *title;
    GtkWidget *win;

    if (priv->camera)
        g_object_unref(priv->camera);
    priv->camera = camera;
    if (priv->camera) {
        g_object_ref(priv->camera);
        title = g_strdup_printf("%s Camera Info - Entangle",
                                entangle_camera_get_model(priv->camera));
    } else {
        title = g_strdup("Camera Info - Entangle");
    }

    win = glade_xml_get_widget(priv->glade, "camera-info");
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);

    do_info_refresh(info);
}


EntangleCamera *entangle_camera_info_get_camera(EntangleCameraInfo *info)
{
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
