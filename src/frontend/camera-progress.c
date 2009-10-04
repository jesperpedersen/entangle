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
#include <unistd.h>

#include "internal.h"
#include "camera-progress.h"
#include "camera.h"
#include "progress.h"

#define CAPA_CAMERA_PROGRESS_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_PROGRESS, CapaCameraProgressPrivate))

struct _CapaCameraProgressPrivate {
  GladeXML *glade;

  gboolean cancelled;

  float target;
};

static void capa_camera_progress_interface_init (gpointer g_iface,
						 gpointer iface_data);

//G_DEFINE_TYPE(CapaCameraProgress, capa_camera_progress, G_TYPE_OBJECT);
G_DEFINE_TYPE_EXTENDED(CapaCameraProgress, capa_camera_progress, G_TYPE_OBJECT, 0,
		       G_IMPLEMENT_INTERFACE(CAPA_PROGRESS_TYPE, capa_camera_progress_interface_init));


static void capa_camera_progress_finalize (GObject *object)
{

  G_OBJECT_CLASS (capa_camera_progress_parent_class)->finalize (object);
}

static void capa_camera_progress_class_init(CapaCameraProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_camera_progress_finalize;

  g_type_class_add_private(klass, sizeof(CapaCameraProgressPrivate));
}

CapaCameraProgress *capa_camera_progress_new(void)
{
  return CAPA_CAMERA_PROGRESS(g_object_new(CAPA_TYPE_CAMERA_PROGRESS, NULL));
}

static gboolean do_progress_cancel(GtkButton *src,
				   CapaCameraProgress *progress)
{
  CapaCameraProgressPrivate *priv = progress->priv;
  GtkWidget *win;
  CAPA_DEBUG("progress cancel");

  priv->cancelled = TRUE;

  gtk_widget_set_sensitive(GTK_WIDGET(src), FALSE);
  win = glade_xml_get_widget(priv->glade, "camera-progress");
  gtk_window_set_title(GTK_WINDOW(win), "Cancelling operation");

  return TRUE;
}

static gboolean do_progress_delete(GtkWidget *src G_GNUC_UNUSED,
				   GdkEvent *ev G_GNUC_UNUSED,
				   CapaCameraProgress *progress)
{
  CapaCameraProgressPrivate *priv = progress->priv;
  GtkWidget *cancel;
  GtkWidget *win;
  CAPA_DEBUG("progress delete");

  priv->cancelled = TRUE;

  cancel = glade_xml_get_widget(priv->glade, "progress-cancel");
  gtk_widget_set_sensitive(GTK_WIDGET(cancel), FALSE);
  win = glade_xml_get_widget(priv->glade, "camera-progress");
  gtk_window_set_title(GTK_WINDOW(win), "Cancelling operation");

  return TRUE;
}

static void capa_camera_progress_init(CapaCameraProgress *progress)
{
  CapaCameraProgressPrivate *priv;

  priv = progress->priv = CAPA_CAMERA_PROGRESS_GET_PRIVATE(progress);

  if (access("./capa.glade", R_OK) == 0)
    priv->glade = glade_xml_new("capa.glade", "camera-progress", "capa");
  else
    priv->glade = glade_xml_new(PKGDATADIR "/capa.glade", "camera-progress", "capa");

  glade_xml_signal_connect_data(priv->glade, "camera_progress_cancel", G_CALLBACK(do_progress_cancel), progress);
  glade_xml_signal_connect_data(priv->glade, "camera_progress_delete", G_CALLBACK(do_progress_delete), progress);
}

void capa_camera_progress_show(CapaCameraProgress *progress,
			       const char *title)
{
  CapaCameraProgressPrivate *priv = progress->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-progress");
  GtkWidget *lbl;
  GtkWidget *mtr;
  GtkWidget *cancel;

  lbl = glade_xml_get_widget(priv->glade, "progress-label");
  mtr = glade_xml_get_widget(priv->glade, "progress-meter");

  gtk_window_set_title(GTK_WINDOW(win), title);
  gtk_label_set_text(GTK_LABEL(lbl), title);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

  priv->cancelled = FALSE;
  cancel = glade_xml_get_widget(priv->glade, "progress-cancel");
  gtk_widget_set_sensitive(GTK_WIDGET(cancel), TRUE);

  gtk_widget_show(win);
  gtk_window_present(GTK_WINDOW(win));
}

void capa_camera_progress_hide(CapaCameraProgress *progress)
{
  CapaCameraProgressPrivate *priv = progress->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-progress");

  priv->cancelled = FALSE;
  gtk_widget_hide(win);
}


static void do_capa_camera_progress_start(CapaProgress *iface, float target, const char *format, va_list args)
{
  CapaCameraProgress *prog = CAPA_CAMERA_PROGRESS(iface);
  CapaCameraProgressPrivate *priv = prog->priv;
  GtkWidget *lbl;
  GtkWidget *mtr;
  char *txt;

  gdk_threads_enter();

  priv->target = target;
  lbl = glade_xml_get_widget(priv->glade, "progress-label");
  mtr = glade_xml_get_widget(priv->glade, "progress-meter");

  txt = g_strdup_vprintf(format, args);

  gtk_label_set_text(GTK_LABEL(lbl), txt);
  g_free(txt);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

  gdk_threads_leave();
}

static void do_capa_camera_progress_update(CapaProgress *iface, float current)
{
  CapaCameraProgress *prog = CAPA_CAMERA_PROGRESS(iface);
  CapaCameraProgressPrivate *priv = prog->priv;
  GtkWidget *mtr;

  gdk_threads_enter();

  mtr = glade_xml_get_widget(priv->glade, "progress-meter");

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), current / priv->target);

  gdk_threads_leave();
}

static void do_capa_camera_progress_stop(CapaProgress *iface)
{
  CapaCameraProgress *prog = CAPA_CAMERA_PROGRESS(iface);
  CapaCameraProgressPrivate *priv = prog->priv;
  GtkWidget *mtr;

  gdk_threads_enter();

  mtr = glade_xml_get_widget(priv->glade, "progress-meter");

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 1);

  gdk_threads_leave();
}

static gboolean do_capa_camera_progress_cancelled(CapaProgress *iface)
{
  CapaCameraProgress *prog = CAPA_CAMERA_PROGRESS(iface);
  CapaCameraProgressPrivate *priv = prog->priv;

  CAPA_DEBUG("Cancel called");

  return priv->cancelled;
}

static void capa_camera_progress_interface_init (gpointer g_iface,
						 gpointer iface_data G_GNUC_UNUSED)
{
  CapaProgressInterface *iface = g_iface;
  iface->start = do_capa_camera_progress_start;
  iface->update = do_capa_camera_progress_update;
  iface->stop = do_capa_camera_progress_stop;
  iface->cancelled = do_capa_camera_progress_cancelled;
}

