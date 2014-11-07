/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2015 Daniel P. Berrange
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
#include <glib/gi18n.h>

#include "entangle-debug.h"
#include "entangle-help-about.h"
#include "entangle-window.h"


#define ENTANGLE_HELP_ABOUT_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_HELP_ABOUT, EntangleHelpAboutPrivate))

gboolean do_about_delete(GtkWidget *src,
                         GdkEvent *ev,
                         gpointer data);

struct _EntangleHelpAboutPrivate {
    GtkBuilder *builder;
};

static void entangle_help_about_window_interface_init(gpointer g_iface,
                                                      gpointer iface_data);

G_DEFINE_TYPE_EXTENDED(EntangleHelpAbout, entangle_help_about, GTK_TYPE_ABOUT_DIALOG, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_WINDOW, entangle_help_about_window_interface_init));


static void entangle_help_about_finalize(GObject *object)
{
    EntangleHelpAbout *about = ENTANGLE_HELP_ABOUT(object);
    EntangleHelpAboutPrivate *priv = about->priv;

    g_object_unref(priv->builder);

    G_OBJECT_CLASS(entangle_help_about_parent_class)->finalize(object);
}

static void do_entangle_help_about_set_builder(EntangleWindow *window,
                                               GtkBuilder *builder);
static GtkBuilder *do_entangle_help_about_get_builder(EntangleWindow *window);


static void entangle_help_about_window_interface_init(gpointer g_iface,
                                                      gpointer iface_data G_GNUC_UNUSED)
{
    EntangleWindowInterface *iface = g_iface;
    iface->set_builder = do_entangle_help_about_set_builder;
    iface->get_builder = do_entangle_help_about_get_builder;
}


static void entangle_help_about_class_init(EntangleHelpAboutClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_help_about_finalize;

    g_type_class_add_private(klass, sizeof(EntangleHelpAboutPrivate));
}


EntangleHelpAbout *entangle_help_about_new(void)
{
    return ENTANGLE_HELP_ABOUT(entangle_window_new(ENTANGLE_TYPE_HELP_ABOUT,
                                                   GTK_TYPE_ABOUT_DIALOG,
                                                   "help-about"));
}


static void do_about_response(GtkDialog *dialog,
                              gint response G_GNUC_UNUSED,
                              gpointer data G_GNUC_UNUSED)
{
    g_return_if_fail(ENTANGLE_IS_HELP_ABOUT(dialog));

    ENTANGLE_DEBUG("about response");

    gtk_widget_hide(GTK_WIDGET(dialog));
}


gboolean do_about_delete(GtkWidget *src,
                         GdkEvent *ev G_GNUC_UNUSED,
                         gpointer data G_GNUC_UNUSED)
{
    g_return_val_if_fail(ENTANGLE_IS_HELP_ABOUT(src), FALSE);

    ENTANGLE_DEBUG("about delete");

    gtk_widget_hide(src);
    return TRUE;
}

static void do_entangle_help_about_set_builder(EntangleWindow *win,
                                               GtkBuilder *builder)
{
    EntangleHelpAbout *about = ENTANGLE_HELP_ABOUT(win);
    EntangleHelpAboutPrivate *priv = about->priv;

    priv->builder = g_object_ref(builder);
}


static GtkBuilder *do_entangle_help_about_get_builder(EntangleWindow *window)
{
    EntangleHelpAbout *about = ENTANGLE_HELP_ABOUT(window);
    EntangleHelpAboutPrivate *priv = about->priv;

    return priv->builder;
}


static void entangle_help_about_init(EntangleHelpAbout *about)
{
    about->priv = ENTANGLE_HELP_ABOUT_GET_PRIVATE(about);

    g_signal_connect(about, "response", G_CALLBACK(do_about_response), about);

    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
