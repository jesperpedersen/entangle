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

#include <unistd.h>
#include <string.h>
#include <glade/glade.h>

#include <gdk/gdkkeysyms.h>

#include "entangle-debug.h"
#include "entangle-image-polaroid.h"
#include "entangle-image.h"
#include "entangle-image-display.h"
#include "entangle-image-loader.h"

#define ENTANGLE_IMAGE_POLAROID_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_IMAGE_POLAROID, EntangleImagePolaroidPrivate))

struct _EntangleImagePolaroidPrivate {
    EntangleImageLoader *imageLoader;
    EntangleImage *image;
    EntangleImageDisplay *display;
    GladeXML *glade;
};

G_DEFINE_TYPE(EntangleImagePolaroid, entangle_image_polaroid, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_IMAGE,
    PROP_IMAGE_LOADER,
};

static void entangle_image_polaroid_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    EntangleImagePolaroid *polaroid = ENTANGLE_IMAGE_POLAROID(object);
    EntangleImagePolaroidPrivate *priv = polaroid->priv;

    switch (prop_id)
        {
        case PROP_IMAGE:
            g_value_set_object(value, priv->image);
            break;

        case PROP_IMAGE_LOADER:
            g_value_set_object(value, priv->imageLoader);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_image_polaroid_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    EntangleImagePolaroid *polaroid = ENTANGLE_IMAGE_POLAROID(object);
    EntangleImagePolaroidPrivate *priv = polaroid->priv;

    ENTANGLE_DEBUG("Set prop on image polaroid %d", prop_id);

    switch (prop_id)
        {
        case PROP_IMAGE: {
            if (priv->image)
                g_object_unref(priv->image);
            priv->image = g_value_get_object(value);
            g_object_ref(priv->image);

            entangle_image_display_set_filename(priv->display,
                                            entangle_image_filename(priv->image));
        } break;

        case PROP_IMAGE_LOADER: {
            if (priv->imageLoader)
                g_object_unref(priv->imageLoader);
            priv->imageLoader = g_value_get_object(value);
            g_object_ref(priv->imageLoader);

            entangle_image_display_set_image_loader(priv->display,
                                                priv->imageLoader);
        } break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_image_polaroid_finalize (GObject *object)
{
    EntangleImagePolaroid *polaroid = ENTANGLE_IMAGE_POLAROID(object);
    EntangleImagePolaroidPrivate *priv = polaroid->priv;

    ENTANGLE_DEBUG("Remove polaroid");

    g_object_unref(priv->glade);

    if (priv->image)
        g_object_unref(priv->image);

    G_OBJECT_CLASS (entangle_image_polaroid_parent_class)->finalize (object);
}

static gboolean entangle_image_polaroid_button_press(GtkWidget *widget G_GNUC_UNUSED,
                                                 GdkEventButton *ev,
                                                 gpointer data)
{
    EntangleImagePolaroid *polaroid = ENTANGLE_IMAGE_POLAROID(data);
    EntangleImagePolaroidPrivate *priv = polaroid->priv;
    GtkWidget *win;
    int w, h;

    win = glade_xml_get_widget(priv->glade, "image-polaroid");

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

static gboolean entangle_image_polaroid_key_release(GtkWidget *widget G_GNUC_UNUSED,
                                                GdkEventKey *ev,
                                                gpointer data)
{
    EntangleImagePolaroid *polaroid = ENTANGLE_IMAGE_POLAROID(data);
    EntangleImagePolaroidPrivate *priv = polaroid->priv;
    GtkWidget *win;

    win = glade_xml_get_widget(priv->glade, "image-polaroid");

    if (ev->keyval == GDK_Escape ||
        ev->keyval == GDK_KP_Enter ||
        ev->keyval == GDK_Return) {
        entangle_image_polaroid_hide(polaroid);
        return TRUE;
    }

    return FALSE;
}

static void entangle_image_polaroid_class_init(EntangleImagePolaroidClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_image_polaroid_finalize;
    object_class->get_property = entangle_image_polaroid_get_property;
    object_class->set_property = entangle_image_polaroid_set_property;

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

    g_object_class_install_property(object_class,
                                    PROP_IMAGE_LOADER,
                                    g_param_spec_object("image-loader",
                                                        "Image loader",
                                                        "Image loader",
                                                        ENTANGLE_TYPE_IMAGE_LOADER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleImagePolaroidPrivate));
}

EntangleImagePolaroid *entangle_image_polaroid_new(void)
{
    return ENTANGLE_IMAGE_POLAROID(g_object_new(ENTANGLE_TYPE_IMAGE_POLAROID, NULL));
}

static gboolean do_polaroid_delete(GtkWidget *src G_GNUC_UNUSED,
                                   GdkEvent *ev G_GNUC_UNUSED,
                                   EntangleImagePolaroid *polaroid)
{
    EntangleImagePolaroidPrivate *priv = polaroid->priv;
    ENTANGLE_DEBUG("polaroid delete");
    GtkWidget *win = glade_xml_get_widget(priv->glade, "image-polaroid");

    gtk_widget_hide(win);
    return TRUE;
}

static void entangle_image_polaroid_init(EntangleImagePolaroid *polaroid)
{
    EntangleImagePolaroidPrivate *priv;
    GtkWidget *win;

    priv = polaroid->priv = ENTANGLE_IMAGE_POLAROID_GET_PRIVATE(polaroid);

    if (access("./entangle.glade", R_OK) == 0)
        priv->glade = glade_xml_new("entangle.glade", "image-polaroid", "entangle");
    else
        priv->glade = glade_xml_new(PKGDATADIR "/entangle.glade", "image-polaroid", "entangle");

    win = glade_xml_get_widget(priv->glade, "image-polaroid");

    g_signal_connect(win, "button-press-event",
                     G_CALLBACK(entangle_image_polaroid_button_press), polaroid);
    g_signal_connect(win, "key-release-event",
                     G_CALLBACK(entangle_image_polaroid_key_release), polaroid);

    priv->display = entangle_image_display_new();
    gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(priv->display));

    g_signal_connect(win, "delete-event", G_CALLBACK(do_polaroid_delete), polaroid);
}

void entangle_image_polaroid_show(EntangleImagePolaroid *polaroid,
                              GtkWindow *parent,
                              int x, int y)
{
    EntangleImagePolaroidPrivate *priv = polaroid->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "image-polaroid");

    win = glade_xml_get_widget(priv->glade, "image-polaroid");

    gtk_widget_realize(win);

    gtk_window_set_transient_for(GTK_WINDOW(win), parent);

    gtk_widget_show(win);
    gtk_window_move(GTK_WINDOW(win), x, y);
    gtk_widget_show(GTK_WIDGET(priv->display));
    gtk_window_present(GTK_WINDOW(win));
}

void entangle_image_polaroid_hide(EntangleImagePolaroid *polaroid)
{
    EntangleImagePolaroidPrivate *priv = polaroid->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "image-polaroid");

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
