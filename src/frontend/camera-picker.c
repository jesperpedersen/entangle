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

#include "camera-picker.h"
#include "camera-list.h"

#define CAPA_CAMERA_PICKER_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_PICKER, CapaCameraPickerPrivate))

struct _CapaCameraPickerPrivate {
  CapaCameraList *cameras;
  GtkListStore *model;

  GladeXML *glade;
};

G_DEFINE_TYPE(CapaCameraPicker, capa_camera_picker, G_TYPE_OBJECT);

enum {
  PROP_O,
  PROP_CAMERAS
};


static void capa_camera_cell_data_model_func(GtkTreeViewColumn *col G_GNUC_UNUSED,
					     GtkCellRenderer *cell,
					     GtkTreeModel *model,
					     GtkTreeIter *iter,
					     gpointer data G_GNUC_UNUSED)
{
  GValue val;
  CapaCamera *cam;

  memset(&val, 0, sizeof val);

  gtk_tree_model_get_value(model, iter, 0, &val);

  cam = g_value_get_object(&val);

  g_object_set(cell, "text", capa_camera_model(cam), NULL);
}

static void capa_camera_cell_data_port_func(GtkTreeViewColumn *col G_GNUC_UNUSED,
					    GtkCellRenderer *cell,
					    GtkTreeModel *model,
					    GtkTreeIter *iter,
					    gpointer data G_GNUC_UNUSED)
{
  GValue val;
  CapaCamera *cam;

  memset(&val, 0, sizeof val);

  gtk_tree_model_get_value(model, iter, 0, &val);

  cam = g_value_get_object(&val);

  g_object_set(cell, "text", capa_camera_port(cam), NULL);
}



static void capa_camera_picker_update_model(CapaCameraPicker *picker, CapaCameraList *cameras)
{
  CapaCameraPickerPrivate *priv = picker->priv;
  GtkWidget *warning;
  GtkWidget *list;
  GtkWidget *win;
  fprintf(stderr, "Refresh model\n");
  gtk_list_store_clear(priv->model);
  capa_camera_list_free(priv->cameras);

  warning = glade_xml_get_widget(priv->glade, "warning-no-cameras");
  list = glade_xml_get_widget(priv->glade, "camera-list");
  win = glade_xml_get_widget(priv->glade, "camera-picker");

  priv->cameras = cameras;
  if (!cameras) {
    gtk_widget_set_sensitive(list, FALSE);
    gtk_widget_show(warning);
    return;
  }

  for (int i = 0 ; i < capa_camera_list_count(priv->cameras) ; i++) {
    CapaCamera *cam = capa_camera_list_get(priv->cameras, i);
    GtkTreeIter iter;

    gtk_list_store_append(priv->model, &iter);

    gtk_list_store_set(priv->model, &iter, 0, cam, -1);
  }
  if (capa_camera_list_count(priv->cameras)) {
    int w, h;
    gtk_window_get_default_size(GTK_WINDOW(win), &w, &h);
    gtk_window_resize(GTK_WINDOW(win), w, h);
    gtk_widget_set_sensitive(list, TRUE);
    gtk_widget_hide(warning);
  } else {
    gtk_widget_set_sensitive(list, FALSE);
    gtk_widget_show(warning);
  }
}

static void capa_camera_picker_get_property(GObject *object,
					    guint prop_id,
					    GValue *value,
					    GParamSpec *pspec)
{
  CapaCameraPicker *picker = CAPA_CAMERA_PICKER(object);
  CapaCameraPickerPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_CAMERAS:
      g_value_set_pointer(value, priv->cameras);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_camera_picker_set_property(GObject *object,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *pspec)
{
  CapaCameraPicker *picker = CAPA_CAMERA_PICKER(object);

  fprintf(stderr, "Set prop %d\n", prop_id);

  switch (prop_id)
    {
    case PROP_CAMERAS:
      capa_camera_picker_update_model(picker, g_value_get_pointer(value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_camera_picker_finalize (GObject *object)
{
  CapaCameraPicker *picker = CAPA_CAMERA_PICKER(object);
  CapaCameraPickerPrivate *priv = picker->priv;

  capa_camera_list_free(priv->cameras);
  g_object_unref(priv->model);

  G_OBJECT_CLASS (capa_camera_picker_parent_class)->finalize (object);
}

static void capa_camera_picker_class_init(CapaCameraPickerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_camera_picker_finalize;
  object_class->get_property = capa_camera_picker_get_property;
  object_class->set_property = capa_camera_picker_set_property;

  g_signal_new("picker-connect",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraPickerClass, picker_connect),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__OBJECT,
	       G_TYPE_NONE,
	       1,
	       G_TYPE_OBJECT);
  g_signal_new("picker-refresh",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraPickerClass, picker_refresh),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__VOID,
	       G_TYPE_NONE,
	       0);
  g_signal_new("picker-close",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraPickerClass, picker_close),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__VOID,
	       G_TYPE_NONE,
	       0);

  g_object_class_install_property(object_class,
				  PROP_CAMERAS,
				  g_param_spec_pointer("cameras",
						       "Camera List",
						       "List of known camera objects",
						       G_PARAM_READWRITE |
						       G_PARAM_STATIC_NAME |
						       G_PARAM_STATIC_NICK |
						       G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaCameraPickerPrivate));
}


CapaCameraPicker *capa_camera_picker_new(void)
{
  return CAPA_CAMERA_PICKER(g_object_new(CAPA_TYPE_CAMERA_PICKER, NULL));
}

static void do_picker_close(GtkButton *src G_GNUC_UNUSED, CapaCameraPicker *picker)
{
  fprintf(stderr, "picker close\n");
  g_signal_emit_by_name(picker, "picker-close", NULL);
}

static void do_picker_refresh(GtkButton *src G_GNUC_UNUSED, CapaCameraPicker *picker)
{
  fprintf(stderr, "picker refresh %p\n", picker);
  g_signal_emit_by_name(picker, "picker-refresh", NULL);
}

static CapaCamera *capa_picker_get_selected_camera(CapaCameraPicker *picker)
{
  CapaCameraPickerPrivate *priv = picker->priv;
  GtkWidget *list;
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  gboolean selected;
  GValue val;

  fprintf(stderr, "select camera\n");

  list = glade_xml_get_widget(priv->glade, "camera-list");

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

  selected = gtk_tree_selection_get_selected(sel, NULL, &iter);
  if (!selected)
    return NULL;

  memset(&val, 0, sizeof val);
  gtk_tree_model_get_value(GTK_TREE_MODEL(priv->model), &iter, 0, &val);

  return g_value_get_object(&val);
}

static void do_picker_activate(GtkTreeView *src G_GNUC_UNUSED,
			       GtkTreePath *path G_GNUC_UNUSED,
			       GtkTreeViewColumn *col G_GNUC_UNUSED,
			       CapaCameraPicker *picker)
{
  CapaCamera *cam;
  cam = capa_picker_get_selected_camera(picker);
  fprintf(stderr, "picker activate %p %p\n", picker, cam);

  if (cam) {
    GValue val;
    memset(&val, 0, sizeof val);
    g_value_init(&val, G_TYPE_OBJECT);
    g_value_set_object(&val, cam);
    //g_signal_emit_by_name(picker, "picker-connect", &val);
    g_signal_emit_by_name(picker, "picker-connect", cam);
  }
}


static void do_picker_connect(GtkButton *src G_GNUC_UNUSED,
			      CapaCameraPicker *picker)
{
  CapaCamera *cam;
  cam = capa_picker_get_selected_camera(picker);
  fprintf(stderr, "picker connect %p %p\n", picker, cam);
  if (cam) {
    GValue val;
    memset(&val, 0, sizeof val);
    g_value_init(&val, G_TYPE_OBJECT);
    g_value_set_object(&val, cam);
    //g_signal_emit_by_name(picker, "picker-connect", &val);
    g_signal_emit_by_name(picker, "picker-connect", cam);
  }
}

static void do_camera_select(GtkTreeSelection *sel, CapaCameraPicker *picker)
{
  CapaCameraPickerPrivate *priv = picker->priv;
  GtkWidget *connect;
  GtkTreeIter iter;
  gboolean selected;

  fprintf(stderr, "selection changed\n");

  connect = glade_xml_get_widget(priv->glade, "picker-connect");

  selected = gtk_tree_selection_get_selected(sel, NULL, &iter);
  gtk_widget_set_sensitive(connect, selected);
}

static void capa_camera_picker_init(CapaCameraPicker *picker)
{
  CapaCameraPickerPrivate *priv;
  GtkWidget *list;
  GtkCellRenderer *model;
  GtkCellRenderer *port;
  GtkTreeViewColumn *modelCol;
  GtkTreeViewColumn *portCol;
  GtkTreeSelection *sel;
  GtkWidget *connect;

  priv = picker->priv = CAPA_CAMERA_PICKER_GET_PRIVATE(picker);

  priv->model = gtk_list_store_new(1, G_TYPE_OBJECT);
  priv->glade = glade_xml_new("capa.glade", "camera-picker", "capa");

  list = glade_xml_get_widget(priv->glade, "camera-list");

  glade_xml_signal_connect_data(priv->glade, "camera_picker_close", G_CALLBACK(do_picker_close), picker);
  glade_xml_signal_connect_data(priv->glade, "camera_picker_refresh", G_CALLBACK(do_picker_refresh), picker);
  glade_xml_signal_connect_data(priv->glade, "camera_picker_connect", G_CALLBACK(do_picker_connect), picker);
  glade_xml_signal_connect_data(priv->glade, "camera_picker_activate", G_CALLBACK(do_picker_activate), picker);

  modelCol = gtk_tree_view_column_new();
  portCol = gtk_tree_view_column_new();

  model = gtk_cell_renderer_text_new();
  port = gtk_cell_renderer_text_new();

  gtk_tree_view_append_column(GTK_TREE_VIEW(list), modelCol);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list), portCol);
  gtk_tree_view_column_pack_start(modelCol, model, TRUE);
  gtk_tree_view_column_pack_start(portCol, port, TRUE);
  gtk_tree_view_column_set_cell_data_func(modelCol, model, capa_camera_cell_data_model_func, NULL, NULL);
  gtk_tree_view_column_set_cell_data_func(portCol, port, capa_camera_cell_data_port_func, NULL, NULL);

  gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(priv->model));

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

  g_signal_connect(sel, "changed", G_CALLBACK(do_camera_select), picker);

  connect = glade_xml_get_widget(priv->glade, "picker-connect");
  gtk_widget_set_sensitive(connect, FALSE);
}

void capa_camera_picker_show(CapaCameraPicker *picker)
{
  CapaCameraPickerPrivate *priv = picker->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-picker");
  GtkWidget *list;
  GtkTreeSelection *sel;

  list = glade_xml_get_widget(priv->glade, "camera-list");
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

  //gtk_tree_selection_unselect_all(sel);

  fprintf(stderr, "Show\n");
  gtk_widget_show(win);
  gtk_window_present(GTK_WINDOW(win));
}

void capa_camera_picker_hide(CapaCameraPicker *picker)
{
  CapaCameraPickerPrivate *priv = picker->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-picker");

  gtk_widget_hide(win);
}

