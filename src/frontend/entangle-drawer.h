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

#ifndef __ENTANGLE_DRAWER_H__
#define __ENTANGLE_DRAWER_H__

#include "entangle-overlay-box.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_DRAWER            (entangle_drawer_get_type())
#define ENTANGLE_DRAWER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), ENTANGLE_TYPE_DRAWER, EntangleDrawer))
#define ENTANGLE_DRAWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), ENTANGLE_TYPE_DRAWER, EntangleDrawerClass))
#define ENTANGLE_IS_DRAWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), ENTANGLE_TYPE_DRAWER))
#define ENTANGLE_IS_DRAWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), ENTANGLE_TYPE_DRAWER))
#define ENTANGLE_DRAWER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), ENTANGLE_TYPE_DRAWER, EntangleDrawerClass))

typedef struct _EntangleDrawer EntangleDrawer;
typedef struct _EntangleDrawerPrivate EntangleDrawerPrivate;
typedef struct _EntangleDrawerClass EntangleDrawerClass;

struct _EntangleDrawer {
    EntangleOverlayBox parent;

    EntangleDrawerPrivate *priv;
};


struct _EntangleDrawerClass {
    EntangleOverlayBoxClass parent;
};


GType entangle_drawer_get_type(void);

EntangleDrawer *entangle_drawer_new(void);

void entangle_drawer_set_speed(EntangleDrawer *drawer, guint period, gdouble step);
void entangle_drawer_set_goal(EntangleDrawer *drawer, gdouble fraction);
int entangle_drawer_get_close_time(EntangleDrawer *drawer);


G_END_DECLS


#endif /* __ENTANGLE_DRAWER_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
