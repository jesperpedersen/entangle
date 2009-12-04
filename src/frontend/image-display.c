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

#include <string.h>

#include "internal.h"
#include "image-display.h"
#include "image-loader.h"

#define CAPA_IMAGE_DISPLAY_GET_PRIVATE(obj)                             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_IMAGE_DISPLAY, CapaImageDisplayPrivate))

struct _CapaImageDisplayPrivate {
    CapaImageLoader *imageLoader;
    gulong imageLoaderNotifyID;
    char *filename;
    GdkPixbuf *pixbuf;

    GdkPixmap *pixmap;

    gboolean autoscale;
    float scale;
};

G_DEFINE_TYPE(CapaImageDisplay, capa_image_display, GTK_TYPE_DRAWING_AREA);

enum {
    PROP_O,
    PROP_IMAGE_LOADER,
    PROP_FILENAME,
    PROP_PIXBUF,
    PROP_AUTOSCALE,
    PROP_SCALE,
};

static void do_capa_pixmap_setup(CapaImageDisplay *display)
{
    CapaImageDisplayPrivate *priv = display->priv;
    int pw, ph;

    if (!GTK_WIDGET_REALIZED(display)) {
        CAPA_DEBUG("Skipping setup for non-realized widget");
        return;
    }

    CAPA_DEBUG("Setting up server pixmap");

    if (priv->pixmap) {
        g_object_unref(priv->pixmap);
        priv->pixmap = NULL;
    }
    if (!priv->pixbuf)
        return;

    pw = gdk_pixbuf_get_width(priv->pixbuf);
    ph = gdk_pixbuf_get_height(priv->pixbuf);
    priv->pixmap = gdk_pixmap_new(GTK_WIDGET(display)->window,
                                  pw, ph, -1);
    gdk_draw_pixbuf(priv->pixmap, NULL, priv->pixbuf,
                    0, 0, 0, 0, pw, ph,
                    GDK_RGB_DITHER_NORMAL, 0, 0);
}


static void capa_image_display_get_property(GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
    CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(object);
    CapaImageDisplayPrivate *priv = display->priv;

    switch (prop_id)
        {
        case PROP_IMAGE_LOADER:
            g_value_set_object(value, priv->imageLoader);
            break;

        case PROP_FILENAME:
            g_value_set_string(value, priv->filename);
            break;

        case PROP_PIXBUF:
            g_value_set_object(value, priv->pixbuf);
            break;

        case PROP_AUTOSCALE:
            g_value_set_boolean(value, priv->autoscale);
            break;

        case PROP_SCALE:
            g_value_set_float(value, priv->scale);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_image_display_image_loaded(CapaPixbufLoader *loader,
                                            const char *filename,
                                            gpointer opaque)
{
    CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(opaque);
    CapaImageDisplayPrivate *priv = display->priv;

    if (strcmp(filename, priv->filename) != 0)
        return;

    GdkPixbuf *pixbuf = capa_pixbuf_loader_get_pixbuf(loader, filename);
    g_object_set(G_OBJECT(display), "pixbuf", pixbuf, NULL);
}

static void capa_image_display_set_property(GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
    CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(object);
    CapaImageDisplayPrivate *priv = display->priv;

    CAPA_DEBUG("Set prop on image display %d", prop_id);

    switch (prop_id)
        {
        case PROP_IMAGE_LOADER:
            if (priv->imageLoader) {
                if (priv->filename)
                    capa_pixbuf_loader_unload(CAPA_PIXBUF_LOADER(priv->imageLoader), priv->filename);
                g_signal_handler_disconnect(G_OBJECT(priv->imageLoader), priv->imageLoaderNotifyID);
                g_object_unref(G_OBJECT(priv->imageLoader));
            }
            priv->imageLoader = g_value_get_object(value);
            if (priv->imageLoader) {
                g_object_ref(G_OBJECT(priv->imageLoader));
                priv->imageLoaderNotifyID = g_signal_connect(G_OBJECT(priv->imageLoader),
                                                        "pixbuf-loaded",
                                                        G_CALLBACK(capa_image_display_image_loaded),
                                                        object);
                if (priv->filename)
                    capa_pixbuf_loader_load(CAPA_PIXBUF_LOADER(priv->imageLoader), priv->filename);
            }
            break;

        case PROP_FILENAME:
            if (priv->filename) {
                if (priv->imageLoader)
                    capa_pixbuf_loader_unload(CAPA_PIXBUF_LOADER(priv->imageLoader), priv->filename);
                g_free(priv->filename);
            }
            priv->filename = g_value_dup_string(value);
            if (priv->imageLoader && priv->filename)
                capa_pixbuf_loader_load(CAPA_PIXBUF_LOADER(priv->imageLoader), priv->filename);
            break;

        case PROP_PIXBUF:
            if (priv->pixbuf)
                g_object_unref(G_OBJECT(priv->pixbuf));
            priv->pixbuf = g_value_get_object(value);
            if (priv->pixbuf)
                g_object_ref(G_OBJECT(priv->pixbuf));

            do_capa_pixmap_setup(display);

            if (GTK_WIDGET_VISIBLE(object))
                gtk_widget_queue_resize(GTK_WIDGET(object));
            break;

        case PROP_AUTOSCALE:
            priv->autoscale = g_value_get_boolean(value);
            if (GTK_WIDGET_VISIBLE(object))
                gtk_widget_queue_resize(GTK_WIDGET(object));
            break;

        case PROP_SCALE:
            priv->scale = g_value_get_float(value);
            if (GTK_WIDGET_VISIBLE(object))
                gtk_widget_queue_resize(GTK_WIDGET(object));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_image_display_finalize (GObject *object)
{
    CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(object);
    CapaImageDisplayPrivate *priv = display->priv;

    if (priv->filename && priv->imageLoader)
        capa_pixbuf_loader_unload(CAPA_PIXBUF_LOADER(priv->imageLoader), priv->filename);
    if (priv->imageLoader)
        g_object_unref(G_OBJECT(priv->imageLoader));
    if (priv->pixbuf)
        g_object_unref(G_OBJECT(priv->pixbuf));
    if (priv->pixmap)
        g_object_unref(G_OBJECT(priv->pixmap));

    G_OBJECT_CLASS (capa_image_display_parent_class)->finalize (object);
}

static void capa_image_display_realize(GtkWidget *widget)
{
    GTK_WIDGET_CLASS(capa_image_display_parent_class)->realize(widget);

    do_capa_pixmap_setup(CAPA_IMAGE_DISPLAY(widget));
}

static gboolean capa_image_display_expose(GtkWidget *widget,
                                          GdkEventExpose *expose)
{
    CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(widget);
    CapaImageDisplayPrivate *priv = display->priv;
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

    CAPA_DEBUG("Got win %dx%d image %dx%d, autoscale=%d scale=%lf",
               ww, wh, pw, ph, priv->autoscale ? 1 : 0, priv->scale);
    CAPA_DEBUG("Drawing image %lf,%lf at %lf %lf sclaed %lfx%lf", iw, ih, mx, my, sx, sy);


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

static void capa_image_display_size_request(GtkWidget *widget,
                                            GtkRequisition *requisition)
{
    CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(widget);
    CapaImageDisplayPrivate *priv = display->priv;

    if (!priv->pixbuf) {
        requisition->width = 100;
        requisition->height = 100;
        CAPA_DEBUG("No image, size request 100,100");
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
        requisition->width = gdk_pixbuf_get_width(priv->pixbuf);
        requisition->height = gdk_pixbuf_get_height(priv->pixbuf);
        if (priv->scale > 0) {
            /* Scaling mode */
            requisition->width = (int)((double)requisition->width * priv->scale);
            requisition->height = (int)((double)requisition->height * priv->scale);
        }
    }

    CAPA_DEBUG("Size request %d %d", requisition->width, requisition->height);
}

static void capa_image_display_class_init(CapaImageDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->finalize = capa_image_display_finalize;
    object_class->get_property = capa_image_display_get_property;
    object_class->set_property = capa_image_display_set_property;

    widget_class->realize = capa_image_display_realize;
    widget_class->expose_event = capa_image_display_expose;
    widget_class->size_request = capa_image_display_size_request;

    g_object_class_install_property(object_class,
                                    PROP_IMAGE_LOADER,
                                    g_param_spec_object("image-loader",
                                                        "Image loader",
                                                        "Image loader",
                                                        CAPA_TYPE_IMAGE_LOADER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_FILENAME,
                                    g_param_spec_string("filename",
                                                        "Filename",
                                                        "Image filename",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_PIXBUF,
                                    g_param_spec_object("pixbuf",
                                                        "Pixbuf",
                                                        "Pixbuf for the image to be displayed",
                                                        GDK_TYPE_PIXBUF,
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

    g_type_class_add_private(klass, sizeof(CapaImageDisplayPrivate));
}

CapaImageDisplay *capa_image_display_new(void)
{
    return CAPA_IMAGE_DISPLAY(g_object_new(CAPA_TYPE_IMAGE_DISPLAY, NULL));
}


static void capa_image_display_init(CapaImageDisplay *display)
{
    CapaImageDisplayPrivate *priv;

    priv = display->priv = CAPA_IMAGE_DISPLAY_GET_PRIVATE(display);
    memset(priv, 0, sizeof *priv);

    gtk_widget_set_double_buffered(GTK_WIDGET(display), FALSE);
    priv->autoscale = TRUE;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
