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

#include "entangle-debug.h"
#include "entangle-image-display.h"
#include "entangle-image.h"

#define ENTANGLE_IMAGE_DISPLAY_GET_PRIVATE(obj)                             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_IMAGE_DISPLAY, EntangleImageDisplayPrivate))

struct _EntangleImageDisplayPrivate {
    gulong imageNotifyID;
    EntangleImage *image;

    GdkPixmap *pixmap;

    gboolean autoscale;
    float scale;
    gboolean infoHint;
};

G_DEFINE_TYPE(EntangleImageDisplay, entangle_image_display, GTK_TYPE_DRAWING_AREA);

enum {
    PROP_O,
    PROP_IMAGE,
    PROP_AUTOSCALE,
    PROP_SCALE,
    PROP_INFOHINT,
};

static void do_entangle_pixmap_setup(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;
    int pw, ph;
    GdkPixbuf *pixbuf = NULL;

    if (!GTK_WIDGET_REALIZED(display)) {
        ENTANGLE_DEBUG("Skipping setup for non-realized widget");
        return;
    }

    ENTANGLE_DEBUG("Setting up server pixmap");

    if (priv->pixmap) {
        g_object_unref(priv->pixmap);
        priv->pixmap = NULL;
    }

    if (priv->image)
        pixbuf = entangle_image_get_pixbuf(priv->image);
    if (!pixbuf)
        return;

    pw = gdk_pixbuf_get_width(pixbuf);
    ph = gdk_pixbuf_get_height(pixbuf);
    priv->pixmap = gdk_pixmap_new(GTK_WIDGET(display)->window,
                                  pw, ph, -1);
    gdk_draw_pixbuf(priv->pixmap, NULL, pixbuf,
                    0, 0, 0, 0, pw, ph,
                    GDK_RGB_DITHER_NORMAL, 0, 0);
}


static void entangle_image_display_update_hint(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;
    gchar *fileInfo = NULL;
    gchar *pixmapInfo = NULL;
    gchar *hint;
    const gchar *filename;
    GdkPixbuf *pixbuf;

    if (!priv->infoHint) {
        gtk_widget_set_tooltip_markup(GTK_WIDGET(display), "");
        return;
    }

    if (priv->image &&
        (filename = entangle_image_get_filename(priv->image))) {
        gchar *base = g_path_get_basename(filename);
        gchar *dir = g_path_get_dirname(filename);
        time_t lastMod = entangle_image_get_last_modified(priv->image);
        off_t size = entangle_image_get_file_size(priv->image);
        GTimeVal tv = { .tv_sec = lastMod, .tv_usec = 0 };
        gchar *datestr = g_time_val_to_iso8601(&tv);

        fileInfo = g_strdup_printf("<b>File:</b> %s\n"
                                   "<b>Session:</b> %s\n"
                                   "<b>Last modified:</b> %s\n"
                                   "<b>Size:</b> %d kb\n",
                                   base, dir,
                                   datestr, (int)size/1024);

        g_free(base);
        g_free(dir);
        g_free(datestr);
    }

    if (priv->image &&
        (pixbuf = entangle_image_get_pixbuf(priv->image))) {
        pixmapInfo = g_strdup_printf("<b>Width:</b> %d\n"
                                     "<b>Height:</b> %d\n",
                                     gdk_pixbuf_get_width(pixbuf),
                                     gdk_pixbuf_get_height(pixbuf));
    }


    if (fileInfo || pixmapInfo) {
        hint = g_strdup_printf("%s%s",
                               fileInfo ? fileInfo : "",
                               pixmapInfo ? pixmapInfo : "");

        gtk_widget_set_tooltip_markup(GTK_WIDGET(display), hint);

        g_free(fileInfo);
        g_free(pixmapInfo);
        g_free(hint);
    } else {
        gtk_widget_set_tooltip_markup(GTK_WIDGET(display), "");
    }
}


static void entangle_image_display_get_property(GObject *object,
                                                guint prop_id,
                                                GValue *value,
                                                GParamSpec *pspec)
{
    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(object);
    EntangleImageDisplayPrivate *priv = display->priv;

    switch (prop_id)
        {
        case PROP_IMAGE:
            g_value_set_object(value, priv->image);
            break;

        case PROP_AUTOSCALE:
            g_value_set_boolean(value, priv->autoscale);
            break;

        case PROP_SCALE:
            g_value_set_float(value, priv->scale);
            break;

        case PROP_INFOHINT:
            g_value_set_boolean(value, priv->infoHint);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_image_display_set_property(GObject *object,
                                                guint prop_id,
                                                const GValue *value,
                                                GParamSpec *pspec)
{
    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(object);
    EntangleImageDisplayPrivate *priv = display->priv;

    ENTANGLE_DEBUG("Set prop on image display %d", prop_id);

    switch (prop_id)
        {
        case PROP_IMAGE:
            entangle_image_display_set_image(display, g_value_get_object(value));
            break;

        case PROP_AUTOSCALE:
            entangle_image_display_set_autoscale(display, g_value_get_boolean(value));
            break;

        case PROP_SCALE:
            entangle_image_display_set_scale(display, g_value_get_float(value));
            break;

        case PROP_INFOHINT:
            priv->infoHint = g_value_get_boolean(value);
            entangle_image_display_update_hint(display);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_image_display_finalize (GObject *object)
{
    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(object);
    EntangleImageDisplayPrivate *priv = display->priv;

    if (priv->image) {
        g_signal_handler_disconnect(priv->image, priv->imageNotifyID);
        g_object_unref(priv->image);
    }

    if (priv->pixmap)
        g_object_unref(priv->pixmap);

    G_OBJECT_CLASS (entangle_image_display_parent_class)->finalize (object);
}

static void entangle_image_display_realize(GtkWidget *widget)
{
    GTK_WIDGET_CLASS(entangle_image_display_parent_class)->realize(widget);

    do_entangle_pixmap_setup(ENTANGLE_IMAGE_DISPLAY(widget));
}

static gboolean entangle_image_display_expose(GtkWidget *widget,
                                              GdkEventExpose *expose)
{
    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(widget);
    EntangleImageDisplayPrivate *priv = display->priv;
    int ww, wh; /* Available drawing area extents */
    int pw = 0, ph = 0; /* Original image size */
    double iw, ih; /* Desired image size */
    double mx = 0, my = 0;  /* Offset of image within available area */
    double sx = 1, sy = 1;  /* Amount to scale by */
    cairo_t *cr;

    gdk_drawable_get_size(widget->window, &ww, &wh);

    if (priv->pixmap)
        gdk_drawable_get_size(GDK_DRAWABLE(priv->pixmap), &pw, &ph);

    /* Decide what size we're going to draw the image */
    if (priv->autoscale) {
        double aspectWin, aspectImage;
        aspectWin = (double)ww / (double)wh;
        aspectImage = (double)pw / (double)ph;

        if (aspectWin > aspectImage) {
            /* Match drawn height to widget height, scale width preserving aspect */
            ih = (double)wh;
            iw = (double)ih * aspectImage;
        } else if (aspectWin < aspectImage) {
            /* Match drawn width to widget width, scale height preserving aspect */
            iw = (double)ww;
            ih = (double)iw / aspectImage;
        } else {
            /* Match drawn size to widget size */
            iw = (double)ww;
            ih = (double)wh;
        }
    } else {
        if (priv->scale > 0) {
            /* Scale image larger */
            iw = (double)pw * priv->scale;
            ih = (double)ph * priv->scale;
        } else {
            /* Use native image size */
            iw = (double)pw;
            ih = (double)ph;
        }
    }

    /* Calculate any offset within available area */
    mx = (ww - iw)/2;
    my = (wh - ih)/2;

    /* Calculate the scale factor for drawing */
    sx = iw / pw;
    sy = ih / ph;

    ENTANGLE_DEBUG("Got win %dx%d image %dx%d, autoscale=%d scale=%lf",
               ww, wh, pw, ph, priv->autoscale ? 1 : 0, priv->scale);
    ENTANGLE_DEBUG("Drawing image %lf,%lf at %lf %lf sclaed %lfx%lf", iw, ih, mx, my, sx, sy);


    cr = gdk_cairo_create(widget->window);
    cairo_rectangle(cr,
                    expose->area.x,
                    expose->area.y,
                    expose->area.width + 1,
                    expose->area.height + 1);
    cairo_clip(cr);

    /* We need to fill the background first */
    cairo_rectangle(cr, 0, 0, ww, wh);
    /* Next cut out the inner area where the pixmap
       will be drawn. This avoids 'flashing' since we're
       not double-buffering. Note we're using the undocumented
       behaviour of drawing the rectangle from right to left
       to cut out the whole */
    if (priv->pixmap)
        cairo_rectangle(cr,
                        mx + iw,
                        my,
                        -1 * iw,
                        ih);
    cairo_fill(cr);

    /* Draw the actual image */
    if (priv->pixmap) {
        cairo_scale(cr, sx, sy);
        gdk_cairo_set_source_pixmap(cr,
                                    priv->pixmap,
                                    mx/sx, my/sy);
        cairo_paint(cr);
    }

    cairo_destroy(cr);

    return TRUE;
}

static void entangle_image_display_size_request(GtkWidget *widget,
                                                GtkRequisition *requisition)
{
    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(widget);
    EntangleImageDisplayPrivate *priv = display->priv;
    GdkPixbuf *pixbuf = NULL;

    if (priv->image)
        pixbuf = entangle_image_get_pixbuf(priv->image);

    if (!pixbuf) {
        requisition->width = 100;
        requisition->height = 100;
        ENTANGLE_DEBUG("No image, size request 100,100");
        return;
    }

    if (priv->autoscale) {
        /* For best fit mode, we'll say 100x100 is the smallest we're happy
         * to draw to. Whatever the container allocates us beyond that will
         * be used filled with the image */
        requisition->width = 100;
        requisition->height = 100;
    } else {
        /* Start a 1-to-1 mode */
        requisition->width = gdk_pixbuf_get_width(pixbuf);
        requisition->height = gdk_pixbuf_get_height(pixbuf);
        if (priv->scale > 0) {
            /* Scaling mode */
            requisition->width = (int)((double)requisition->width * priv->scale);
            requisition->height = (int)((double)requisition->height * priv->scale);
        }
    }

    ENTANGLE_DEBUG("Size request %d %d", requisition->width, requisition->height);
}

static void entangle_image_display_class_init(EntangleImageDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->finalize = entangle_image_display_finalize;
    object_class->get_property = entangle_image_display_get_property;
    object_class->set_property = entangle_image_display_set_property;

    widget_class->realize = entangle_image_display_realize;
    widget_class->expose_event = entangle_image_display_expose;
    widget_class->size_request = entangle_image_display_size_request;

    g_object_class_install_property(object_class,
                                    PROP_IMAGE,
                                    g_param_spec_object("image",
                                                        "Image",
                                                        "Image",
                                                        ENTANGLE_TYPE_IMAGE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_AUTOSCALE,
                                    g_param_spec_boolean("autoscale",
                                                         "Automatic scaling",
                                                         "Automatically scale image to fit available area",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_SCALE,
                                    g_param_spec_float("scale",
                                                       "Scale image",
                                                       "Scale factor for image, 0-1 for zoom out, 1->32 for zoom in",
                                                       0.0,
                                                       32.0,
                                                       0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_INFOHINT,
                                    g_param_spec_boolean("infohint",
                                                         "Image info hint",
                                                         "Image information hint",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleImageDisplayPrivate));
}

EntangleImageDisplay *entangle_image_display_new(void)
{
    return ENTANGLE_IMAGE_DISPLAY(g_object_new(ENTANGLE_TYPE_IMAGE_DISPLAY, NULL));
}


static void entangle_image_display_init(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv;

    priv = display->priv = ENTANGLE_IMAGE_DISPLAY_GET_PRIVATE(display);
    memset(priv, 0, sizeof *priv);

    gtk_widget_set_double_buffered(GTK_WIDGET(display), FALSE);
    priv->autoscale = TRUE;
}


static void entangle_image_display_image_pixbuf_notify(GObject *image G_GNUC_UNUSED,
                                                       GParamSpec *pspec G_GNUC_UNUSED,
                                                       gpointer opaque)
{
    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(opaque);

    do_entangle_pixmap_setup(display);
    entangle_image_display_update_hint(display);
    gtk_widget_queue_resize(GTK_WIDGET(display));
    gtk_widget_queue_draw(GTK_WIDGET(display));
}


void entangle_image_display_set_image(EntangleImageDisplay *display,
                                      EntangleImage *image)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    if (priv->image) {
        g_signal_handler_disconnect(priv->image, priv->imageNotifyID);
        g_object_unref(priv->image);
    }
    priv->image = image;
    if (priv->image) {
        g_object_ref(priv->image);
        priv->imageNotifyID = g_signal_connect(priv->image,
                                               "notify::pixbuf",
                                               G_CALLBACK(entangle_image_display_image_pixbuf_notify),
                                               display);
    }

    do_entangle_pixmap_setup(display);
    entangle_image_display_update_hint(display);
    gtk_widget_queue_resize(GTK_WIDGET(display));
    gtk_widget_queue_draw(GTK_WIDGET(display));
}


EntangleImage *entangle_image_display_get_image(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->image;
}


void entangle_image_display_set_autoscale(EntangleImageDisplay *display,
                                          gboolean autoscale)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    priv->autoscale = autoscale;

    if (GTK_WIDGET_VISIBLE(display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gboolean entangle_image_display_get_autoscale(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->autoscale;
}


void entangle_image_display_set_scale(EntangleImageDisplay *display,
                                      gfloat scale)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    priv->scale = scale;

    if (GTK_WIDGET_VISIBLE(display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gfloat entangle_image_display_get_scale(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->scale;
}


void entangle_image_display_set_infohint(EntangleImageDisplay *display,
                                         gboolean hint)
{
    g_object_set(display, "infohint", hint, NULL);
}


gboolean entangle_image_display_get_infohint(EntangleImageDisplay *display)
{
    gboolean hint;
    g_object_get(display, "infohint", &hint, NULL);
    return hint;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
