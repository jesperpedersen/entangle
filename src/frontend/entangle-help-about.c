/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>

#include "entangle-debug.h"
#include "entangle-help-about.h"

#define ENTANGLE_HELP_ABOUT_GET_PRIVATE(obj)                                \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_HELP_ABOUT, EntangleHelpAboutPrivate))

struct _EntangleHelpAboutPrivate {
    GtkBuilder *builder;
};

G_DEFINE_TYPE(EntangleHelpAbout, entangle_help_about, G_TYPE_OBJECT);


static void entangle_help_about_finalize (GObject *object)
{
    EntangleHelpAbout *about = ENTANGLE_HELP_ABOUT(object);
    EntangleHelpAboutPrivate *priv = about->priv;

    g_object_unref(priv->builder);

    G_OBJECT_CLASS (entangle_help_about_parent_class)->finalize (object);
}

static void entangle_help_about_class_init(EntangleHelpAboutClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_help_about_finalize;

    g_type_class_add_private(klass, sizeof(EntangleHelpAboutPrivate));
}

EntangleHelpAbout *entangle_help_about_new(void)
{
    return ENTANGLE_HELP_ABOUT(g_object_new(ENTANGLE_TYPE_HELP_ABOUT, NULL));
}

static void do_about_response(GtkDialog *dialog G_GNUC_UNUSED,
                              gint response G_GNUC_UNUSED,
                              EntangleHelpAbout *about)
{
    EntangleHelpAboutPrivate *priv = about->priv;
    ENTANGLE_DEBUG("about response");
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "help-about"));

    gtk_widget_hide(win);
}

static gboolean do_about_delete(GtkWidget *src G_GNUC_UNUSED,
                                GdkEvent *ev G_GNUC_UNUSED,
                                EntangleHelpAbout *about)
{
    EntangleHelpAboutPrivate *priv = about->priv;
    ENTANGLE_DEBUG("about delete");
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "help-about"));

    gtk_widget_hide(win);
    return TRUE;
}

static void entangle_help_about_init(EntangleHelpAbout *about)
{
    EntangleHelpAboutPrivate *priv;
    GtkWidget *win;
    GdkPixbuf *buf;
    GError *error = NULL;

    priv = about->priv = ENTANGLE_HELP_ABOUT_GET_PRIVATE(about);

    priv->builder = gtk_builder_new();

    if (access("./entangle", R_OK) == 0)
        gtk_builder_add_from_file(priv->builder, "frontend/entangle-help-about.xml", &error);
    else
        gtk_builder_add_from_file(priv->builder, PKGDATADIR "/entangle-help-about.xml", &error);

    if (error)
        g_error("Couldn't load builder file: %s", error->message);

    gtk_builder_connect_signals(priv->builder, about);

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "help-about"));

    g_signal_connect(win, "delete-event", G_CALLBACK(do_about_delete), about);
    g_signal_connect(win, "response", G_CALLBACK(do_about_response), about);

    if (access("./entangle-256x256.png", R_OK) < 0)
        buf = gdk_pixbuf_new_from_file(PKGDATADIR "/entangle-256x256.png", NULL);
    else
        buf = gdk_pixbuf_new_from_file("./entangle-256x256.png", NULL);
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(win),buf);

    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(win), VERSION);
}


GtkWindow *entangle_help_about_get_window(EntangleHelpAbout *about)
{
    EntangleHelpAboutPrivate *priv = about->priv;

    return GTK_WINDOW(gtk_builder_get_object(priv->builder, "help-about"));
}


void entangle_help_about_show(EntangleHelpAbout *about)
{
    EntangleHelpAboutPrivate *priv = about->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "help-about"));

    gtk_widget_show(win);
    gtk_window_present(GTK_WINDOW(win));
}

void entangle_help_about_hide(EntangleHelpAbout *about)
{
    EntangleHelpAboutPrivate *priv = about->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "help-about"));

    gtk_widget_hide(win);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
