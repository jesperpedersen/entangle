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
#include <unistd.h>
#include <string.h>
#include <glade/glade.h>

#include "internal.h"
#include "help-about.h"

#define CAPA_HELP_ABOUT_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_HELP_ABOUT, CapaHelpAboutPrivate))

struct _CapaHelpAboutPrivate {
  GladeXML *glade;
};

G_DEFINE_TYPE(CapaHelpAbout, capa_help_about, G_TYPE_OBJECT);


static void capa_help_about_finalize (GObject *object)
{
  CapaHelpAbout *about = CAPA_HELP_ABOUT(object);
  CapaHelpAboutPrivate *priv = about->priv;

  g_object_unref(priv->glade);

  G_OBJECT_CLASS (capa_help_about_parent_class)->finalize (object);
}

static void capa_help_about_class_init(CapaHelpAboutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_help_about_finalize;

  g_type_class_add_private(klass, sizeof(CapaHelpAboutPrivate));
}

CapaHelpAbout *capa_help_about_new(void)
{
  return CAPA_HELP_ABOUT(g_object_new(CAPA_TYPE_HELP_ABOUT, NULL));
}

static void do_about_response(GtkDialog *dialog G_GNUC_UNUSED,
			      gint response G_GNUC_UNUSED,
			      CapaHelpAbout *about)
{
  CapaHelpAboutPrivate *priv = about->priv;
  CAPA_DEBUG("about response");
  GtkWidget *win = glade_xml_get_widget(priv->glade, "help-about");

  gtk_widget_hide(win);
}

static gboolean do_about_delete(GtkWidget *src G_GNUC_UNUSED,
				GdkEvent *ev G_GNUC_UNUSED,
				CapaHelpAbout *about)
{
  CapaHelpAboutPrivate *priv = about->priv;
  CAPA_DEBUG("about delete");
  GtkWidget *win = glade_xml_get_widget(priv->glade, "help-about");

  gtk_widget_hide(win);
  return TRUE;
}

static void capa_help_about_init(CapaHelpAbout *about)
{
  CapaHelpAboutPrivate *priv;
  GtkWidget *win;
  GdkPixbuf *buf;

  priv = about->priv = CAPA_HELP_ABOUT_GET_PRIVATE(about);

  if (access("./capa.glade", R_OK) == 0)
    priv->glade = glade_xml_new("capa.glade", "help-about", "capa");
  else
    priv->glade = glade_xml_new(PKGDATADIR "/capa.glade", "help-about", "capa");

  win = glade_xml_get_widget(priv->glade, "help-about");

  g_signal_connect(G_OBJECT(win), "delete-event", G_CALLBACK(do_about_delete), about);
  g_signal_connect(G_OBJECT(win), "response", G_CALLBACK(do_about_response), about);

  buf = gdk_pixbuf_new_from_file(PKGDATADIR "/" PACKAGE ".svg", NULL);
  gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(win),buf);

  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(win), VERSION);
}

void capa_help_about_show(CapaHelpAbout *about)
{
  CapaHelpAboutPrivate *priv = about->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "help-about");

  gtk_widget_show(win);
  gtk_window_present(GTK_WINDOW(win));
}

void capa_help_about_hide(CapaHelpAbout *about)
{
  CapaHelpAboutPrivate *priv = about->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "help-about");

  gtk_widget_hide(win);
}

