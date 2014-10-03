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
#include <math.h>

#include "entangle-debug.h"
#include "entangle-image-histogram.h"
#include "entangle-image.h"

#define ENTANGLE_IMAGE_HISTOGRAM_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_IMAGE_HISTOGRAM, EntangleImageHistogramPrivate))

struct _EntangleImageHistogramPrivate {
    double freq_red[256];
    double freq_green[256];
    double freq_blue[256];
    gboolean hasFreq;
    gboolean linear;
    gulong imageNotifyID;
    EntangleImage *image;
};

G_DEFINE_TYPE(EntangleImageHistogram, entangle_image_histogram, GTK_TYPE_DRAWING_AREA);

enum {
    PROP_O,
    PROP_IMAGE,
};

#define DOUBLE_EQUAL(a, b)                      \
    (fabs((a) - (b)) < 0.005)

static void do_entangle_pixmap_setup(EntangleImageHistogram *histogram)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(histogram));

    EntangleImageHistogramPrivate *priv = histogram->priv;
    GdkPixbuf *pixbuf = NULL;

    if (priv->image)
        pixbuf = entangle_image_get_pixbuf(priv->image);

    memset(priv->freq_red, 0, sizeof(priv->freq_red));
    memset(priv->freq_green, 0, sizeof(priv->freq_green));
    memset(priv->freq_blue, 0, sizeof(priv->freq_blue));

    if (!pixbuf) {
        priv->hasFreq = FALSE;
        return;
    }

    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    guint w = gdk_pixbuf_get_width(pixbuf);
    guint h = gdk_pixbuf_get_height(pixbuf);
    guint stride = gdk_pixbuf_get_rowstride(pixbuf);
    int x, y;

    for (y = 0; y < h; y++) {
        guchar *pixel = pixels;
        for (x = 0; x < w; x++) {
            guchar level_red = pixel[0];
            guchar level_green = pixel[1];
            guchar level_blue = pixel[2];

            priv->freq_red[level_red]++;
            priv->freq_green[level_green]++;
            priv->freq_blue[level_blue]++;

            pixel+=3;
        }

        pixels += stride;
    }

    priv->hasFreq = TRUE;
}


static void entangle_image_histogram_get_property(GObject *object,
                                                  guint prop_id,
                                                  GValue *value,
                                                  GParamSpec *pspec)
{
    EntangleImageHistogram *histogram = ENTANGLE_IMAGE_HISTOGRAM(object);
    EntangleImageHistogramPrivate *priv = histogram->priv;

    switch (prop_id)
        {
        case PROP_IMAGE:
            g_value_set_object(value, priv->image);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_image_histogram_set_property(GObject *object,
                                                  guint prop_id,
                                                  const GValue *value,
                                                  GParamSpec *pspec)
{
    EntangleImageHistogram *histogram = ENTANGLE_IMAGE_HISTOGRAM(object);

    ENTANGLE_DEBUG("Set prop on image histogram %d", prop_id);

    switch (prop_id)
        {
        case PROP_IMAGE:
            entangle_image_histogram_set_image(histogram, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_image_histogram_finalize(GObject *object)
{
    EntangleImageHistogram *histogram = ENTANGLE_IMAGE_HISTOGRAM(object);
    EntangleImageHistogramPrivate *priv = histogram->priv;

    if (priv->image) {
        g_signal_handler_disconnect(priv->image, priv->imageNotifyID);
        g_object_unref(priv->image);
    }

    G_OBJECT_CLASS(entangle_image_histogram_parent_class)->finalize(object);
}


static void entangle_image_histogram_draw_line(cairo_t *cr, float from_x, float from_y, float to_x, float to_y)
{
    cairo_move_to(cr, from_x, from_y);
    cairo_line_to(cr, to_x,  to_y);
}


static void entangle_image_histogram_draw_grid(cairo_t *cr, const float width, const float height)
{
    for (int k = 1; k < 4; k++) {
        entangle_image_histogram_draw_line(cr, k/(float)4 * width, 0, k/(float)4 * width, height);
        cairo_stroke(cr);
        entangle_image_histogram_draw_line(cr, 0, k/(float)4 * height, width, k/(float)4 * height);
        cairo_stroke(cr);
    }
}


static double entangle_image_histogram_calculate_value(double d, gboolean l)
{
    if (!l) {
        return DOUBLE_EQUAL(d, 0.0) ? 0.0 : log(d);
    }

    return d;
}


static gboolean entangle_image_histogram_draw(GtkWidget *widget, cairo_t *cr)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(widget), FALSE);

    EntangleImageHistogram *histogram = ENTANGLE_IMAGE_HISTOGRAM(widget);
    EntangleImageHistogramPrivate *priv = histogram->priv;
    int ww, wh; /* Available drawing area extents */
    double peak = 0.0;
    int idx;

    ww = gdk_window_get_width(gtk_widget_get_window(widget));
    wh = gdk_window_get_height(gtk_widget_get_window(widget));

    cairo_save(cr);

    /* We need to fill the background first */
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_rectangle(cr, 0, 0, ww, wh);
    cairo_fill(cr);

    /* Draw a grid */
    cairo_save(cr);
    cairo_set_line_width(cr, 0.4);
    cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.2);
    entangle_image_histogram_draw_grid(cr, ww, wh);
    cairo_restore(cr);

    if (priv->hasFreq) {
        for (idx = 0; idx < 255; idx++) {
            double rv = entangle_image_histogram_calculate_value(priv->freq_red[idx], priv->linear);
            double gv = entangle_image_histogram_calculate_value(priv->freq_green[idx], priv->linear);
            double bv = entangle_image_histogram_calculate_value(priv->freq_blue[idx], priv->linear);

            if (rv > peak)
                peak = rv;
            if (gv > peak)
                peak = gv;
            if (bv > peak)
                peak = bv;
        }
        cairo_set_line_width(cr, 3);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
        /* Draw the actual histogram */

        /* Red channel */
        cairo_save(cr);
        cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.7);
        cairo_move_to(cr, 0, wh);

        for (idx = 0; idx < 255; idx++) {
            double rv = entangle_image_histogram_calculate_value(priv->freq_red[idx], priv->linear);
            double x = (double)ww * (double)idx / 255.0;
            double y = (double)(wh - 2) * (double)rv / peak;

            cairo_line_to(cr, x, wh - y);
        }
        cairo_line_to(cr, ww, wh);
        cairo_line_to(cr, 0, wh);
        cairo_fill(cr);
        cairo_restore(cr);

        /* Green channel */
        cairo_save(cr);
        cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.7);
        cairo_move_to(cr, 0, wh);

        for (idx = 0; idx < 255; idx++) {
            double gv = entangle_image_histogram_calculate_value(priv->freq_green[idx], priv->linear);
            double x = (double)ww * (double)idx / 255.0;
            double y = (double)(wh - 2) * (double)gv / peak;

            cairo_line_to(cr, x, wh - y);
        }
        cairo_line_to(cr, ww, wh);
        cairo_line_to(cr, 0, wh);
        cairo_fill(cr);
        cairo_restore(cr);

        /* Blue channel */
        cairo_save(cr);
        cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.7);
        cairo_move_to(cr, 0, wh);

        for (idx = 0; idx < 255; idx++) {
            double bv = entangle_image_histogram_calculate_value(priv->freq_blue[idx], priv->linear);
            double x = (double)ww * (double)idx / 255.0;
            double y = (double)(wh - 2) * (double)bv / peak;

            cairo_line_to(cr, x, wh - y);
        }
        cairo_line_to(cr, ww, wh);
        cairo_line_to(cr, 0, wh);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    cairo_restore(cr);

    return TRUE;
}


static void entangle_image_histogram_get_preferred_width(GtkWidget *widget,
                                                         gint *minwidth,
                                                         gint *natwidth)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(widget));

    *minwidth = 100;
    *natwidth = 250;
}


static void entangle_image_histogram_get_preferred_height(GtkWidget *widget,
                                                          gint *minheight,
                                                          gint *natheight)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(widget));

    *minheight = 170;
    *natheight = 170;
}


static void entangle_image_histogram_class_init(EntangleImageHistogramClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->finalize = entangle_image_histogram_finalize;
    object_class->get_property = entangle_image_histogram_get_property;
    object_class->set_property = entangle_image_histogram_set_property;

    widget_class->draw = entangle_image_histogram_draw;
    widget_class->get_preferred_width = entangle_image_histogram_get_preferred_width;
    widget_class->get_preferred_height = entangle_image_histogram_get_preferred_height;

    g_object_class_install_property(object_class,
                                    PROP_IMAGE,
                                    g_param_spec_object("image",
                                                        "Image",
                                                        "Image to be histogramed",
                                                        ENTANGLE_TYPE_IMAGE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleImageHistogramPrivate));
}


EntangleImageHistogram *entangle_image_histogram_new(void)
{
    return ENTANGLE_IMAGE_HISTOGRAM(g_object_new(ENTANGLE_TYPE_IMAGE_HISTOGRAM, NULL));
}


static void entangle_image_histogram_init(EntangleImageHistogram *histogram)
{
    EntangleImageHistogramPrivate *priv;

    priv = histogram->priv = ENTANGLE_IMAGE_HISTOGRAM_GET_PRIVATE(histogram);
    memset(priv, 0, sizeof *priv);

    gtk_widget_set_double_buffered(GTK_WIDGET(histogram), FALSE);
}


static void entangle_image_histogram_image_pixbuf_notify(GObject *image G_GNUC_UNUSED,
                                                         GParamSpec *pspec G_GNUC_UNUSED,
                                                         gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(data));

    EntangleImageHistogram *histogram = ENTANGLE_IMAGE_HISTOGRAM(data);

    do_entangle_pixmap_setup(histogram);
    gtk_widget_queue_draw(GTK_WIDGET(histogram));
}


/**
 * entangle_image_histogram_set_image:
 * @histogram: (transfer none): the histogram widget
 * @image: (transfer none)(allow-none): the image to display histogram for, or NULL
 *
 * Set the image to display the histogram for
 */
void entangle_image_histogram_set_image(EntangleImageHistogram *histogram,
                                        EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(histogram));
    g_return_if_fail(!image || ENTANGLE_IS_IMAGE(image));

    EntangleImageHistogramPrivate *priv = histogram->priv;

    if (priv->image) {
        g_signal_handler_disconnect(priv->image, priv->imageNotifyID);
        g_object_unref(priv->image);
    }
    priv->image = image;
    if (priv->image) {
        g_object_ref(priv->image);
        priv->imageNotifyID = g_signal_connect(priv->image,
                                               "notify::pixbuf",
                                               G_CALLBACK(entangle_image_histogram_image_pixbuf_notify),
                                               histogram);
    }

    do_entangle_pixmap_setup(histogram);

    if (gtk_widget_get_visible((GtkWidget*)histogram))
        gtk_widget_queue_draw(GTK_WIDGET(histogram));
}


/**
 * entangle_image_histogram_get_image:
 * @histogram: (transfer none): the histogram widget
 *
 * Retrieve the image that the histogram is displayed for
 *
 * Returns: (transfer none): the image displayed
 */
EntangleImage *entangle_image_histogram_get_image(EntangleImageHistogram *histogram)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(histogram), NULL);

    EntangleImageHistogramPrivate *priv = histogram->priv;

    return priv->image;
}


void entangle_image_histogram_set_histogram_linear(EntangleImageHistogram *histogram, gboolean linear)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(histogram));

    EntangleImageHistogramPrivate *priv = histogram->priv;

    priv->linear = linear;

    gtk_widget_queue_draw(GTK_WIDGET(histogram));
}


gboolean entangle_image_histogram_get_histogram_linear(EntangleImageHistogram *histogram)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_HISTOGRAM(histogram), FALSE);

    EntangleImageHistogramPrivate *priv = histogram->priv;

    return priv->linear;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
