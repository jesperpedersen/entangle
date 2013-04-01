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

#ifndef __ENTANGLE_PIXBUF_H__
#define __ENTANGLE_PIXBUF_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gexiv2.h>
#include "entangle-image.h"

G_BEGIN_DECLS

GdkPixbuf *entangle_pixbuf_auto_rotate(GdkPixbuf *src,
                                       GExiv2Metadata *metadata);

typedef enum {
  ENTANGLE_PIXBUF_IMAGE_SLOT_MASTER,
  ENTANGLE_PIXBUF_IMAGE_SLOT_PREVIEW,
  ENTANGLE_PIXBUF_IMAGE_SLOT_THUMBNAIL,
} EntanglePixbufImageSlot;

GdkPixbuf *entangle_pixbuf_open_image(EntangleImage *image,
                                      EntanglePixbufImageSlot slot,
                                      gboolean applyOrientation,
                                      GExiv2Metadata **metadata);

#endif /* __ENTANGLE_PIXBUF_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
