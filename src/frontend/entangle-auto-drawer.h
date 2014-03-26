/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (c) 2005 VMware Inc.
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

#ifndef __ENTANGLE_AUTO_DRAWER_H__
#define __ENTANGLE_AUTO_DRAWER_H__

#include "entangle-drawer.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_AUTO_DRAWER            (entangle_auto_drawer_get_type())
#define ENTANGLE_AUTO_DRAWER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), ENTANGLE_TYPE_AUTO_DRAWER, EntangleAutoDrawer))
#define ENTANGLE_AUTO_DRAWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), ENTANGLE_TYPE_AUTO_DRAWER, EntangleAutoDrawerClass))
#define ENTANGLE_IS_AUTO_DRAWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), ENTANGLE_TYPE_AUTO_DRAWER))
#define ENTANGLE_IS_AUTO_DRAWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), ENTANGLE_TYPE_AUTO_DRAWER))
#define ENTANGLE_AUTO_DRAWER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), ENTANGLE_TYPE_AUTO_DRAWER, EntangleAutoDrawerClass))

typedef struct _EntangleAutoDrawer EntangleAutoDrawer;
typedef struct _EntangleAutoDrawerPrivate EntangleAutoDrawerPrivate;
typedef struct _EntangleAutoDrawerClass EntangleAutoDrawerClass;

struct _EntangleAutoDrawer {
    EntangleDrawer parent;

    EntangleAutoDrawerPrivate *priv;
};


struct _EntangleAutoDrawerClass {
    EntangleDrawerClass parent;
};


GType entangle_auto_drawer_get_type(void);

EntangleAutoDrawer *entangle_auto_drawer_new(void);

void
entangle_auto_drawer_set_slide_delay(EntangleAutoDrawer *drawer,
                                     guint delay);
void
entangle_auto_drawer_set_overlap_pixels(EntangleAutoDrawer *drawer,
                                        guint overlapPixels);
void
entangle_auto_drawer_set_no_overlap_pixels(EntangleAutoDrawer *drawer,
                                           guint noOverlapPixels);

void
entangle_auto_drawer_set_active(EntangleAutoDrawer *drawer,
                                gboolean active);

void
entangle_auto_drawer_set_pinned(EntangleAutoDrawer *drawer,
                                gboolean pinned);
gboolean
entangle_auto_drawer_get_pinned(EntangleAutoDrawer *drawer);

void
entangle_auto_drawer_set_fill(EntangleAutoDrawer *drawer,
                              gboolean fill);

void
entangle_auto_drawer_set_offset(EntangleAutoDrawer *drawer,
                                gint offset);

void
entangle_auto_drawer_close(EntangleAutoDrawer *drawer);

G_END_DECLS


#endif /* LIBENTANGLE_AUTO_DRAWER_H */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
