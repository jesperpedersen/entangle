/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (c) 2005 VMware, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __ENTANGLE_OVERLAY_BOX_H__
#define __ENTANGLE_OVERLAY_BOX_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_OVERLAY_BOX            (entangle_overlay_box_get_type())
#define ENTANGLE_OVERLAY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), ENTANGLE_TYPE_OVERLAY_BOX, EntangleOverlayBox))
#define ENTANGLE_OVERLAY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), ENTANGLE_TYPE_OVERLAY_BOX, EntangleOverlayBoxClass))
#define ENTANGLE_IS_OVERLAY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), ENTANGLE_TYPE_OVERLAY_BOX))
#define ENTANGLE_IS_OVERLAY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), ENTANGLE_TYPE_OVERLAY_BOX))
#define ENTANGLE_OVERLAY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), ENTANGLE_TYPE_OVERLAY_BOX, EntangleOverlayBoxClass))

typedef struct _EntangleOverlayBox EntangleOverlayBox;
typedef struct _EntangleOverlayBoxPrivate EntangleOverlayBoxPrivate;
typedef struct _EntangleOverlayBoxClass EntangleOverlayBoxClass;

struct _EntangleOverlayBox {
    GtkBox parent;

    EntangleOverlayBoxPrivate *priv;
};

struct _EntangleOverlayBoxClass {
    GtkBoxClass parent;

    void (* set_over)(EntangleOverlayBox *box, GtkWidget *widget);
};



GType entangle_overlay_box_get_type(void);

EntangleOverlayBox *entangle_overlay_box_new(void);

void
entangle_overlay_box_set_under(EntangleOverlayBox *box,
                               GtkWidget *widget);

void
entangle_overlay_box_set_over(EntangleOverlayBox *box,
                              GtkWidget *widget);

void
entangle_overlay_box_set_min(EntangleOverlayBox *box,
                             unsigned int min);

void
entangle_overlay_box_set_fraction(EntangleOverlayBox *box,
                                  double fraction);

double
entangle_overlay_box_get_fraction(EntangleOverlayBox *box);


G_END_DECLS


#endif /* __ENTANGLE_OVERLAY_BOX_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
