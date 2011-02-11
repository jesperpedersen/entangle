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

#ifndef __ENTANGLE_IMAGE_POPUP_H__
#define __ENTANGLE_IMAGE_POPUP_H__

#include <glib-object.h>

#include "entangle-image.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_IMAGE_POPUP            (entangle_image_popup_get_type ())
#define ENTANGLE_IMAGE_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_IMAGE_POPUP, EntangleImagePopup))
#define ENTANGLE_IMAGE_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_IMAGE_POPUP, EntangleImagePopupClass))
#define ENTANGLE_IS_IMAGE_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_IMAGE_POPUP))
#define ENTANGLE_IS_IMAGE_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_IMAGE_POPUP))
#define ENTANGLE_IMAGE_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_IMAGE_POPUP, EntangleImagePopupClass))


typedef struct _EntangleImagePopup EntangleImagePopup;
typedef struct _EntangleImagePopupPrivate EntangleImagePopupPrivate;
typedef struct _EntangleImagePopupClass EntangleImagePopupClass;

struct _EntangleImagePopup
{
    GObject parent;

    EntangleImagePopupPrivate *priv;
};

struct _EntangleImagePopupClass
{
    GObjectClass parent_class;

    void (*popup_close)(EntangleImagePopup *popup);
};


GType entangle_image_popup_get_type(void) G_GNUC_CONST;

EntangleImagePopup* entangle_image_popup_new(void);

void entangle_image_popup_show(EntangleImagePopup *popup,
                               GtkWindow *parent,
                               int x, int y);
void entangle_image_popup_move_to_monitor(EntangleImagePopup *popup, gint monitor);
void entangle_image_popup_show_on_monitor(EntangleImagePopup *popup, gint monitor);
void entangle_image_popup_hide(EntangleImagePopup *popup);

void entangle_image_popup_set_image(EntangleImagePopup *popup, EntangleImage *image);
EntangleImage *entangle_image_popup_get_image(EntangleImagePopup *popup);

G_END_DECLS

#endif /* __ENTANGLE_IMAGE_POPUP_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
