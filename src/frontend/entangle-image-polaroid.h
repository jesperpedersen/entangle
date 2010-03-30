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

#ifndef __ENTANGLE_IMAGE_POLAROID_H__
#define __ENTANGLE_IMAGE_POLAROID_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_IMAGE_POLAROID            (entangle_image_polaroid_get_type ())
#define ENTANGLE_IMAGE_POLAROID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_IMAGE_POLAROID, EntangleImagePolaroid))
#define ENTANGLE_IMAGE_POLAROID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_IMAGE_POLAROID, EntangleImagePolaroidClass))
#define ENTANGLE_IS_IMAGE_POLAROID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_IMAGE_POLAROID))
#define ENTANGLE_IS_IMAGE_POLAROID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_IMAGE_POLAROID))
#define ENTANGLE_IMAGE_POLAROID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_IMAGE_POLAROID, EntangleImagePolaroidClass))


typedef struct _EntangleImagePolaroid EntangleImagePolaroid;
typedef struct _EntangleImagePolaroidPrivate EntangleImagePolaroidPrivate;
typedef struct _EntangleImagePolaroidClass EntangleImagePolaroidClass;

struct _EntangleImagePolaroid
{
    GObject parent;

    EntangleImagePolaroidPrivate *priv;
};

struct _EntangleImagePolaroidClass
{
    GObjectClass parent_class;

    void (*polaroid_close)(EntangleImagePolaroid *polaroid);
};


GType entangle_image_polaroid_get_type(void) G_GNUC_CONST;

EntangleImagePolaroid* entangle_image_polaroid_new(void);

void entangle_image_polaroid_show(EntangleImagePolaroid *polaroid,
                              GtkWindow *parent,
                              int x, int y);
void entangle_image_polaroid_hide(EntangleImagePolaroid *polaroid);

G_END_DECLS

#endif /* __ENTANGLE_IMAGE_POLAROID_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
