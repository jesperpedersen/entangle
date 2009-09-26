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

#include "image-display.h"

#define CAPA_IMAGE_DISPLAY_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_IMAGE_DISPLAY, CapaImageDisplayPrivate))

struct _CapaImageDisplayPrivate {
  GdkPixbuf *pixbuf;

  gboolean autoscale;
  float scale;
};

G_DEFINE_TYPE(CapaImageDisplay, capa_image_display, GTK_TYPE_DRAWING_AREA);

enum {
  PROP_O,
  PROP_PIXBUF,
  PROP_AUTOSCALE,
  PROP_SCALE,
};

static void capa_image_display_get_property(GObject *object,
					  guint prop_id,
					  GValue *value,
					  GParamSpec *pspec)
{
  CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(object);
  CapaImageDisplayPrivate *priv = display->priv;

  switch (prop_id)
    {
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

static void capa_image_display_set_property(GObject *object,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *pspec)
{
  CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(object);
  CapaImageDisplayPrivate *priv = display->priv;

  fprintf(stderr, "Set prop on image display %d\n", prop_id);

  switch (prop_id)
    {
    case PROP_PIXBUF:
      if (priv->pixbuf)
	g_object_unref(G_OBJECT(priv->pixbuf));
      priv->pixbuf = g_value_get_object(value);
      g_object_ref(G_OBJECT(priv->pixbuf));

      if (GTK_WIDGET_VISIBLE(object))
	gtk_widget_queue_resize(GTK_WIDGET(object));
      else
	fprintf(stderr, "not visible\n");
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

  if (priv->pixbuf)
    g_object_unref(G_OBJECT(priv->pixbuf));

  G_OBJECT_CLASS (capa_image_display_parent_class)->finalize (object);
}

static gboolean capa_image_display_expose(GtkWidget *widget,
					  GdkEventExpose *expose)
{
  CapaImageDisplay *display = CAPA_IMAGE_DISPLAY(widget);
  CapaImageDisplayPrivate *priv = display->priv;
  int ww, wh; /* Available drawing area extents */
  int pw, ph; /* Original image size */
  double iw, ih; /* Desired image size */
  double mx = 0, my = 0;  /* Offset of image within available area */
  double sx = 1, sy = 1;  /* Amount to scale by */
  cairo_t *cr;

  gdk_drawable_get_size(widget->window, &ww, &wh);

  pw = gdk_pixbuf_get_width(priv->pixbuf);
  ph = gdk_pixbuf_get_height(priv->pixbuf);

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

  fprintf(stderr, "Got win %dx%d image %dx%d, autoscale=%d scale=%lf\n",
	  ww, wh, pw, ph, priv->autoscale ? 1 : 0, priv->scale);
  fprintf(stderr, "Drawing image %lf,%lf at %lf %lf sclaed %lfx%lf\n", iw, ih, mx, my, sx, sy);


  cr = gdk_cairo_create(widget->window);
  cairo_rectangle(cr,
		  expose->area.x,
		  expose->area.y,
		  expose->area.width + 1,
		  expose->area.height + 1);
  cairo_clip(cr);

  /* We need to fill the background first */
  cairo_rectangle(cr, 0, 0, ww, wh);
  /* Next cut out the inner area where the pixbuf
     will be drawn. This avoids 'flashing' since we're
     not double-buffering. Note we're using the undocumented
     behaviour of drawing the rectangle from right to left
     to cut out the whole */
  if (priv->pixbuf)
    cairo_rectangle(cr,
		    mx + iw,
		    my,
		    -1 * iw,
		    ih);
  cairo_fill(cr);

  /* Draw the actual image */
  if (priv->pixbuf) {
#if 0
    cairo_scale(cr, sx, sy);
    gdk_cairo_set_source_pixbuf(cr,
				priv->pixbuf,
				mx, my);
#else
    cairo_scale(cr, sx, sy);
    gdk_cairo_set_source_pixbuf(cr,
				priv->pixbuf,
				mx/sx, my/sy);
#endif
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
    fprintf(stderr, "No image, size request 100,100\n");
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

  fprintf(stderr, "Size request %d %d\n", requisition->width, requisition->height);
}

static void capa_image_display_class_init(CapaImageDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  object_class->finalize = capa_image_display_finalize;
  object_class->get_property = capa_image_display_get_property;
  object_class->set_property = capa_image_display_set_property;

  widget_class->expose_event = capa_image_display_expose;
  widget_class->size_request = capa_image_display_size_request;

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

