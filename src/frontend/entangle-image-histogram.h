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

#ifndef __ENTANGLE_IMAGE_HISTOGRAM_H__
#define __ENTANGLE_IMAGE_HISTOGRAM_H__

#include <gtk/gtk.h>

#include "entangle-image-loader.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_IMAGE_HISTOGRAM            (entangle_image_histogram_get_type ())
#define ENTANGLE_IMAGE_HISTOGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_IMAGE_HISTOGRAM, EntangleImageHistogram))
#define ENTANGLE_IMAGE_HISTOGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_IMAGE_HISTOGRAM, EntangleImageHistogramClass))
#define ENTANGLE_IS_IMAGE_HISTOGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_IMAGE_HISTOGRAM))
#define ENTANGLE_IS_IMAGE_HISTOGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_IMAGE_HISTOGRAM))
#define ENTANGLE_IMAGE_HISTOGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_IMAGE_HISTOGRAM, EntangleImageHistogramClass))


typedef struct _EntangleImageHistogram EntangleImageHistogram;
typedef struct _EntangleImageHistogramPrivate EntangleImageHistogramPrivate;
typedef struct _EntangleImageHistogramClass EntangleImageHistogramClass;

struct _EntangleImageHistogram
{
    GtkDrawingArea parent;

    EntangleImageHistogramPrivate *priv;
};

struct _EntangleImageHistogramClass
{
    GtkDrawingAreaClass parent_class;

};

GType entangle_image_histogram_get_type(void) G_GNUC_CONST;

EntangleImageHistogram* entangle_image_histogram_new(void);

void entangle_image_histogram_set_image(EntangleImageHistogram *histogram,
                                        EntangleImage *image);
EntangleImage *entangle_image_histogram_get_image(EntangleImageHistogram *histogram);

void entangle_image_histogram_set_histogram_linear(EntangleImageHistogram *histogram,
                                                   gboolean linear);
gboolean entangle_image_histogram_get_histogram_linear(EntangleImageHistogram *histogram);

G_END_DECLS

#endif /* __ENTANGLE_IMAGE_HISTOGRAM_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
