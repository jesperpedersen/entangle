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

#include <string.h>
#include <glade/glade.h>

#include "camera-info.h"
#include "camera.h"

#define CAPA_CAMERA_INFO_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_INFO, CapaCameraInfoPrivate))

struct _CapaCameraInfoPrivate {
  CapaCamera *camera;
  int data;

  GladeXML *glade;
};

G_DEFINE_TYPE(CapaCameraInfo, capa_camera_info, G_TYPE_OBJECT);

enum {
  PROP_O,
  PROP_CAMERA,
  PROP_DATA,
};

static void do_info_refresh(CapaCameraInfo *info)
{
  CapaCameraInfoPrivate *priv = info->priv;
  GtkWidget *text = glade_xml_get_widget(priv->glade, "info-text");
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));


  if (priv->camera) {
    switch (priv->data) {
    case CAPA_CAMERA_INFO_DATA_SUMMARY:
      gtk_text_buffer_set_text(buf, capa_camera_summary(priv->camera), -1);
      break;
    case CAPA_CAMERA_INFO_DATA_MANUAL:
      gtk_text_buffer_set_text(buf, capa_camera_manual(priv->camera), -1);
      break;
    case CAPA_CAMERA_INFO_DATA_DRIVER:
      gtk_text_buffer_set_text(buf, capa_camera_driver(priv->camera), -1);
      break;
    case CAPA_CAMERA_INFO_DATA_SUPPORTED:
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

static void capa_camera_info_get_property(GObject *object,
					  guint prop_id,
					  GValue *value,
					  GParamSpec *pspec)
{
  CapaCameraInfo *info = CAPA_CAMERA_INFO(object);
  CapaCameraInfoPrivate *priv = info->priv;

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

static void capa_camera_info_set_property(GObject *object,
					     guint prop_id,
					     const GValue *value,
					     GParamSpec *pspec)
{
  CapaCameraInfo *info = CAPA_CAMERA_INFO(object);
  CapaCameraInfoPrivate *priv = info->priv;

  fprintf(stderr, "Set prop %d\n", prop_id);

  switch (prop_id)
    {
    case PROP_CAMERA: {
      char *title;
      GtkWidget *win;
      if (priv->camera)
	g_object_unref(G_OBJECT(priv->camera));
      priv->camera = g_value_get_object(value);
      g_object_ref(G_OBJECT(priv->camera));

      title = g_strdup_printf("%s Camera Info - Capa",
			      capa_camera_model(priv->camera));

      win = glade_xml_get_widget(priv->glade, "camera-info");
      gtk_window_set_title(GTK_WINDOW(win), title);
      g_free(title);
      do_info_refresh(info);
    } break;

    case PROP_DATA:
      priv->data = g_value_get_enum(value);
      do_info_refresh(info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_camera_info_finalize (GObject *object)
{
  CapaCameraInfo *info = CAPA_CAMERA_INFO(object);
  CapaCameraInfoPrivate *priv = info->priv;

  if (priv->camera)
    g_object_unref(G_OBJECT(priv->camera));

  G_OBJECT_CLASS (capa_camera_info_parent_class)->finalize (object);
}

static void capa_camera_info_class_init(CapaCameraInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_camera_info_finalize;
  object_class->get_property = capa_camera_info_get_property;
  object_class->set_property = capa_camera_info_set_property;

  g_signal_new("info-close",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraInfoClass, info_close),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__VOID,
	       G_TYPE_NONE,
	       0);

  g_object_class_install_property(object_class,
				  PROP_CAMERA,
				  g_param_spec_object("camera",
						      "Camera",
						      "Camera to be managed",
						      CAPA_TYPE_CAMERA,
						      G_PARAM_READWRITE |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));
  g_object_class_install_property(object_class,
				  PROP_DATA,
				  g_param_spec_enum("data",
						    "Data",
						    "Data type to display",
						    CAPA_TYPE_CAMERA_INFO_DATA,
						    CAPA_CAMERA_INFO_DATA_SUMMARY,
						    G_PARAM_READWRITE |
						    G_PARAM_STATIC_NAME |
						    G_PARAM_STATIC_NICK |
						    G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaCameraInfoPrivate));
}

GType capa_camera_info_data_get_type(void)
{
  static GType etype = 0;

  if (etype == 0) {
    static const GEnumValue values[] = {
      { CAPA_CAMERA_INFO_DATA_SUMMARY, "CAPA_CAMERA_INFO_DATA_SUMMARY", "summary" },
      { CAPA_CAMERA_INFO_DATA_MANUAL, "CAPA_CAMERA_INFO_DATA_MANUAL", "manual" },
      { CAPA_CAMERA_INFO_DATA_DRIVER, "CAPA_CAMERA_INFO_DATA_DRIVER", "driver" },
      { CAPA_CAMERA_INFO_DATA_SUPPORTED, "CAPA_CAMERA_INFO_DATA_SUPPORTED", "supported" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CapaCameraInfoData", values );
  }

  return etype;
}

CapaCameraInfo *capa_camera_info_new(void)
{
  return CAPA_CAMERA_INFO(g_object_new(CAPA_TYPE_CAMERA_INFO, NULL));
}

static gboolean do_info_close(GtkButton *src G_GNUC_UNUSED,
			      CapaCameraInfo *info)
{
  CapaCameraInfoPrivate *priv = info->priv;
  fprintf(stderr, "info close\n");
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

  gtk_widget_hide(win);
  return TRUE;
}

static gboolean do_info_delete(GtkWidget *src G_GNUC_UNUSED,
			       GdkEvent *ev G_GNUC_UNUSED,
			       CapaCameraInfo *info)
{
  CapaCameraInfoPrivate *priv = info->priv;
  fprintf(stderr, "info delete\n");
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

  gtk_widget_hide(win);
  return FALSE;
}

static void capa_camera_info_init(CapaCameraInfo *info)
{
  CapaCameraInfoPrivate *priv;
  GtkWidget *txt;

  priv = info->priv = CAPA_CAMERA_INFO_GET_PRIVATE(info);

  priv->glade = glade_xml_new("capa.glade", "camera-info", "capa");

  glade_xml_signal_connect_data(priv->glade, "camera_info_close", G_CALLBACK(do_info_close), info);
  glade_xml_signal_connect_data(priv->glade, "camera_info_delete", G_CALLBACK(do_info_delete), info);

  txt = glade_xml_get_widget(priv->glade, "info-text");

  gtk_widget_set_sensitive(txt, FALSE);
}

void capa_camera_info_show(CapaCameraInfo *info)
{
  CapaCameraInfoPrivate *priv = info->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

  gtk_widget_show(win);
  gtk_window_present(GTK_WINDOW(win));
}

void capa_camera_info_hide(CapaCameraInfo *info)
{
  CapaCameraInfoPrivate *priv = info->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-info");

  gtk_widget_hide(win);
}

