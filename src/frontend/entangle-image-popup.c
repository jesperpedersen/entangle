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

#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <gdk/gdkkeysyms.h>

#include "entangle-debug.h"
#include "entangle-image-popup.h"
#include "entangle-image-display.h"
#include "entangle-window.h"

#define ENTANGLE_IMAGE_POPUP_GET_PRIVATE(obj)                           \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_IMAGE_POPUP, EntangleImagePopupPrivate))

gboolean do_popup_delete(GtkWidget *src,
                         GdkEvent *ev,
                         gpointer data);

struct _EntangleImagePopupPrivate {
    EntangleImage *image;
    EntangleImageDisplay *display;
    GtkBuilder *builder;
};

static void entangle_image_popup_window_interface_init(gpointer g_iface,
                                                       gpointer iface_data);

G_DEFINE_TYPE_EXTENDED(EntangleImagePopup, entangle_image_popup, GTK_TYPE_WINDOW, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_WINDOW, entangle_image_popup_window_interface_init));


enum {
    PROP_0,
    PROP_IMAGE,
};

static void entangle_image_popup_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    EntangleImagePopup *popup = ENTANGLE_IMAGE_POPUP(object);
    EntangleImagePopupPrivate *priv = popup->priv;

    switch (prop_id)
        {
        case PROP_IMAGE:
            g_value_set_object(value, priv->image);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_image_popup_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    EntangleImagePopup *popup = ENTANGLE_IMAGE_POPUP(object);
    EntangleImagePopupPrivate *priv = popup->priv;

    ENTANGLE_DEBUG("Set prop on image popup %d", prop_id);

    switch (prop_id)
        {
        case PROP_IMAGE: {
            if (priv->image)
                g_object_unref(priv->image);
            priv->image = g_value_dup_object(value);

            entangle_image_display_set_image(priv->display, priv->image);
        } break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_image_popup_finalize(GObject *object)
{
    EntangleImagePopup *popup = ENTANGLE_IMAGE_POPUP(object);
    EntangleImagePopupPrivate *priv = popup->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "image-popup"));

    ENTANGLE_DEBUG("Remove popup");

    g_object_unref(priv->builder);

    gtk_widget_destroy(win);

    if (priv->image)
        g_object_unref(priv->image);

    G_OBJECT_CLASS(entangle_image_popup_parent_class)->finalize(object);
}


static gboolean entangle_image_popup_button_press(GtkWidget *widget,
                                                  GdkEventButton *ev,
                                                  gpointer data G_GNUC_UNUSED)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_POPUP(widget), FALSE);

    EntangleImagePopup *popup = ENTANGLE_IMAGE_POPUP(widget);
    EntangleImagePopupPrivate *priv = popup->priv;
    GtkWidget *win;
    int w, h;

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "image-popup"));

    gtk_window_get_size(GTK_WINDOW(win), &w, &h);

    if (ev->button == 1) {
        gtk_window_begin_move_drag(GTK_WINDOW(win), ev->button, ev->x_root, ev->y_root, ev->time);
    } else if (ev->button == 2 || ev->button == 3) {
        GdkWindowEdge edge;
        if (ev->x > (w/2)) {
            if (ev->y > (h/2))
                edge = GDK_WINDOW_EDGE_SOUTH_EAST;
            else
                edge = GDK_WINDOW_EDGE_NORTH_EAST;
        } else {
            if (ev->y > (h/2))
                edge = GDK_WINDOW_EDGE_SOUTH_WEST;
            else
                edge = GDK_WINDOW_EDGE_NORTH_WEST;
        }
        gtk_window_begin_resize_drag(GTK_WINDOW(win), edge, ev->button, ev->x_root, ev->y_root, ev->time);
    } else {
        return FALSE;
    }

    return TRUE;
}

static gboolean entangle_image_popup_key_release(GtkWidget *widget,
                                                 GdkEventKey *ev,
                                                 gpointer data G_GNUC_UNUSED)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_POPUP(widget), FALSE);

    if (ev->keyval == GDK_KEY_Escape ||
        ev->keyval == GDK_KEY_KP_Enter ||
        ev->keyval == GDK_KEY_Return) {
        gtk_widget_hide(widget);
        return TRUE;
    }

    return FALSE;
}

static void do_entangle_image_popup_set_builder(EntangleWindow *window,
                                                GtkBuilder *builder);

static void entangle_image_popup_window_interface_init(gpointer g_iface,
                                                       gpointer iface_data G_GNUC_UNUSED)
{
    EntangleWindowInterface *iface = g_iface;
    iface->set_builder = do_entangle_image_popup_set_builder;
}

static void entangle_image_popup_class_init(EntangleImagePopupClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_image_popup_finalize;
    object_class->get_property = entangle_image_popup_get_property;
    object_class->set_property = entangle_image_popup_set_property;

    g_object_class_install_property(object_class,
                                    PROP_IMAGE,
                                    g_param_spec_object("image",
                                                        "Image",
                                                        "Image to be displayed",
                                                        ENTANGLE_TYPE_IMAGE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_signal_new("popup-close",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleImagePopupClass, popup_close),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);


    g_type_class_add_private(klass, sizeof(EntangleImagePopupPrivate));
}


EntangleImagePopup *entangle_image_popup_new(void)
{
    return ENTANGLE_IMAGE_POPUP(entangle_window_new(ENTANGLE_TYPE_IMAGE_POPUP,
                                                    GTK_TYPE_WINDOW,
                                                    "image-popup"));
}

gboolean do_popup_delete(GtkWidget *src,
                         GdkEvent *ev G_GNUC_UNUSED,
                         gpointer data G_GNUC_UNUSED)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_POPUP(src), FALSE);

    ENTANGLE_DEBUG("popup delete");

    gtk_widget_hide(src);
    return TRUE;
}


static void do_entangle_image_popup_set_builder(EntangleWindow *win,
                                                GtkBuilder *builder)
{
    EntangleImagePopup *popup = ENTANGLE_IMAGE_POPUP(win);
    EntangleImagePopupPrivate *priv = popup->priv;

    priv->builder = g_object_ref(builder);

    g_signal_connect(popup, "button-press-event",
                     G_CALLBACK(entangle_image_popup_button_press), NULL);
    g_signal_connect(popup, "key-release-event",
                     G_CALLBACK(entangle_image_popup_key_release), NULL);

    priv->display = entangle_image_display_new();
    gtk_container_add(GTK_CONTAINER(popup), GTK_WIDGET(priv->display));

    g_signal_connect(popup, "delete-event", G_CALLBACK(do_popup_delete), NULL);
}


static void entangle_image_popup_init(EntangleImagePopup *popup)
{
    popup->priv = ENTANGLE_IMAGE_POPUP_GET_PRIVATE(popup);
}


void entangle_image_popup_show(EntangleImagePopup *popup,
                               GtkWindow *parent,
                               int x, int y)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup));
    g_return_if_fail(GTK_IS_WINDOW(parent));

    EntangleImagePopupPrivate *priv = popup->priv;

    gtk_widget_realize(GTK_WIDGET(popup));

    gtk_window_set_transient_for(GTK_WINDOW(popup), parent);

    gtk_widget_show(GTK_WIDGET(popup));
    gtk_window_move(GTK_WINDOW(popup), x, y);
    gtk_widget_show(GTK_WIDGET(priv->display));
    gtk_window_present(GTK_WINDOW(popup));
}


void entangle_image_popup_move_to_monitor(EntangleImagePopup *popup, gint monitor)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup));

    EntangleImagePopupPrivate *priv = popup->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "image-popup"));
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(win));
    GdkRectangle r;

    gdk_screen_get_monitor_geometry(screen, monitor, &r);

    ENTANGLE_DEBUG("At %d,%d Size %d,%d", r.x, r.y, r.width, r.height);

    gtk_window_move(GTK_WINDOW(win), r.x, r.y);
    gtk_window_resize(GTK_WINDOW(win), r.width, r.height);
    gtk_window_fullscreen(GTK_WINDOW(win));
}


void entangle_image_popup_show_on_monitor(EntangleImagePopup *popup, gint monitor)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup));

    EntangleImagePopupPrivate *priv = popup->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "image-popup"));
    GdkCursor *null_cursor = gdk_cursor_new(GDK_BLANK_CURSOR);

    gtk_widget_realize(win);

    gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(win)),
                          null_cursor);
    g_object_unref(null_cursor);

    entangle_image_popup_move_to_monitor(popup, monitor);

    gtk_widget_show(win);
    gtk_widget_show(GTK_WIDGET(priv->display));
    gtk_window_present(GTK_WINDOW(win));
}


/**
 * entangle_image_popup_set_image:
 * @popup: (transfer none): the popup widget
 * @image: (transfer none)(allow-none): the image to display, or NULL
 *
 * Set the image to be displayed by the popup
 */
void entangle_image_popup_set_image(EntangleImagePopup *popup, EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup));
    g_return_if_fail(ENTANGLE_IS_IMAGE(image));

    g_object_set(popup, "image", image, NULL);
}


/**
 * entangle_image_popup_get_image:
 * @popup: (transfer none): the popup widget
 *
 * Retrieve the image that the popup is currently displaying
 *
 * Returns: (transfer none): the image displayed
 */
EntangleImage *entangle_image_popup_get_image(EntangleImagePopup *popup)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup), NULL);

    EntangleImagePopupPrivate *priv = popup->priv;
    return priv->image;
}


void entangle_image_popup_set_background(EntangleImagePopup *popup,
                                         const gchar *background)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup));

    EntangleImagePopupPrivate *priv = popup->priv;

    entangle_image_display_set_background(priv->display, background);
}

gchar *entangle_image_popup_get_background(EntangleImagePopup *popup)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup), NULL);

    EntangleImagePopupPrivate *priv = popup->priv;

    return entangle_image_display_get_background(priv->display);
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
