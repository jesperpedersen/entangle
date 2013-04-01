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

#include <string.h>
#include <math.h>

#include "entangle-debug.h"
#include "entangle-image-display.h"
#include "entangle-image.h"

#define ENTANGLE_IMAGE_DISPLAY_GET_PRIVATE(obj)                         \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_IMAGE_DISPLAY, EntangleImageDisplayPrivate))

struct _EntangleImageDisplayPrivate {
    GList *images;

    cairo_surface_t *pixmap;

    gboolean autoscale;
    gdouble scale;

    gdouble aspectRatio;
    gdouble maskOpacity;
    gboolean maskEnabled;

    gboolean focusPoint;
    EntangleImageDisplayGrid gridDisplay;
};

G_DEFINE_TYPE(EntangleImageDisplay, entangle_image_display, GTK_TYPE_DRAWING_AREA);

enum {
    PROP_O,
    PROP_IMAGE,
    PROP_AUTOSCALE,
    PROP_SCALE,
    PROP_ASPECT_RATIO,
    PROP_MASK_OPACITY,
    PROP_MASK_ENABLED,
    PROP_FOCUS_POINT,
    PROP_GRID_DISPLAY,
};


static void entangle_image_display_image_pixbuf_notify(GObject *image,
                                                       GParamSpec *pspec,
                                                       gpointer data);


static void do_entangle_image_display_render_pixmap(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;
    int pw, ph;
    GdkPixbuf *pixbuf = NULL;
    GList *tmp = priv->images;
    EntangleImage *image = ENTANGLE_IMAGE(priv->images->data);

    ENTANGLE_DEBUG("Setting up server pixmap for %p %s",
                   image, entangle_image_get_filename(image));

    image = ENTANGLE_IMAGE(tmp->data);
    pixbuf = entangle_image_get_pixbuf(image);

    pw = gdk_pixbuf_get_width(pixbuf);
    ph = gdk_pixbuf_get_height(pixbuf);
    priv->pixmap = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pw, ph);

    /* Paint the stack of images - the first one
     * is completely opaque. Others are layers
     * on top */
    cairo_t *cr = cairo_create(priv->pixmap);
    while (tmp) {
        image = ENTANGLE_IMAGE(tmp->data);
        pixbuf = entangle_image_get_pixbuf(image);

        gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
        if (tmp == priv->images)
            cairo_paint(cr);
        else
            cairo_paint_with_alpha(cr, 0.5);

        tmp = tmp->next;
    }
    cairo_destroy(cr);
}


static void entangle_image_display_try_render_pixmap(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    GList *tmp = priv->images;
    gboolean missing = FALSE;

    if (!gtk_widget_get_realized(GTK_WIDGET(display))) {
        ENTANGLE_DEBUG("Skipping setup for non-realized widget");
        return;
    }

    if (priv->pixmap) {
        cairo_surface_destroy(priv->pixmap);
        priv->pixmap = NULL;
    }

    if (!priv->images)
        return;

    while (tmp) {
        EntangleImage *image = tmp->data;

        if (entangle_image_get_pixbuf(image) == NULL)
            missing = TRUE;

        tmp = tmp->next;
    }

    if (!missing) {
        do_entangle_image_display_render_pixmap(display);
    } else {
        ENTANGLE_DEBUG("Not ready to render yet");
    }
}


static void entangle_image_display_image_pixbuf_notify(GObject *object G_GNUC_UNUSED,
                                                       GParamSpec *pspec G_GNUC_UNUSED,
                                                       gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(data));

    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(data);

    entangle_image_display_try_render_pixmap(display);
    gtk_widget_queue_resize(GTK_WIDGET(display));
    gtk_widget_queue_draw(GTK_WIDGET(display));
}



static void do_entangle_image_display_connect(EntangleImageDisplay *display,
                                              EntangleImage *image)
{
    g_signal_connect(image,
                     "notify::pixbuf",
                     G_CALLBACK(entangle_image_display_image_pixbuf_notify),
                     display);

    entangle_image_display_try_render_pixmap(display);
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
        case PROP_AUTOSCALE:
            g_value_set_boolean(value, priv->autoscale);
            break;

        case PROP_SCALE:
            g_value_set_float(value, priv->scale);
            break;

        case PROP_ASPECT_RATIO:
            g_value_set_float(value, priv->aspectRatio);
            break;

        case PROP_MASK_OPACITY:
            g_value_set_float(value, priv->maskOpacity);
            break;

        case PROP_MASK_ENABLED:
            g_value_set_boolean(value, priv->maskEnabled);
            break;

        case PROP_FOCUS_POINT:
            g_value_set_boolean(value, priv->focusPoint);
            break;

        case PROP_GRID_DISPLAY:
            g_value_set_enum(value, priv->gridDisplay);
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

    ENTANGLE_DEBUG("Set prop on image display %d", prop_id);

    switch (prop_id)
        {
        case PROP_AUTOSCALE:
            entangle_image_display_set_autoscale(display, g_value_get_boolean(value));
            break;

        case PROP_SCALE:
            entangle_image_display_set_scale(display, g_value_get_float(value));
            break;

        case PROP_ASPECT_RATIO:
            entangle_image_display_set_aspect_ratio(display, g_value_get_float(value));
            break;

        case PROP_MASK_OPACITY:
            entangle_image_display_set_mask_opacity(display, g_value_get_float(value));
            break;

        case PROP_MASK_ENABLED:
            entangle_image_display_set_mask_enabled(display, g_value_get_boolean(value));
            break;

        case PROP_FOCUS_POINT:
            entangle_image_display_set_focus_point(display, g_value_get_boolean(value));
            break;

        case PROP_GRID_DISPLAY:
            entangle_image_display_set_grid_display(display, g_value_get_enum(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_image_display_finalize(GObject *object)
{
    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(object);
    EntangleImageDisplayPrivate *priv = display->priv;
    GList *tmp = priv->images;

    while (tmp) {
        EntangleImage *image = tmp->data;

        g_signal_handlers_disconnect_by_data(image, display);
        g_object_unref(image);

        tmp = tmp->next;
    }
    g_list_free(priv->images);

    if (priv->pixmap)
        cairo_surface_destroy(priv->pixmap);

    G_OBJECT_CLASS (entangle_image_display_parent_class)->finalize (object);
}


static void entangle_image_display_realize(GtkWidget *widget)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(widget));

    GTK_WIDGET_CLASS(entangle_image_display_parent_class)->realize(widget);

    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(widget);

    entangle_image_display_try_render_pixmap(display);
}


static void entangle_image_display_draw_focus_point(GtkWidget *widget, cairo_t *cr)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(widget));

    gint ww, wh, cx, cy;

    ww = gdk_window_get_width(gtk_widget_get_window(widget));
    wh = gdk_window_get_height(gtk_widget_get_window(widget));

    cx = ww / 2;
    cy = wh / 2;

    cairo_set_source_rgba(cr, 0.7, 0, 0, 1);

#define SIZE 12
#define GAP 4
#define WIDTH 3

    /* top left */
    cairo_move_to(cr, cx - SIZE, cy - SIZE);
    cairo_line_to(cr, cx - GAP, cy - SIZE);
    cairo_line_to(cr, cx - GAP, cy - SIZE + WIDTH);
    cairo_line_to(cr, cx - SIZE + WIDTH, cy - SIZE + WIDTH);
    cairo_line_to(cr, cx - SIZE + WIDTH, cy - GAP);
    cairo_line_to(cr, cx - SIZE, cy - GAP);
    cairo_move_to(cr, cx - SIZE, cy - SIZE);
    cairo_fill(cr);

    /* top right */
    cairo_move_to(cr, cx + GAP, cy - SIZE);
    cairo_line_to(cr, cx + SIZE, cy - SIZE);
    cairo_line_to(cr, cx + SIZE, cy - GAP);
    cairo_line_to(cr, cx + SIZE - WIDTH, cy - GAP);
    cairo_line_to(cr, cx + SIZE - WIDTH, cy - SIZE + WIDTH);
    cairo_line_to(cr, cx + GAP, cy - SIZE + WIDTH);
    cairo_move_to(cr, cx + GAP, cy - SIZE);
    cairo_fill(cr);

    /* bottom left */
    cairo_move_to(cr, cx - SIZE, cy + SIZE);
    cairo_line_to(cr, cx - SIZE, cy + GAP);
    cairo_line_to(cr, cx - SIZE + WIDTH, cy + GAP);
    cairo_line_to(cr, cx - SIZE + WIDTH, cy + SIZE - WIDTH);
    cairo_line_to(cr, cx - GAP, cy + SIZE - WIDTH);
    cairo_line_to(cr, cx - GAP, cy + SIZE);
    cairo_line_to(cr, cx - SIZE, cy + SIZE);
    cairo_fill(cr);

    /* bottom right */
    cairo_move_to(cr, cx + WIDTH, cy + SIZE);
    cairo_line_to(cr, cx + SIZE, cy + SIZE);
    cairo_line_to(cr, cx + SIZE, cy + WIDTH);
    cairo_line_to(cr, cx + SIZE - WIDTH, cy + WIDTH);
    cairo_line_to(cr, cx + SIZE - WIDTH, cy + SIZE - WIDTH);
    cairo_line_to(cr, cx + WIDTH, cy + SIZE - WIDTH);
    cairo_line_to(cr, cx + WIDTH, cy + SIZE);
    cairo_fill(cr);
}


static void entangle_image_display_draw_grid_display(GtkWidget *widget, cairo_t *cr,
                                                     gdouble mx, gdouble my)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(widget));

    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(widget);
    EntangleImageDisplayPrivate *priv = display->priv;
    gint ww = gdk_window_get_width(gtk_widget_get_window(widget));
    gint wh = gdk_window_get_height(gtk_widget_get_window(widget));
    gdouble cx = (ww - (mx * 2))/2.0;
    gdouble cy = (wh - (my * 2))/2.0;
    gdouble offx[4], offy[4];
    gint count;
    gint i;

    cairo_set_source_rgba(cr, 0.7, 0, 0, 1);

    if (priv->focusPoint) {
        /* Cut out the area where the focus point lives
         * to avoid lines going through it */
        cairo_rectangle(cr, mx, my, ww - (mx * 2), wh - (my * 2));
        cairo_rectangle(cr,
                        ww - mx - cx + SIZE,
                        my + cy - SIZE,
                        -1 * SIZE * 2,
                        SIZE * 2);
        cairo_clip(cr);
    }

    switch (priv->gridDisplay) {
    case ENTANGLE_IMAGE_DISPLAY_GRID_CENTER_LINES:
        count = 1;
        offx[0] = (ww - (mx * 2)) / 2;
        offy[0] = (wh - (my * 2)) / 2;
        break;

    case ENTANGLE_IMAGE_DISPLAY_GRID_RULE_OF_3RDS:
        count = 2;
        offx[0] = (ww - (mx * 2)) / 3;
        offy[0] = (wh - (my * 2)) / 3;
        offx[1] = (ww - (mx * 2)) / 3 * 2;
        offy[1] = (wh - (my * 2)) / 3 * 2;
        break;

    case ENTANGLE_IMAGE_DISPLAY_GRID_QUARTERS:
        count = 3;
        offx[0] = (ww - (mx * 2)) / 4;
        offy[0] = (wh - (my * 2)) / 4;
        offx[1] = (ww - (mx * 2)) / 4 * 2;
        offy[1] = (wh - (my * 2)) / 4 * 2;
        offx[2] = (ww - (mx * 2)) / 4 * 3;
        offy[2] = (wh - (my * 2)) / 4 * 3;
        break;

    case ENTANGLE_IMAGE_DISPLAY_GRID_RULE_OF_5THS:
        count = 4;
        offx[0] = (ww - (mx * 2)) / 5;
        offy[0] = (wh - (my * 2)) / 5;
        offx[1] = (ww - (mx * 2)) / 5 * 2;
        offy[1] = (wh - (my * 2)) / 5 * 2;
        offx[2] = (ww - (mx * 2)) / 5 * 3;
        offy[2] = (wh - (my * 2)) / 5 * 3;
        offx[3] = (ww - (mx * 2)) / 5 * 4;
        offy[3] = (wh - (my * 2)) / 5 * 4;
        break;

    case ENTANGLE_IMAGE_DISPLAY_GRID_GOLDEN_SECTIONS:
        count = 2;
        offx[0] = (ww - (mx * 2)) / 100 * 38;
        offy[0] = (wh - (my * 2)) / 100 * 38;
        offx[1] = (ww - (mx * 2)) / 100 * 62;
        offy[1] = (wh - (my * 2)) / 100 * 62;
        break;

    case ENTANGLE_IMAGE_DISPLAY_GRID_NONE:
    default:
        count = 0;
        break;
    }

    for (i = 0 ; i < count ; i++) {
        /* Horizontal line */
        cairo_move_to(cr, mx, my + offy[i] - 1);
        cairo_line_to(cr, ww - mx, my + offy[i] - 1);
        cairo_line_to(cr, ww - mx, my + offy[i]);
        cairo_line_to(cr, mx, my + offy[i]);
        cairo_line_to(cr, mx, my + offy[i] - 1);
        cairo_fill(cr);

        /* Vertical line */
        cairo_move_to(cr, mx + offx[i] - 1, my);
        cairo_line_to(cr, mx + offx[i], my);
        cairo_line_to(cr, mx + offx[i], wh - my);
        cairo_line_to(cr, mx + offx[i] - 1, wh - my);
        cairo_line_to(cr, mx + offx[i] - 1, my);
        cairo_fill(cr);
    }
}


static gboolean entangle_image_display_draw(GtkWidget *widget, cairo_t *cr)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(widget), FALSE);

    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(widget);
    EntangleImageDisplayPrivate *priv = display->priv;
    int ww, wh; /* Available drawing area extents */
    int pw = 0, ph = 0; /* Original image size */
    double iw, ih; /* Desired image size */
    double mx = 0, my = 0;  /* Offset of image within available area */
    double sx = 1, sy = 1;  /* Amount to scale by */
    double aspectWin, aspectImage = 0.0;

    ww = gdk_window_get_width(gtk_widget_get_window(widget));
    wh = gdk_window_get_height(gtk_widget_get_window(widget));
    aspectWin = (double)ww / (double)wh;

    if (priv->pixmap) {
        pw = cairo_image_surface_get_width(priv->pixmap);
        ph = cairo_image_surface_get_height(priv->pixmap);
        aspectImage = (double)pw / (double)ph;
    }


    /* Decide what size we're going to draw the image */
    if (priv->autoscale) {
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

    /* Draw the actual image(s) */
    if (priv->pixmap) {
        cairo_matrix_t m;
        cairo_get_matrix(cr, &m);
        cairo_scale(cr, sx, sy);

        cairo_set_source_surface(cr,
                                 priv->pixmap,
                                 mx/sx, my/sy);
        cairo_paint(cr);
        cairo_set_matrix(cr, &m);
    }

    /* Draw the focus point */
    if (priv->focusPoint)
        entangle_image_display_draw_focus_point(widget, cr);

    /* Draw the grid lines */
    entangle_image_display_draw_grid_display(widget, cr, mx, my);

    /* Finally a possible aspect ratio mask */
    if (priv->pixmap && priv->maskEnabled &&
        (fabs(priv->aspectRatio - aspectImage)  > 0.005)) {
        cairo_set_source_rgba(cr, 0, 0, 0, priv->maskOpacity);

        if (priv->aspectRatio > aspectImage) { /* mask top & bottom */
            double ah = (ih - (iw / priv->aspectRatio)) / 2;

            cairo_rectangle(cr,
                            mx, my,
                            iw, ah);
            cairo_fill(cr);

            cairo_rectangle(cr,
                            mx, my + ih - ah,
                            iw, ah);
            cairo_fill(cr);
        } else { /* mask left & right */
            double aw = (iw - (ih * priv->aspectRatio)) / 2;

            cairo_rectangle(cr,
                            mx, my,
                            aw, ih);
            cairo_fill(cr);

            cairo_rectangle(cr,
                            mx + iw - aw, my,
                            aw, ih);
            cairo_fill(cr);
        }
    }

    return TRUE;
}


static void entangle_image_display_get_preferred_width(GtkWidget *widget,
                                                       gint *minwidth,
                                                       gint *natwidth)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(widget));

    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(widget);
    EntangleImageDisplayPrivate *priv = display->priv;
    GdkPixbuf *pixbuf = NULL;
    EntangleImage *image = entangle_image_display_get_image(display);

    if (image)
        pixbuf = entangle_image_get_pixbuf(image);

    if (!pixbuf) {
        *minwidth = *natwidth = 100;
        ENTANGLE_DEBUG("No image, size request 100,100");
        return;
    }

    if (priv->autoscale) {
        /* For best fit mode, we'll say 100x100 is the smallest we're happy
         * to draw to. Whatever the container allocates us beyond that will
         * be used filled with the image */
        *minwidth = *natwidth = 100;
    } else {
        /* Start a 1-to-1 mode */
        *minwidth = *natwidth = gdk_pixbuf_get_width(pixbuf);
        if (priv->scale > 0) {
            /* Scaling mode */
            *minwidth = *natwidth = (int)((double)*minwidth * priv->scale);
        }
    }
}


static void entangle_image_display_get_preferred_height(GtkWidget *widget,
                                                        gint *minheight,
                                                        gint *natheight)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(widget));

    EntangleImageDisplay *display = ENTANGLE_IMAGE_DISPLAY(widget);
    EntangleImageDisplayPrivate *priv = display->priv;
    GdkPixbuf *pixbuf = NULL;
    EntangleImage *image = entangle_image_display_get_image(display);

    if (image)
        pixbuf = entangle_image_get_pixbuf(image);

    if (!pixbuf) {
        *minheight = *natheight = 100;
        ENTANGLE_DEBUG("No image, size request 100,100");
        return;
    }

    if (priv->autoscale) {
        /* For best fit mode, we'll say 100x100 is the smallest we're happy
         * to draw to. Whatever the container allocates us beyond that will
         * be used filled with the image */
        *minheight = *natheight = 100;
    } else {
        /* Start a 1-to-1 mode */
        *minheight = *natheight = gdk_pixbuf_get_height(pixbuf);
        if (priv->scale > 0) {
            /* Scaling mode */
            *minheight = *natheight = (int)((double)*minheight * priv->scale);
        }
    }
}


static void entangle_image_display_class_init(EntangleImageDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->finalize = entangle_image_display_finalize;
    object_class->get_property = entangle_image_display_get_property;
    object_class->set_property = entangle_image_display_set_property;

    widget_class->realize = entangle_image_display_realize;
    widget_class->draw = entangle_image_display_draw;
    widget_class->get_preferred_width = entangle_image_display_get_preferred_width;
    widget_class->get_preferred_height = entangle_image_display_get_preferred_height;

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
                                    PROP_ASPECT_RATIO,
                                    g_param_spec_float("aspect-ratio",
                                                       "Aspect ratio",
                                                       "Aspect ratio to mask image to",
                                                       0.0,
                                                       100.0,
                                                       1.69,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_MASK_OPACITY,
                                    g_param_spec_float("mask-opacity",
                                                       "Mask opacity",
                                                       "Mask opacity when adjusting aspect ratio",
                                                       0.0,
                                                       1.0,
                                                       0.5,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_MASK_ENABLED,
                                    g_param_spec_boolean("mask-enabled",
                                                         "Mask enabled",
                                                         "Enable aspect ratio image mask",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_FOCUS_POINT,
                                    g_param_spec_boolean("focus-point",
                                                         "Focus point",
                                                         "Overlay center focus point",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_GRID_DISPLAY,
                                    g_param_spec_enum("grid-display",
                                                      "Grid display",
                                                      "Grid line display",
                                                      ENTANGLE_TYPE_IMAGE_DISPLAY_GRID,
                                                      ENTANGLE_IMAGE_DISPLAY_GRID_NONE,
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

    priv->autoscale = TRUE;
    priv->maskOpacity = 0.9;
    priv->aspectRatio = 1.33;
    priv->maskEnabled = FALSE;
}


void entangle_image_display_set_image(EntangleImageDisplay *display,
                                      EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display));
    g_return_if_fail(!image || ENTANGLE_IS_IMAGE(image));

    GList *tmp = g_list_append(NULL, image);
    entangle_image_display_set_image_list(display, tmp);
    g_list_free(tmp);
}


EntangleImage *entangle_image_display_get_image(EntangleImageDisplay *display)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display), NULL);

    EntangleImageDisplayPrivate *priv = display->priv;

    if (!priv->images)
        return NULL;

    return ENTANGLE_IMAGE(priv->images->data);
}


void entangle_image_display_set_image_list(EntangleImageDisplay *display,
                                           GList *images)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display));

    EntangleImageDisplayPrivate *priv = display->priv;
    GList *tmp;

    tmp = priv->images;
    while (tmp) {
        EntangleImage *image = ENTANGLE_IMAGE(tmp->data);

        g_signal_handlers_disconnect_by_data(image, display);
        g_object_unref(image);

        tmp = tmp->next;
    }
    g_list_free(priv->images);

    priv->images = NULL;

    tmp = images;
    while (tmp) {
        EntangleImage *image = ENTANGLE_IMAGE(tmp->data);

        do_entangle_image_display_connect(display, image);

        priv->images = g_list_append(priv->images, g_object_ref(image));

        tmp = tmp->next;
    }
    priv->images = g_list_reverse(priv->images);

    entangle_image_display_try_render_pixmap(display);
    gtk_widget_queue_resize(GTK_WIDGET(display));
    gtk_widget_queue_draw(GTK_WIDGET(display));
}


GList *entangle_image_display_get_image_list(EntangleImageDisplay *display)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display), FALSE);

    EntangleImageDisplayPrivate *priv = display->priv;

    g_list_foreach(priv->images, (GFunc)g_object_ref, NULL);

    return g_list_copy(priv->images);
}


void entangle_image_display_set_autoscale(EntangleImageDisplay *display,
                                          gboolean autoscale)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display));

    EntangleImageDisplayPrivate *priv = display->priv;

    priv->autoscale = autoscale;

    if (gtk_widget_get_visible((GtkWidget*)display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gboolean entangle_image_display_get_autoscale(EntangleImageDisplay *display)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display), FALSE);

    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->autoscale;
}


void entangle_image_display_set_scale(EntangleImageDisplay *display,
                                      gdouble scale)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display));

    EntangleImageDisplayPrivate *priv = display->priv;

    priv->scale = scale;

    if (gtk_widget_get_visible((GtkWidget*)display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gdouble entangle_image_display_get_scale(EntangleImageDisplay *display)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display), 1.0);

    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->scale;
}


void entangle_image_display_set_aspect_ratio(EntangleImageDisplay *display,
                                             gdouble aspect)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display));

    EntangleImageDisplayPrivate *priv = display->priv;

    priv->aspectRatio = aspect;

    if (gtk_widget_get_visible((GtkWidget*)display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gdouble entangle_image_display_get_aspect_ratio(EntangleImageDisplay *display)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display), 1.0);

    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->aspectRatio;
}


void entangle_image_display_set_mask_opacity(EntangleImageDisplay *display,
                                             gdouble opacity)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display));

    EntangleImageDisplayPrivate *priv = display->priv;

    priv->maskOpacity = opacity;

    if (gtk_widget_get_visible((GtkWidget*)display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gdouble entangle_image_display_get_mask_opacity(EntangleImageDisplay *display)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display), 1.0);

    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->maskOpacity;
}


void entangle_image_display_set_mask_enabled(EntangleImageDisplay *display,
                                             gboolean enabled)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display));

    EntangleImageDisplayPrivate *priv = display->priv;

    priv->maskEnabled = enabled;

    if (gtk_widget_get_visible((GtkWidget*)display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gboolean entangle_image_display_get_mask_enabled(EntangleImageDisplay *display)
{
    g_return_val_if_fail(ENTANGLE_IS_IMAGE_DISPLAY(display), FALSE);

    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->maskEnabled;
}



void entangle_image_display_set_focus_point(EntangleImageDisplay *display,
                                            gboolean enabled)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    priv->focusPoint = enabled;

    if (gtk_widget_get_visible((GtkWidget*)display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


gboolean entangle_image_display_get_focus_point(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->focusPoint;
}

void entangle_image_display_set_grid_display(EntangleImageDisplay *display,
                                             EntangleImageDisplayGrid mode)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    priv->gridDisplay = mode;

    if (gtk_widget_get_visible((GtkWidget*)display))
        gtk_widget_queue_resize(GTK_WIDGET(display));
}


EntangleImageDisplayGrid entangle_image_display_get_grid_display(EntangleImageDisplay *display)
{
    EntangleImageDisplayPrivate *priv = display->priv;

    return priv->gridDisplay;
}



/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
