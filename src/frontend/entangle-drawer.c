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
 * Formerly: drawer.c -
 *
 *      Implementation of a GTK+ drawer, i.e. a widget that opens and closes by
 *      sliding smoothly, at constant speed, over another one.
 */

#include <config.h>

#include <math.h>
#include "entangle-drawer.h"


struct _EntangleDrawerPrivate
{
    unsigned int period;
    double step;
    double goal;
    struct {
        gboolean pending;
        guint id;
    } timer;
};

#define ENTANGLE_DRAWER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_DRAWER, EntangleDrawerPrivate))

G_DEFINE_TYPE(EntangleDrawer, entangle_drawer, ENTANGLE_TYPE_OVERLAY_BOX);

static void
entangle_drawer_init(EntangleDrawer *drawer)
{
    EntangleDrawerPrivate *priv;

    priv = drawer->priv = ENTANGLE_DRAWER_GET_PRIVATE(drawer);

    priv->period = 10;
    priv->step = 0.2;
    priv->timer.pending = FALSE;
}


static void
entangle_drawer_finalize(GObject *object)
{
    EntangleDrawer *drawer;
    EntangleDrawerPrivate *priv;

    drawer = ENTANGLE_DRAWER(object);
    priv = drawer->priv;

    if (priv->timer.pending) {
        g_source_remove(priv->timer.id);
        priv->timer.pending = FALSE;
    }

    G_OBJECT_CLASS(entangle_drawer_parent_class)->finalize(object);
}


static void
entangle_drawer_class_init(EntangleDrawerClass *klass)
{
    GObjectClass *objectClass = G_OBJECT_CLASS(klass);

    objectClass->finalize = entangle_drawer_finalize;

    g_type_class_add_private(objectClass, sizeof(EntangleDrawerPrivate));
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_drawer_new --
 *
 *      Create a new EntangleDrawer GTK+ widget.
 *
 * Results:
 *      The widget
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

EntangleDrawer *
entangle_drawer_new(void)
{
    return ENTANGLE_DRAWER(g_object_new(ENTANGLE_TYPE_DRAWER, NULL));
}


/*
 *-----------------------------------------------------------------------------
 *
 * EntangleDrawerOnTimer --
 *
 *      Timer callback of a EntangleDrawer. If we have reached the goal, deschedule
 *      the timer. Otherwise make progress towards the goal, and keep the timer
 *      scheduled.
 *
 * Results:
 *      TRUE if the timer must be rescheduled.
 *      FALSE if the timer must not be rescheduled.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

static gint
entangle_drawer_on_timer(gpointer data)
{
    EntangleDrawer *drawer;
    EntangleDrawerPrivate *priv;
    double fraction;

    drawer = ENTANGLE_DRAWER(data);
    priv = drawer->priv;

    fraction = entangle_overlay_box_get_fraction(ENTANGLE_OVERLAY_BOX(drawer));
    if (fabs(priv->goal - fraction) < 0.00001) {
        return priv->timer.pending = FALSE;
    }

    entangle_overlay_box_set_fraction(ENTANGLE_OVERLAY_BOX(drawer),
                                      priv->goal > fraction
                                      ? MIN(fraction + priv->step, priv->goal)
                                      : MAX(fraction - priv->step, priv->goal));
    return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_drawer_set_speed --
 *
 *      Set the 'period' (in ms.) and 'step' properties of a EntangleDrawer, which
 *      determine the speed and smoothness of the drawer's motion.
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
entangle_drawer_set_speed(EntangleDrawer *drawer,
                          unsigned int period,
                          double step)
{
    EntangleDrawerPrivate *priv;

    g_return_if_fail(drawer != NULL);

    priv = drawer->priv;

    priv->period = period;
    if (priv->timer.pending) {
        g_source_remove(priv->timer.id);
        priv->timer.id = g_timeout_add(priv->period, entangle_drawer_on_timer, drawer);
    }
    priv->step = step;
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_drawer_set_goal --
 *
 *      Set the 'goal' property of a EntangleDrawer, i.e. how much the drawer should
 *      be opened when it is done sliding.
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
entangle_drawer_set_goal(EntangleDrawer *drawer,
                         double goal)
{
    EntangleDrawerPrivate *priv;

    g_return_if_fail(drawer != NULL);
    g_return_if_fail(goal >= 0 && goal <= 1);

    priv = drawer->priv;

    priv->goal = goal;
    if (priv->timer.pending == FALSE) {
        priv->timer.id = g_timeout_add(priv->period, entangle_drawer_on_timer, drawer);
        priv->timer.pending = TRUE;
    }
}


/*
 *-----------------------------------------------------------------------------
 *
 * entangle_drawer_get_close_time --
 *
 *    Get the approximate amount of time it will take for this drawer to
 *    open and close, in ms.
 *
 * Results:
 *      The time it takes to open or close the drawer.
 *
 * Side effects:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

int
entangle_drawer_get_close_time(EntangleDrawer *drawer)
{
    EntangleDrawerPrivate *priv;

    if (drawer == NULL) {
        return 0;
    }

    priv = drawer->priv;

    return priv->period * ((int)(1/priv->step) + 1);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
