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
#include <math.h>
#include <glade/glade.h>
#include <unistd.h>

#include "camera-manager.h"
#include "camera-list.h"
#include "camera-info.h"
#include "camera-progress.h"
#include "image-display.h"
#include "help-about.h"

#define CAPA_CAMERA_MANAGER_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerPrivate))

struct _CapaCameraManagerPrivate {
  CapaCamera *camera;
  CapaPreferences *prefs;

  CapaHelpAbout *about;

  CapaCameraInfo *summary;
  CapaCameraInfo *manual;
  CapaCameraInfo *driver;
  CapaCameraInfo *supported;

  CapaCameraProgress *progress;

  CapaImageDisplay *imageDisplay;
  //GtkImage *imageDisplay;

  int zoomLevel;

  GThread *captureThread;

  GladeXML *glade;
};

G_DEFINE_TYPE(CapaCameraManager, capa_camera_manager, G_TYPE_OBJECT);

enum {
  PROP_O,
  PROP_CAMERA,
  PROP_PREFERENCES,
};

static void do_capture_widget_sensitivity(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *toolCapture;
  GtkWidget *toolPreview;
  GtkWidget *settingsScroll;

  toolCapture = glade_xml_get_widget(priv->glade, "toolbar-capture");
  toolPreview = glade_xml_get_widget(priv->glade, "toolbar-preview");
  settingsScroll = glade_xml_get_widget(priv->glade, "settings-scroll");

  gtk_widget_set_sensitive(toolCapture,
			   priv->camera &&
			   capa_camera_has_capture(priv->camera) &&
			   priv->captureThread == NULL ? TRUE : FALSE);
  gtk_widget_set_sensitive(toolPreview,
			   priv->camera &&
			   capa_camera_has_preview(priv->camera) &&
			   priv->captureThread == NULL ? TRUE : FALSE);

  if (priv->camera && !capa_camera_has_capture(priv->camera))
    gtk_widget_set_tooltip_text(toolCapture, "This camera does not support image capture");
  if (priv->camera && !capa_camera_has_preview(priv->camera))
    gtk_widget_set_tooltip_text(toolPreview, "This camera does not support image preview");

  if (priv->camera && !capa_camera_has_settings(priv->camera))
    gtk_widget_hide(settingsScroll);
}


static void capa_camera_manager_get_property(GObject *object,
					     guint prop_id,
					     GValue *value,
					     GParamSpec *pspec)
{
  CapaCameraManager *manager = CAPA_CAMERA_MANAGER(object);
  CapaCameraManagerPrivate *priv = manager->priv;

  switch (prop_id)
    {
    case PROP_CAMERA:
      g_value_set_object(value, priv->camera);
      break;

    case PROP_PREFERENCES:
      g_value_set_object(value, priv->prefs);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_camera_manager_set_property(GObject *object,
					     guint prop_id,
					     const GValue *value,
					     GParamSpec *pspec)
{
  CapaCameraManager *manager = CAPA_CAMERA_MANAGER(object);
  CapaCameraManagerPrivate *priv = manager->priv;

  fprintf(stderr, "Set prop %d\n", prop_id);

  switch (prop_id)
    {
    case PROP_CAMERA: {
      char *title;
      GtkWidget *win;
      GValue prog;
      if (priv->camera) {
	g_object_set_property(G_OBJECT(priv->camera), "progress", NULL);
	g_object_unref(G_OBJECT(priv->camera));
      }
      priv->camera = g_value_get_object(value);
      g_object_ref(G_OBJECT(priv->camera));

      title = g_strdup_printf("%s Camera Manager - Capa",
			      capa_camera_model(priv->camera));

      win = glade_xml_get_widget(priv->glade, "camera-manager");
      gtk_window_set_title(GTK_WINDOW(win), title);
      g_free(title);

      memset(&prog, 0, sizeof prog);
      g_value_init(&prog, CAPA_TYPE_CAMERA_PROGRESS);
      g_value_set_object(&prog, priv->progress);
      fprintf(stderr, "setting\n");
      g_object_set_property(G_OBJECT(priv->camera), "progress", &prog);
      g_value_unset(&prog);
      fprintf(stderr, "done\n");

      do_capture_widget_sensitivity(manager);
    } break;

    case PROP_PREFERENCES:
      if (priv->prefs)
	g_object_unref(G_OBJECT(priv->prefs));
      priv->prefs = g_value_get_object(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_camera_manager_finalize (GObject *object)
{
  CapaCameraManager *manager = CAPA_CAMERA_MANAGER(object);
  CapaCameraManagerPrivate *priv = manager->priv;

  fprintf(stderr, "Finalize manager\n");

  if (priv->camera)
    g_object_unref(G_OBJECT(priv->camera));
  if (priv->prefs)
    g_object_unref(G_OBJECT(priv->prefs));

  g_object_unref(G_OBJECT(priv->progress));
  g_object_unref(priv->glade);

  G_OBJECT_CLASS (capa_camera_manager_parent_class)->finalize (object);
}


static void capa_camera_manager_class_init(CapaCameraManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_camera_manager_finalize;
  object_class->get_property = capa_camera_manager_get_property;
  object_class->set_property = capa_camera_manager_set_property;

  g_signal_new("manager-connect",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraManagerClass, manager_connect),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__VOID,
	       G_TYPE_NONE,
	       0);

  g_signal_new("manager-disconnect",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraManagerClass, manager_disconnect),
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
				  PROP_PREFERENCES,
				  g_param_spec_object("preferences",
						      "Preferences",
						      "Application preferences",
						      CAPA_TYPE_PREFERENCES,
						      G_PARAM_READWRITE |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaCameraManagerPrivate));
}


CapaCameraManager *capa_camera_manager_new(CapaPreferences *prefs)
{
  return CAPA_CAMERA_MANAGER(g_object_new(CAPA_TYPE_CAMERA_MANAGER,
					  "preferences", prefs,
					  NULL));
}

static void do_manager_help_summary(GtkMenuItem *src G_GNUC_UNUSED,
				    CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  if (!priv->summary) {
    priv->summary = capa_camera_info_new();
    g_object_set(G_OBJECT(priv->summary),
		 "data", CAPA_CAMERA_INFO_DATA_SUMMARY,
		 "camera", priv->camera,
		 NULL);
  }
  capa_camera_info_show(priv->summary);
}

static void do_manager_help_manual(GtkMenuItem *src G_GNUC_UNUSED,
				    CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  if (!priv->manual) {
    priv->manual = capa_camera_info_new();
    g_object_set(G_OBJECT(priv->manual),
		 "data", CAPA_CAMERA_INFO_DATA_MANUAL,
		 "camera", priv->camera,
		 NULL);
  }
  capa_camera_info_show(priv->manual);
}

static void do_manager_help_driver(GtkMenuItem *src G_GNUC_UNUSED,
				    CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  if (!priv->driver) {
    priv->driver = capa_camera_info_new();
    g_object_set(G_OBJECT(priv->driver),
		 "data", CAPA_CAMERA_INFO_DATA_DRIVER,
		 "camera", priv->camera,
		 NULL);
  }
  capa_camera_info_show(priv->driver);
}

static void capture_load_image(CapaCameraManager *manager,
			       const char *localpath)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GdkPixbuf *pixbuf;
  pixbuf = gdk_pixbuf_new_from_file(localpath, NULL);

  g_object_set(G_OBJECT(priv->imageDisplay),
	       "pixbuf", pixbuf,
	       NULL);
  g_object_unref(pixbuf);
}

static gpointer capture_thread(void *data)
{
  CapaCameraManager *manager = data;
  CapaCameraManagerPrivate *priv = manager->priv;
  char *localpath = g_strdup_printf("%s/%s",
				    capa_preferences_picture_dir(priv->prefs),
				    "capture.tiff"); /* XXX sequence*/

  fprintf(stderr, "starting Capture\n");
  gdk_threads_enter();
  capa_camera_progress_show(priv->progress, "Capturing image");
  gdk_threads_leave();
  capa_camera_capture(priv->camera, localpath);

  gdk_threads_enter();
  capa_camera_progress_hide(priv->progress);

  capture_load_image(manager, localpath);

  fprintf(stderr, "all done\n");

  priv->captureThread = NULL;
  do_capture_widget_sensitivity(manager);

  gdk_threads_leave();

  g_free(localpath);

  return NULL;
}

static gpointer preview_thread(void *data)
{
  CapaCameraManager *manager = data;
  CapaCameraManagerPrivate *priv = manager->priv;
  char *localpath = g_strdup_printf("%s/%s",
				    capa_preferences_picture_dir(priv->prefs),
				    "preview.tiff"); /* XXX sequence*/

  fprintf(stderr, "starting Preview\n");
  gdk_threads_enter();
  capa_camera_progress_show(priv->progress, "Capturing image");
  gdk_threads_leave();
  capa_camera_preview(priv->camera, localpath);

  gdk_threads_enter();
  capa_camera_progress_hide(priv->progress);

  capture_load_image(manager, localpath);

  fprintf(stderr, "all done\n");

  priv->captureThread = NULL;
  do_capture_widget_sensitivity(manager);

  gdk_threads_leave();

  unlink(localpath);
  g_free(localpath);

  return NULL;
}

static void do_toolbar_capture(GtkToolButton *src G_GNUC_UNUSED,
			       CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  fprintf(stderr, "starting Capture thread\n");

  priv->captureThread = g_thread_create(capture_thread, manager, FALSE, NULL);
  do_capture_widget_sensitivity(manager);
}

static void do_toolbar_preview(GtkToolButton *src G_GNUC_UNUSED,
			       CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  fprintf(stderr, "starting Preview thread\n");

  priv->captureThread = g_thread_create(preview_thread, manager, FALSE, NULL);
  do_capture_widget_sensitivity(manager);
}


static void do_zoom_widget_sensitivity(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GValue autoscale;
  GtkWidget *toolzoomnormal = glade_xml_get_widget(priv->glade, "toolbar-zoom-normal");
  GtkWidget *toolzoombest = glade_xml_get_widget(priv->glade, "toolbar-zoom-best");
  GtkWidget *toolzoomin = glade_xml_get_widget(priv->glade, "toolbar-zoom-in");
  GtkWidget *toolzoomout = glade_xml_get_widget(priv->glade, "toolbar-zoom-out");

  GtkWidget *menuzoomnormal = glade_xml_get_widget(priv->glade, "menu-zoom-normal");
  GtkWidget *menuzoombest = glade_xml_get_widget(priv->glade, "menu-zoom-best");
  GtkWidget *menuzoomin = glade_xml_get_widget(priv->glade, "menu-zoom-in");
  GtkWidget *menuzoomout = glade_xml_get_widget(priv->glade, "menu-zoom-out");


  memset(&autoscale, 0, sizeof autoscale);
  g_value_init(&autoscale, G_TYPE_BOOLEAN);
  g_object_get_property(G_OBJECT(priv->imageDisplay), "autoscale", &autoscale);

  if (g_value_get_boolean(&autoscale)) {
    gtk_widget_set_sensitive(toolzoombest, FALSE);
    gtk_widget_set_sensitive(toolzoomnormal, TRUE);
    gtk_widget_set_sensitive(toolzoomin, FALSE);
    gtk_widget_set_sensitive(toolzoomout, FALSE);

    gtk_widget_set_sensitive(menuzoombest, FALSE);
    gtk_widget_set_sensitive(menuzoomnormal, TRUE);
    gtk_widget_set_sensitive(menuzoomin, FALSE);
    gtk_widget_set_sensitive(menuzoomout, FALSE);
  } else {
    gtk_widget_set_sensitive(toolzoombest, TRUE);
    gtk_widget_set_sensitive(toolzoomnormal, priv->zoomLevel == 0 ? FALSE : TRUE);
    gtk_widget_set_sensitive(toolzoomin, priv->zoomLevel == 10 ? FALSE : TRUE);
    gtk_widget_set_sensitive(toolzoomout, priv->zoomLevel == -10 ? FALSE : TRUE);

    gtk_widget_set_sensitive(menuzoombest, TRUE);
    gtk_widget_set_sensitive(menuzoomnormal, priv->zoomLevel == 0 ? FALSE : TRUE);
    gtk_widget_set_sensitive(menuzoomin, priv->zoomLevel == 10 ? FALSE : TRUE);
    gtk_widget_set_sensitive(menuzoomout, priv->zoomLevel == -10 ? FALSE : TRUE);
  }
}

static void capa_camera_manager_zoom_in(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  if (priv->zoomLevel < 10)
    priv->zoomLevel += 1;
  if (priv->zoomLevel > 0)
    g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0+priv->zoomLevel, NULL);
  else if (priv->zoomLevel < 0)
    g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0/pow(1.5, -priv->zoomLevel), NULL);
  else
    g_object_set(G_OBJECT(priv->imageDisplay), "scale", 0.0, NULL);
  do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_out(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  if (priv->zoomLevel > -10)
    priv->zoomLevel -= 1;
  if (priv->zoomLevel > 0)
    g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0+priv->zoomLevel, NULL);
  else if (priv->zoomLevel < 0)
    g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0/pow(1.5, -priv->zoomLevel), NULL);
  else
    g_object_set(G_OBJECT(priv->imageDisplay), "scale", 0.0, NULL);
  do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_normal(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  priv->zoomLevel = 0;
  g_object_set(G_OBJECT(priv->imageDisplay), "autoscale", FALSE, NULL);
  g_object_set(G_OBJECT(priv->imageDisplay), "scale", 0.0, NULL);
  do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_best(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  priv->zoomLevel = 0;
  g_object_set(G_OBJECT(priv->imageDisplay), "autoscale", TRUE, NULL);
  do_zoom_widget_sensitivity(manager);
}

static void do_toolbar_zoom_in(GtkToolButton *src G_GNUC_UNUSED,
			       CapaCameraManager *manager)
{
  capa_camera_manager_zoom_in(manager);
}

static void do_toolbar_zoom_out(GtkToolButton *src G_GNUC_UNUSED,
				CapaCameraManager *manager)
{
  capa_camera_manager_zoom_out(manager);
}

static void do_toolbar_zoom_normal(GtkToolButton *src G_GNUC_UNUSED,
				   CapaCameraManager *manager)
{
  capa_camera_manager_zoom_normal(manager);
}

static void do_toolbar_zoom_best(GtkToolButton *src G_GNUC_UNUSED,
				 CapaCameraManager *manager)
{
  capa_camera_manager_zoom_best(manager);
}


static void do_menu_zoom_in(GtkImageMenuItem *src G_GNUC_UNUSED,
			    CapaCameraManager *manager)
{
  capa_camera_manager_zoom_in(manager);
}

static void do_menu_zoom_out(GtkImageMenuItem *src G_GNUC_UNUSED,
				CapaCameraManager *manager)
{
  capa_camera_manager_zoom_out(manager);
}

static void do_menu_zoom_normal(GtkImageMenuItem *src G_GNUC_UNUSED,
				   CapaCameraManager *manager)
{
  capa_camera_manager_zoom_normal(manager);
}

static void do_menu_zoom_best(GtkImageMenuItem *src G_GNUC_UNUSED,
				 CapaCameraManager *manager)
{
  capa_camera_manager_zoom_best(manager);
}


static void do_toolbar_fullscreen(GtkToggleToolButton *src,
				  CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
  GtkWidget *menu = glade_xml_get_widget(priv->glade, "menu-fullscreen");

  if (gtk_toggle_tool_button_get_active(src))
    gtk_window_fullscreen(GTK_WINDOW(win));
  else
    gtk_window_unfullscreen(GTK_WINDOW(win));

  if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) !=
      gtk_toggle_tool_button_get_active(src))
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu),
				   gtk_toggle_tool_button_get_active(src));
}

static void do_menu_fullscreen(GtkCheckMenuItem *src,
			       CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
  GtkWidget *tool = glade_xml_get_widget(priv->glade, "toolbar-fullscreen");

  if (gtk_check_menu_item_get_active(src))
    gtk_window_fullscreen(GTK_WINDOW(win));
  else
    gtk_window_unfullscreen(GTK_WINDOW(win));

  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(tool)) !=
      gtk_check_menu_item_get_active(src))
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool),
				      gtk_check_menu_item_get_active(src));
}


static void do_app_quit(GtkMenuItem *src G_GNUC_UNUSED,
			CapaCameraManager *manager G_GNUC_UNUSED)
{
  gtk_main_quit();
}


static void do_help_about(GtkMenuItem *src G_GNUC_UNUSED,
			  CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;

  if (!priv->about)
    priv->about = capa_help_about_new();

  capa_help_about_show(priv->about);
}


static void do_manager_connect(GtkMenuItem *src G_GNUC_UNUSED,
			       CapaCameraManager *manager)
{
  g_signal_emit_by_name(manager, "manager-connect", NULL);
}


static void do_manager_disconnect(GtkMenuItem *src G_GNUC_UNUSED,
				  CapaCameraManager *manager)
{
  g_signal_emit_by_name(manager, "manager-disconnect", NULL);
}


static gboolean do_manager_delete(GtkWidget *widget G_GNUC_UNUSED,
				  GdkEvent *event G_GNUC_UNUSED,
				  CapaCameraManager *manager)
{
  fprintf(stderr, "Got delete\n");
  g_signal_emit_by_name(manager, "manager-disconnect", NULL);
  return TRUE;
}



static void capa_camera_manager_init(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv;
  GtkWidget *viewport;
  GtkWidget *display;
  GtkWidget *imageScroll;
  GtkWidget *iconScroll;
  GtkWidget *win;

  priv = manager->priv = CAPA_CAMERA_MANAGER_GET_PRIVATE(manager);

  if (access("./capa.glade", R_OK) == 0)
    priv->glade = glade_xml_new("capa.glade", "camera-manager", "capa");
  else
    priv->glade = glade_xml_new(PKGDATADIR "/capa.glade", "camera-manager", "capa");

  glade_xml_signal_connect_data(priv->glade, "camera_menu_help_summary", G_CALLBACK(do_manager_help_summary), manager);
  glade_xml_signal_connect_data(priv->glade, "camera_menu_help_manual", G_CALLBACK(do_manager_help_manual), manager);
  glade_xml_signal_connect_data(priv->glade, "camera_menu_help_driver", G_CALLBACK(do_manager_help_driver), manager);

  glade_xml_signal_connect_data(priv->glade, "toolbar_capture_click", G_CALLBACK(do_toolbar_capture), manager);
  glade_xml_signal_connect_data(priv->glade, "toolbar_preview_click", G_CALLBACK(do_toolbar_preview), manager);

  glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_in_click", G_CALLBACK(do_toolbar_zoom_in), manager);
  glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_out_click", G_CALLBACK(do_toolbar_zoom_out), manager);
  glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_best_click", G_CALLBACK(do_toolbar_zoom_best), manager);
  glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_normal_click", G_CALLBACK(do_toolbar_zoom_normal), manager);

  glade_xml_signal_connect_data(priv->glade, "toolbar_fullscreen_toggle", G_CALLBACK(do_toolbar_fullscreen), manager);

  glade_xml_signal_connect_data(priv->glade, "menu_zoom_in_activate", G_CALLBACK(do_menu_zoom_in), manager);
  glade_xml_signal_connect_data(priv->glade, "menu_zoom_out_activate", G_CALLBACK(do_menu_zoom_out), manager);
  glade_xml_signal_connect_data(priv->glade, "menu_zoom_best_activate", G_CALLBACK(do_menu_zoom_best), manager);
  glade_xml_signal_connect_data(priv->glade, "menu_zoom_normal_activate", G_CALLBACK(do_menu_zoom_normal), manager);

  glade_xml_signal_connect_data(priv->glade, "menu_fullscreen_toggle", G_CALLBACK(do_menu_fullscreen), manager);

  glade_xml_signal_connect_data(priv->glade, "menu_connect_activate", G_CALLBACK(do_manager_connect), manager);
  glade_xml_signal_connect_data(priv->glade, "menu_disconnect_activate", G_CALLBACK(do_manager_disconnect), manager);
  glade_xml_signal_connect_data(priv->glade, "menu_quit_activate", G_CALLBACK(do_app_quit), manager);

  glade_xml_signal_connect_data(priv->glade, "menu_about_activate", G_CALLBACK(do_help_about), manager);

  win = glade_xml_get_widget(priv->glade, "camera-manager");
  g_signal_connect(win, "delete-event", G_CALLBACK(do_manager_delete), manager);

  priv->progress = capa_camera_progress_new();

  viewport = glade_xml_get_widget(priv->glade, "image-viewport");

  priv->imageDisplay = capa_image_display_new();

  imageScroll = glade_xml_get_widget(priv->glade, "image-scroll");
  iconScroll = glade_xml_get_widget(priv->glade, "icon-scroll");
  display = glade_xml_get_widget(priv->glade, "display-panel");

  gtk_container_child_set(GTK_CONTAINER(display), imageScroll, "resize", TRUE, NULL);
  gtk_container_child_set(GTK_CONTAINER(display), iconScroll, "resize", FALSE, NULL);

  gtk_widget_set_size_request(iconScroll, 100, 100);

  fprintf(stderr, "Adding %p to %p\n", priv->imageDisplay, viewport);
  gtk_container_add(GTK_CONTAINER(viewport), GTK_WIDGET(priv->imageDisplay));
  do_zoom_widget_sensitivity(manager);
  do_capture_widget_sensitivity(manager);
}

void capa_camera_manager_show(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

  gtk_widget_show(win);
  gtk_widget_show(GTK_WIDGET(priv->imageDisplay));
  gtk_window_present(GTK_WINDOW(win));
}

void capa_camera_manager_hide(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

  gtk_widget_hide(win);
}

gboolean capa_camera_manager_visible(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

  return GTK_WIDGET_FLAGS(win) & GTK_VISIBLE;
}
