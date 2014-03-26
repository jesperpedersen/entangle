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
 * Formerly autoDrawer.c -
 *
 *    Subclass of ViewDrawer that encapsulates the behaviour typically required
 *    when using the drawer to implement a menu/toolbar that auto-opens when
 *    moused-over and auto-closes when the mouse leaves.
 */

#include <config.h>

#include "entangle-auto-drawer.h"


struct _EntangleAutoDrawerPrivate
{
    gboolean active;
    gboolean pinned;
    gboolean inputUngrabbed;

    gboolean opened;
    gboolean forceClosing;

    gboolean fill;
    gint offset;

    guint closeConnection;
    guint delayConnection;
    guint delayValue;
    guint overlapPixels;
    guint noOverlapPixels;

    GtkWidget *over;
    GtkWidget *evBox;
};

#define ENTANGLE_AUTO_DRAWER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_AUTO_DRAWER, EntangleAutoDrawerPrivate))

G_DEFINE_TYPE(EntangleAutoDrawer, entangle_auto_drawer, ENTANGLE_TYPE_DRAWER);

/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_enforce
 *
 *      Enforce an AutoDrawer's goal now.
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
entangle_auto_drawer_enforce(EntangleAutoDrawer *drawer,
                             gboolean animate)
{
    double fraction;
    GtkAllocation allocation;
    EntangleAutoDrawerPrivate *priv = drawer->priv;

    if (!priv->active) {
        entangle_overlay_box_set_min(ENTANGLE_OVERLAY_BOX(drawer), -1);
        entangle_overlay_box_set_fraction(ENTANGLE_OVERLAY_BOX(drawer), 0);
        return;
    }

    g_assert(priv->over != NULL);
    g_assert(GTK_IS_WIDGET(priv->over));

    entangle_overlay_box_set_min(ENTANGLE_OVERLAY_BOX(drawer), priv->noOverlapPixels);

    // The forceClosing flag overrides the opened flag.
    if (priv->opened && !priv->forceClosing) {
        fraction = 1;
    } else {
        gtk_widget_get_allocation(priv->over, &allocation);
        fraction = ((double)priv->overlapPixels / allocation.height);
    }

    if (!animate) {
        entangle_overlay_box_set_fraction(ENTANGLE_OVERLAY_BOX(drawer), fraction);
    }
    entangle_drawer_set_goal(ENTANGLE_DRAWER(drawer), fraction);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_on_enforce_delay --
 *
 *      Callback fired when a delayed update happens to update the drawer state.
 *
 * Results:
 *      FALSE to indicate timer should not repeat.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static gboolean
entangle_auto_drawer_on_enforce_delay(EntangleAutoDrawer *drawer)
{
    drawer->priv->delayConnection = 0;
    entangle_auto_drawer_enforce(drawer, TRUE);

    return FALSE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_on_close_delay --
 *
 *      Callback fired when the drawer is closed manually. This prevents the
 *      drawer from reopening right away.
 *
 * Results:
 *      FALSE to indicate timer should not repeat.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static gboolean
entangle_auto_drawer_on_close_delay(EntangleAutoDrawer *drawer)
{
    drawer->priv->closeConnection = 0;
    drawer->priv->forceClosing = FALSE;

    return FALSE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_update --
 *
 *      Decide whether an AutoDrawer should be opened or closed, and enforce
 *      drawer decision now or later.
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
entangle_auto_drawer_update(EntangleAutoDrawer *drawer,
                            gboolean immediate)
{
    EntangleAutoDrawerPrivate *priv = drawer->priv;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(drawer));
    GtkWindow *window;
    GtkAllocation allocation;

    if (!toplevel || !gtk_widget_is_toplevel(toplevel)) {
        // The autoDrawer cannot function properly without a toplevel.
        return;
    }
    window = GTK_WINDOW(toplevel);

    /*
     * We decide to open the drawer by OR'ing several conditions. Evaluating a
     * condition can have the side-effect of setting 'immediate' to TRUE, so we
     * cannot stop evaluating the conditions after we have found one to be TRUE.
     */

    priv->opened = FALSE;

    /* Is the AutoDrawer pinned? */

    if (priv->pinned) {
        immediate = TRUE;

        priv->opened = TRUE;
    }

    /* Is the mouse cursor inside the event box? */

    if (gtk_widget_get_window(priv->evBox)) {
        int x;
        int y;
        GdkDevice *dev;
        GdkDeviceManager *devmgr;

        devmgr = gdk_display_get_device_manager(gtk_widget_get_display(priv->evBox));
        dev = gdk_device_manager_get_client_pointer(devmgr);

        gdk_window_get_device_position(gtk_widget_get_window(priv->evBox),
                                       dev, &x, &y, NULL);

        gtk_widget_get_allocation(priv->evBox, &allocation);
        g_assert(gtk_container_get_border_width(GTK_CONTAINER(priv->evBox))
                 == 0);
        if ((guint)x < (guint)allocation.width &&
            (guint)y < (guint)allocation.height) {
            priv->opened = TRUE;
        }
    }

    /* If there is a focused widget, is it inside the event box? */

    {
        GtkWidget *focus;

        focus = gtk_window_get_focus(window);
        if (focus && gtk_widget_is_ancestor(focus, priv->evBox)) {
            /*
             * Override the default 'immediate' to make sure the 'over' widget
             * immediately appears along with the widget the focused widget.
             */
            immediate = TRUE;

            priv->opened = TRUE;
        }
    }

    /* If input is grabbed, is it on behalf of a widget inside the event box? */

    if (!priv->inputUngrabbed) {
        GtkWidget *grabbed = NULL;

        if (gtk_window_has_group(window)) {
            GtkWindowGroup *group = gtk_window_get_group(window);
            grabbed = gtk_window_group_get_current_grab(group);
        }
        if (!grabbed) {
            grabbed = gtk_grab_get_current();
        }

        if (grabbed && GTK_IS_MENU(grabbed)) {
            /*
             * With cascading menus, the deepest menu owns the grab. Traverse the
             * menu hierarchy up until we reach the attach widget for the whole
             * hierarchy.
             */

            for (;;) {
                GtkWidget *menuAttach;
                GtkWidget *menuItemParent;

                menuAttach = gtk_menu_get_attach_widget(GTK_MENU(grabbed));
                if (!menuAttach) {
                    /*
                     * It is unfortunately not mandatory for a menu to have a proper
                     * attach widget set.
                     */
                    break;
                }

                grabbed = menuAttach;
                if (!GTK_IS_MENU_ITEM(grabbed)) {
                    break;
                }

                menuItemParent = gtk_widget_get_parent(grabbed);
                g_return_if_fail(menuItemParent);
                if (!GTK_IS_MENU(menuItemParent)) {
                    break;
                }

                grabbed = menuItemParent;
            }
        }

        if (grabbed && gtk_widget_is_ancestor(grabbed, priv->evBox)) {
            /*
             * Override the default 'immediate' to make sure the 'over' widget
             * immediately appears along with the widget the grab happens on
             * behalf of.
             */
            immediate = TRUE;

            priv->opened = TRUE;
        }
    }

    if (priv->delayConnection) {
        g_source_remove(priv->delayConnection);
    }

    if (priv->forceClosing) {
        entangle_auto_drawer_enforce(drawer, TRUE);
    } else if (immediate) {
        entangle_auto_drawer_enforce(drawer, FALSE);
    } else {
        priv->delayConnection = g_timeout_add(priv->delayValue,
                                              (GSourceFunc)entangle_auto_drawer_on_enforce_delay, drawer);
    }
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_on_over_enter_leave --
 *
 *      Respond to enter/leave events by doing a delayed update of the drawer
 *      state.
 *
 * Results:
 *      FALSE to indicate event was not handled.
 *
 * Side effects:
 *      Will queue delayed update.
 *
 *-----------------------------------------------------------------------------
 */

static gboolean
entangle_auto_drawer_on_over_enter_leave(GtkWidget *evBox G_GNUC_UNUSED,
                                         GdkEventCrossing *event G_GNUC_UNUSED,
                                         EntangleAutoDrawer *drawer)
{
    /*
     * This change happens in response to user input. By default, give the user
     * some time to correct his input before reacting to the change.
     */
    entangle_auto_drawer_update(drawer, FALSE);

    return FALSE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_OnGrabNotify --
 *
 *      Respond to grab notifications by updating the drawer state.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Might queue delayed update.
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_auto_drawer_on_grab_notify(GtkWidget *evBox G_GNUC_UNUSED,
                                    gboolean ungrabbed,
                                    EntangleAutoDrawer *drawer)
{
    EntangleAutoDrawerPrivate *priv = drawer->priv;

    priv->inputUngrabbed = ungrabbed;

    /*
     * This change happens in response to user input. By default, give the user
     * some time to correct his input before reacting to the change.
     */
    entangle_auto_drawer_update(drawer, FALSE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_on_set_focus
 *
 *      Respond to changes in the focus widget of the autoDrawer's toplevel
 *      by recalculating the state.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_auto_drawer_on_set_focus(GtkWindow *window G_GNUC_UNUSED,
                                  GtkWidget *widget G_GNUC_UNUSED,
                                  EntangleAutoDrawer *drawer)
{
    /*
     * This change happens in response to user input. By default, give the user
     * some time to correct his input before reacting to the change.
     */
    entangle_auto_drawer_update(drawer, FALSE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_on_hierarchy_changed --
 *
 *      Respond to changes in the toplevel for the AutoDrawer. A toplevel is
 *      required for the AutoDrawer to calculate its state.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_auto_drawer_on_hierarchy_changed(EntangleAutoDrawer *drawer,
                                          GtkWidget *oldToplevel)
{
    GtkWidget *newToplevel = gtk_widget_get_toplevel(GTK_WIDGET(drawer));

    if (oldToplevel && gtk_widget_is_toplevel(oldToplevel)) {
        g_signal_handlers_disconnect_by_func(oldToplevel,
                                             G_CALLBACK(entangle_auto_drawer_on_set_focus),
                                             drawer);
    }

    if (newToplevel && gtk_widget_is_toplevel(newToplevel)) {
        g_signal_connect_after(newToplevel, "set-focus",
                               G_CALLBACK(entangle_auto_drawer_on_set_focus), drawer);
    }

    /* This change happens programmatically. Always react to it immediately. */
    entangle_auto_drawer_update(drawer, TRUE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_over --
 *
 *      Virtual method override so drawer the user's over widget is placed
 *      inside the AutoDrawer's event box.
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
entangle_auto_drawer_set_over(EntangleOverlayBox *ovBox,
                              GtkWidget *widget)
{
    EntangleAutoDrawer *drawer = ENTANGLE_AUTO_DRAWER(ovBox);
    EntangleAutoDrawerPrivate *priv = drawer->priv;
    GtkWidget *oldChild = gtk_bin_get_child(GTK_BIN(priv->evBox));

    if (oldChild) {
        g_object_ref(oldChild);
        gtk_container_remove(GTK_CONTAINER(priv->evBox), oldChild);
    }

    if (widget) {
        gtk_container_add(GTK_CONTAINER(priv->evBox), widget);
    }

    if (oldChild) {
        g_object_unref(oldChild);
    }

    priv->over = widget;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_refresh_packing --
 *
 *      Sets the actual packing values for fill, expand, and packing
 *      given internal settings.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

static void
entangle_auto_drawer_refresh_packing(EntangleAutoDrawer *drawer)
{
    gboolean expand;
    gboolean fill;
    guint padding;

    expand = (drawer->priv->fill || (drawer->priv->offset < 0));
    fill = drawer->priv->fill;
    padding = (expand || fill) ? 0 : drawer->priv->offset;

    gtk_box_set_child_packing(GTK_BOX(drawer), drawer->priv->evBox,
                              expand, fill, padding, GTK_PACK_START);
}


static void
entangle_auto_drawer_init(EntangleAutoDrawer *drawer)
{
    EntangleAutoDrawerPrivate *priv;

    priv = drawer->priv = ENTANGLE_AUTO_DRAWER_GET_PRIVATE(drawer);

    priv->active = TRUE;
    priv->pinned = FALSE;
    priv->forceClosing = FALSE;
    priv->inputUngrabbed = TRUE;
    priv->delayConnection = 0;
    priv->delayValue = 250;
    priv->overlapPixels = 0;
    priv->noOverlapPixels = 1;

    priv->fill = TRUE;
    priv->offset = -1;

    priv->evBox = gtk_event_box_new();

    gtk_widget_show(priv->evBox);
    ENTANGLE_OVERLAY_BOX_CLASS(entangle_auto_drawer_parent_class)->set_over(ENTANGLE_OVERLAY_BOX(drawer), priv->evBox);

    g_signal_connect(priv->evBox, "enter-notify-event",
                     G_CALLBACK(entangle_auto_drawer_on_over_enter_leave), drawer);
    g_signal_connect(priv->evBox, "leave-notify-event",
                     G_CALLBACK(entangle_auto_drawer_on_over_enter_leave), drawer);
    g_signal_connect(priv->evBox, "grab-notify",
                     G_CALLBACK(entangle_auto_drawer_on_grab_notify), drawer);

    g_signal_connect(drawer, "hierarchy-changed",
                     G_CALLBACK(entangle_auto_drawer_on_hierarchy_changed), NULL);

    /* This change happens programmatically. Always react to it immediately. */
    entangle_auto_drawer_update(drawer, TRUE);

    entangle_auto_drawer_refresh_packing(drawer);
}


static void
entangle_auto_drawer_finalize(GObject *object)
{
    EntangleAutoDrawer *drawer;

    drawer = ENTANGLE_AUTO_DRAWER(object);
    if (drawer->priv->delayConnection) {
        g_source_remove(drawer->priv->delayConnection);
    }

    G_OBJECT_CLASS(entangle_auto_drawer_parent_class)->finalize(object);
}

static void
entangle_auto_drawer_class_init(EntangleAutoDrawerClass *klass)
{
    GObjectClass *objectClass = G_OBJECT_CLASS(klass);
    EntangleOverlayBoxClass *ovBoxClass = ENTANGLE_OVERLAY_BOX_CLASS(klass);

    objectClass->finalize = entangle_auto_drawer_finalize;

    ovBoxClass->set_over = entangle_auto_drawer_set_over;

    g_type_class_add_private(objectClass, sizeof(EntangleAutoDrawerPrivate));
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_new --
 *
 *      Create a new EntangleAutoDrawer GTK+ widget.
 *
 * Results:
 *      The widget
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

EntangleAutoDrawer *
entangle_auto_drawer_new(void)
{
    return ENTANGLE_AUTO_DRAWER(g_object_new(ENTANGLE_TYPE_AUTO_DRAWER, NULL));
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_slide_delay --
 *
 *      Set the response time of an AutoDrawer in ms., i.e. the time drawer
 *      elapses between:
 *      - when the AutoDrawer notices a change drawer can impact the outcome of
 *        the decision to open or close the drawer,
 *      and
 *      - when the AutoDrawer makes such decision.
 *
 *      Users move the mouse inaccurately. If they temporarily move the mouse in
 *      or out of the AutoDrawer for less than the reponse time, their move will
 *      be ignored.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_set_slide_delay(EntangleAutoDrawer *drawer,
                                     guint delay)
{
    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));

    drawer->priv->delayValue = delay;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_overlap_pixels --
 *
 *      Set the number of pixels drawer the over widget overlaps the under widget
 *      when not open.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_set_overlap_pixels(EntangleAutoDrawer *drawer,
                                        guint overlapPixels)
{
    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));

    drawer->priv->overlapPixels = overlapPixels;

    /* This change happens programmatically. Always react to it immediately. */
    entangle_auto_drawer_update(drawer, TRUE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_no_overlap_pixels --
 *
 *      Set the number of pixels drawer the drawer reserves when not open. The
 *      over widget does not overlap the under widget over these pixels.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_set_no_overlap_pixels(EntangleAutoDrawer *drawer,
                                           guint noOverlapPixels)
{
    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));

    drawer->priv->noOverlapPixels = noOverlapPixels;

    /* This change happens programmatically. Always react to it immediately. */
    entangle_auto_drawer_update(drawer, TRUE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_active --
 *
 *      Set whether the AutoDrawer is active or not. Drawer is to say, whether
 *      it is acting as a drawer or not. When inactive, the over and under
 *      widget do not overlap and the net result is very much like a vbox.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_set_active(EntangleAutoDrawer *drawer,
                                gboolean active)
{
    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));

    drawer->priv->active = active;

    /* This change happens programmatically. Always react to it immediately. */
    entangle_auto_drawer_update(drawer, TRUE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_pinned --
 *
 *      Set whether the AutoDrawer is pinned or not. When pinned, the
 *      AutoDrawer will stay open regardless of the state of any other inputs.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_set_pinned(EntangleAutoDrawer *drawer,
                                gboolean pinned)
{
    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));

    drawer->priv->pinned = pinned;

    /*
     * This change happens in response to user input. By default, give the user
     * some time to correct his input before reacting to the change.
     */
    entangle_auto_drawer_update(drawer, FALSE);
}

gboolean
entangle_auto_drawer_get_pinned(EntangleAutoDrawer *drawer)
{
    g_return_val_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer), FALSE);

    return drawer->priv->pinned;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_fill --
 *
 *      Set whether the Over widget of the AutoDrawer should fill the full
 *      width of the AutoDrawer or just occupy the minimum space it needs.
 *      A value of TRUE overrides offset settings.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_set_fill(EntangleAutoDrawer *drawer,
                              gboolean fill)
{
    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));

    drawer->priv->fill = fill;
    entangle_auto_drawer_refresh_packing(drawer);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_set_offset --
 *
 *      Set the drawer's X offset, or distance in pixels from the left side.
 *      If offset is -1, the drawer will be centered.  If fill has been set
 *      TRUE by SetFill, these settings will have no effect.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated.
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_set_offset(EntangleAutoDrawer *drawer,
                                gint offset)
{
    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));

    drawer->priv->offset = offset;
    entangle_auto_drawer_refresh_packing(drawer);
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_auto_drawer_close --
 *
 *      Closes the drawer. This will not unset the pinned state.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      Drawer state is updated. If there is a focused widget inside the
 *      drawer, unfocus it.
 *
 *-----------------------------------------------------------------------------
 */

void
entangle_auto_drawer_close(EntangleAutoDrawer *drawer)
{
    GtkWindow *window;
    GtkWidget *focus;
    GtkWidget *toplevel;

    g_return_if_fail(ENTANGLE_IS_AUTO_DRAWER(drawer));
    toplevel = gtk_widget_get_toplevel(GTK_WIDGET(drawer));

    if (!toplevel || !gtk_widget_is_toplevel(toplevel)) {
        // The autoDrawer cannot function properly without a toplevel.
        return;
    }
    window = GTK_WINDOW(toplevel);

    focus = gtk_window_get_focus(window);
    if (focus && gtk_widget_is_ancestor(focus, drawer->priv->evBox)) {
        gtk_window_set_focus(window, NULL);
    }

    drawer->priv->forceClosing = TRUE;
    drawer->priv->closeConnection =
        g_timeout_add(entangle_drawer_get_close_time(&drawer->parent) +
                      drawer->priv->delayValue,
                      (GSourceFunc)entangle_auto_drawer_on_close_delay, drawer);

    /* This change happens programmatically. Always react to it immediately. */
    entangle_auto_drawer_update(drawer, TRUE);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
