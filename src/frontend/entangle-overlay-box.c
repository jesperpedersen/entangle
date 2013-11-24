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

/*
 * Formerly ovBox.c --
 *
 *      Implementation of a GTK+ overlapping box. Allows you to display and
 *      quickly move a child that overlaps another child.
 *
 *      Implementation notes
 *      --------------------
 *
 *      Changing 'fraction' is fast (we just move the 'overWin' X window, which
 *      ultimately copies a rectangle on the X server side), and does not
 *      flicker (the 'under' and 'over' GTK children are not re-drawn, except
 *      for parts of them that become exposed).
 *
 *      o Initially, we thought it could be done with only 2 X windows
 *
 *        Layout                  Hierarchy
 *        ------                  ---------
 *
 *          /- overWin --\        underWin
 *          |            |           overWin
 *        /-+- underWin -+-\
 *        | |            | |
 *        | \------------/ |
 *        |                |
 *        \----------------/
 *
 *        But the 'under' GTK child could create other X windows inside
 *        'underWin', which makes it impossible to guarantee that 'overWin'
 *        will stay stacked on top.
 *
 *      o So we are forced to use 3 X windows
 *
 *        Layout                  Hierarchy
 *        ------                  ---------
 *
 *            /- overWin --\      window
 *            |            |         overWin
 *        /---+- window ---+---\     underWin
 *        |   |            |   |
 *        | /-+- underWin -+-\ |
 *        | | |            | | |
 *        | | \------------/ | |
 *        | |                | |
 *        | \----------------/ |
 *        |                    |
 *        \--------------------/
 *
 *  --hpreg
 */

#include <config.h>

#include "entangle-overlay-box.h"


struct _EntangleOverlayBoxPrivate
{
    GdkWindow *underWin;
    GtkWidget *under;
    GdkWindow *overWin;
    GtkWidget *over;
    GtkRequisition overR;
    unsigned int min;
    double fraction;
    gint verticalOffset;
};

#define ENTANGLE_OVERLAY_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_OVERLAY_BOX, EntangleOverlayBoxPrivate))

G_DEFINE_TYPE(EntangleOverlayBox, entangle_overlay_box, GTK_TYPE_BOX);

/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_init --
 *
 *      Initialize a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_init(EntangleOverlayBox *box)
{
    EntangleOverlayBoxPrivate *priv;

    priv = box->priv = ENTANGLE_OVERLAY_BOX_GET_PRIVATE(box);

    gtk_widget_set_has_window (GTK_WIDGET (box), TRUE);

    priv->underWin = NULL;
    priv->under = NULL;
    priv->overWin = NULL;
    priv->over = NULL;
    priv->overR.height = -1;
    priv->overR.width = -1;
    priv->min = 0;
    priv->fraction = 0;
    priv->verticalOffset = 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_map --
 *
 *      "map" method of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_map(GtkWidget *widget)
{
    gdk_window_show(gtk_widget_get_window (widget));
    GTK_WIDGET_CLASS(entangle_overlay_box_parent_class)->map(widget);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_unmap --
 *
 *      "unmap" method of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_unmap(GtkWidget *widget)
{
    gdk_window_hide(gtk_widget_get_window (widget));
    GTK_WIDGET_CLASS(entangle_overlay_box_parent_class)->unmap(widget);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_get_actual_min --
 *
 *      Retrieve the actual 'min' value, i.e. a value that is guaranteed not to
 *      exceed the height of the 'over' child.
 *
 * Results:
 *      The actual 'min' value.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static inline unsigned int
entangle_overlay_box_get_actual_min(EntangleOverlayBox *box)
{
    return MIN(box->priv->min, box->priv->overR.height);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_get_under_geometry --
 *
 *      Retrieve the geometry to apply to 'that->underWin'.
 *
 * Results:
 *      The geometry
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_get_under_geometry(EntangleOverlayBox *box,
                                        int *x,
                                        int *y,
                                        int *width,
                                        int *height)
{
    unsigned int min;
    GtkAllocation allocation;

    min = entangle_overlay_box_get_actual_min(box);
    gtk_widget_get_allocation(GTK_WIDGET(box), &allocation);

    *x = 0;
    *y = min;
    *width = allocation.width;
    *height = allocation.height - min;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_get_over_geometry --
 *
 *      Retrieve the geometry to apply to 'that->overWin'.
 *
 * Results:
 *      The geometry
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_get_over_geometry(EntangleOverlayBox *box,
                                       int *x,
                                       int *y,
                                       int *width,
                                       int *height)
{
    EntangleOverlayBoxPrivate *priv;
    gboolean expand;
    gboolean fill;
    guint padding;
    unsigned int boxWidth;
    GtkAllocation allocation;

    priv = box->priv;

    if (priv->over) {
        /*
         * When a child's expand or fill property changes, GtkBox queues
         * a resize for the child.
         */
        gtk_container_child_get(GTK_CONTAINER(box), priv->over,
                                "expand", &expand,
                                "fill", &fill,
                                "padding", &padding,
                                NULL);
    } else {
        /* Default values used by GtkBox. */
        expand = TRUE;
        fill = TRUE;
        padding = 0;
    }

    gtk_widget_get_allocation(GTK_WIDGET(box), &allocation);
    boxWidth = allocation.width;
    if (!expand) {
        *width = MIN(priv->overR.width, boxWidth - padding);
        *x = padding;
    } else if (!fill) {
        *width = MIN(priv->overR.width, boxWidth);
        *x = (boxWidth - *width) / 2;
    } else {
        *width = boxWidth;
        *x = 0;
    }

    *y = (priv->overR.height - entangle_overlay_box_get_actual_min(box))
        * (priv->fraction - 1) + priv->verticalOffset;
    *height = priv->overR.height;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_set_background --
 *
 *      Set the background color of the 'underWin' and 'overWin' X windows.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_set_background(EntangleOverlayBox *box)
{
    GtkWidget *widget;
    GtkStyleContext *stylecontext;

    widget = GTK_WIDGET(box);
    stylecontext = gtk_widget_get_style_context(widget);
    gtk_style_context_set_background(stylecontext, gtk_widget_get_window(widget));
    gtk_style_context_set_background(stylecontext, box->priv->underWin);
    gtk_style_context_set_background(stylecontext, box->priv->overWin);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_realize --
 *
 *      "realize" method of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_realize(GtkWidget *widget)
{
    EntangleOverlayBox *box;
    EntangleOverlayBoxPrivate *priv;
    GdkWindowAttr attributes;
    gint mask;
    GtkAllocation allocation;
    GdkWindow *window;

    gtk_widget_set_realized (widget, TRUE);

    box = ENTANGLE_OVERLAY_BOX(widget);
    priv = box->priv;

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
    mask = GDK_WA_VISUAL | GDK_WA_X | GDK_WA_Y;

    gtk_widget_get_allocation(widget, &allocation);
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    window = gdk_window_new(gtk_widget_get_parent_window(widget),
                            &attributes, mask);
    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, box);

    /*
     * The order in which we create the children X window matters: the child
     * created last is stacked on top. --hpreg
     */

    entangle_overlay_box_get_under_geometry(box, &attributes.x, &attributes.y,
                                            &attributes.width, &attributes.height);
    priv->underWin = gdk_window_new(window, &attributes, mask);
    gdk_window_set_user_data(priv->underWin, box);
    if (priv->under) {
        gtk_widget_set_parent_window(priv->under, priv->underWin);
    }
    gdk_window_show(priv->underWin);

    entangle_overlay_box_get_over_geometry(box, &attributes.x, &attributes.y,
                                           &attributes.width, &attributes.height);
    priv->overWin = gdk_window_new(window, &attributes, mask);
    gdk_window_set_user_data(priv->overWin, box);
    if (priv->over) {
        gtk_widget_set_parent_window(priv->over, priv->overWin);
    }
    gdk_window_show(priv->overWin);

    entangle_overlay_box_set_background(box);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_unrealize --
 *
 *      "unrealize" method of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_unrealize(GtkWidget *widget)
{
    EntangleOverlayBox *box;
    EntangleOverlayBoxPrivate *priv;

    box = ENTANGLE_OVERLAY_BOX(widget);
    priv = box->priv;

    /*
     * Unrealize the parent before destroying the windows so that we end up
     * unrealizing all the child widgets before destroying the child windows,
     * giving them a chance to reparent their windows before we clobber them.
     */
    GTK_WIDGET_CLASS(entangle_overlay_box_parent_class)->unrealize(widget);


    gdk_window_set_user_data(priv->underWin, NULL);
    gdk_window_destroy(priv->underWin);
    priv->underWin = NULL;

    gdk_window_set_user_data(priv->overWin, NULL);
    gdk_window_destroy(priv->overWin);
    priv->overWin = NULL;

}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_size_request --
 *
 *      "size_request" method of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */
static void
entangle_overlay_box_size_request(GtkWidget *widget,
                                  GtkRequisition *requisition)
{
    EntangleOverlayBox *box;
    EntangleOverlayBoxPrivate *priv;
    GtkRequisition underR;
    gboolean expand;
    gboolean fill;
    guint padding;
    unsigned int min;

    box = ENTANGLE_OVERLAY_BOX(widget);
    priv = box->priv;

    gtk_widget_get_preferred_size(priv->under, NULL, &underR);
    gtk_widget_get_preferred_size(priv->over, NULL, &priv->overR);

    gtk_container_child_get(GTK_CONTAINER(box), priv->over,
                            "expand", &expand,
                            "fill", &fill,
                            "padding", &padding,
                            NULL);
    requisition->width = MAX(underR.width,
                             priv->overR.width + ((expand || fill) ? 0 : padding));
    min = entangle_overlay_box_get_actual_min(box);
    requisition->height = MAX(underR.height + min, priv->overR.height);
}

static void
entangle_overlay_box_get_preferred_width(GtkWidget *widget,
                                         gint      *minimal_width,
                                         gint      *natural_width)
{
    GtkRequisition requisition;

    entangle_overlay_box_size_request(widget, &requisition);

    *minimal_width = *natural_width = requisition.width;
}

static void
entangle_overlay_box_get_preferred_height(GtkWidget *widget,
                                          gint      *minimal_height,
                                          gint      *natural_height)
{
    GtkRequisition requisition;

    entangle_overlay_box_size_request(widget, &requisition);

    *minimal_height = *natural_height = requisition.height;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_size_allocate --
 *
 *      "size_allocate" method of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_size_allocate(GtkWidget *widget,
                                   GtkAllocation *allocation)
{
    EntangleOverlayBox *box;
    EntangleOverlayBoxPrivate *priv;
    GtkAllocation under;
    GtkAllocation over;

    gtk_widget_set_allocation (widget, allocation);

    box = ENTANGLE_OVERLAY_BOX(widget);
    priv = box->priv;

    entangle_overlay_box_get_under_geometry(box, &under.x, &under.y, &under.width,
                                            &under.height);
    entangle_overlay_box_get_over_geometry(box, &over.x, &over.y, &over.width, &over.height);
    
    if (gtk_widget_get_realized(widget)) {
        gdk_window_move_resize(gtk_widget_get_window(widget),
                               allocation->x, allocation->y,
                               allocation->width, allocation->height);
        gdk_window_move_resize(priv->underWin, under.x, under.y, under.width,
                               under.height);
        gdk_window_move_resize(priv->overWin, over.x, over.y, over.width,
                               over.height);
    }

    under.x = 0;
    under.y = 0;
    gtk_widget_size_allocate(priv->under, &under);
    over.x = 0;
    over.y = 0;
    gtk_widget_size_allocate(priv->over, &over);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_style_set --
 *
 *      "style_set" method of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_style_set(GtkWidget *widget,
                               GtkStyle *previousStyle)
{
    EntangleOverlayBox *box;

    box = ENTANGLE_OVERLAY_BOX(widget);

    if (gtk_widget_get_realized(widget)) {
        entangle_overlay_box_set_background(box);
    }

    GTK_WIDGET_CLASS(entangle_overlay_box_parent_class)->style_set(widget, previousStyle);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_set_child --
 *
 *      Set a child of a EntangleOverlayBox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_set_child(EntangleOverlayBox *box,
                               GtkWidget **child,
                               GdkWindow *childWin,
                               GtkWidget *widget)
{
    GtkWidget *oldChild = *child;

    if (oldChild) {
        g_object_ref(oldChild);
        gtk_container_remove(GTK_CONTAINER(box), oldChild);
    }

    *child = widget;
    if (*child) {
        gtk_widget_set_parent_window(widget, childWin);
        gtk_container_add(GTK_CONTAINER(box), *child);
    }

    if (oldChild) {
        g_object_unref(oldChild);
    }
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_overlay_box_set_over_default --
 *
 *      Base implementation of entangle_overlay_box_set_over.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_overlay_box_set_over_default(EntangleOverlayBox *box,
                                      GtkWidget *widget)
{
    entangle_overlay_box_set_child(box, &box->priv->over, box->priv->overWin, widget);
}

static void
entangle_overlay_box_class_init(EntangleOverlayBoxClass *klass)
{
    GObjectClass *objectClass;
    GtkWidgetClass *widgetClass;

    objectClass = G_OBJECT_CLASS(klass);
    widgetClass = GTK_WIDGET_CLASS(klass);

    widgetClass->map = entangle_overlay_box_map;
    widgetClass->unmap = entangle_overlay_box_unmap;
    widgetClass->realize = entangle_overlay_box_realize;
    widgetClass->unrealize = entangle_overlay_box_unrealize;
    widgetClass->get_preferred_width = entangle_overlay_box_get_preferred_width;
    widgetClass->get_preferred_height = entangle_overlay_box_get_preferred_height;
    widgetClass->size_allocate = entangle_overlay_box_size_allocate;
    widgetClass->style_set = entangle_overlay_box_style_set;

    klass->set_over = entangle_overlay_box_set_over_default;

    g_type_class_add_private(objectClass, sizeof(EntangleOverlayBoxPrivate));
}


/**
 * entangle_overlay_box_new:
 *
 * Create a new EntangleOverlayBox GTK+ widget.
 */
EntangleOverlayBox *
entangle_overlay_box_new(void)
{
    return ENTANGLE_OVERLAY_BOX(g_object_new(ENTANGLE_TYPE_OVERLAY_BOX, NULL));
}


/**
 * entangle_overlay_box_set_under:
 *
 * Set the under widget of a EntangleOverlayBox.
 */
void
entangle_overlay_box_set_under(EntangleOverlayBox *box,
                               GtkWidget *widget)
{
    g_return_if_fail(box != NULL);

    entangle_overlay_box_set_child(box, &box->priv->under, box->priv->underWin, widget);
}


/**
 * entangle_overlay_box_set_over:
 *
 * Set the over widget of a EntangleOverlayBox.
 */
void
entangle_overlay_box_set_over(EntangleOverlayBox *box,
                              GtkWidget *widget)
{
    g_return_if_fail(box != NULL);

    ENTANGLE_OVERLAY_BOX_GET_CLASS(box)->set_over(box, widget);
}


/**
 * entangle_overlay_box_set_min:
 *
 * Set the 'min' property of a EntangleOverlayBox, i.e. the number of pixel of the
 * 'over' child that should always be displayed without overlapping on the
 * 'under' child.
 *
 * Using a value of -1 displays the 'over' child entirely.
 */
void
entangle_overlay_box_set_min(EntangleOverlayBox *box,
                             unsigned int min)
{
    g_return_if_fail(box != NULL);

    box->priv->min = min;
    gtk_widget_queue_resize(GTK_WIDGET(box));
}


/**
 * entangle_overlay_box_set_fraction:
 *
 * Set the 'fraction' property of a EntangleOverlayBox, i.e. how much of the 'over'
 * child should overlap on the 'under' child.
 */
void
entangle_overlay_box_set_fraction(EntangleOverlayBox *box,
                                  double fraction)
{
    g_return_if_fail(box != NULL);
    g_return_if_fail(fraction >=0 && fraction <= 1);

    box->priv->fraction = fraction;
    if (gtk_widget_get_realized(GTK_WIDGET (box))) {
        int x;
        int y;
        int width;
        int height;

        entangle_overlay_box_get_over_geometry(box, &x, &y, &width, &height);
        gdk_window_move(box->priv->overWin, x, y);
    }
}


/**
 * entangle_overlay_box_get_fraction:
 *
 * Retrieve the 'fraction' property of a EntangleOverlayBox.
 */
double
entangle_overlay_box_get_fraction(EntangleOverlayBox *box)
{
    g_return_val_if_fail(box != NULL, 0);

    return box->priv->fraction;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
