/*
 *  Capa: Capa Assists Photograph Aquisition
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

#ifndef __CAPA_IMAGE_POLAROID_H__
#define __CAPA_IMAGE_POLAROID_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_IMAGE_POLAROID            (capa_image_polaroid_get_type ())
#define CAPA_IMAGE_POLAROID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_IMAGE_POLAROID, CapaImagePolaroid))
#define CAPA_IMAGE_POLAROID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_IMAGE_POLAROID, CapaImagePolaroidClass))
#define CAPA_IS_IMAGE_POLAROID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_IMAGE_POLAROID))
#define CAPA_IS_IMAGE_POLAROID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_IMAGE_POLAROID))
#define CAPA_IMAGE_POLAROID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_IMAGE_POLAROID, CapaImagePolaroidClass))


typedef struct _CapaImagePolaroid CapaImagePolaroid;
typedef struct _CapaImagePolaroidPrivate CapaImagePolaroidPrivate;
typedef struct _CapaImagePolaroidClass CapaImagePolaroidClass;

struct _CapaImagePolaroid
{
  GObject parent;

  CapaImagePolaroidPrivate *priv;
};

struct _CapaImagePolaroidClass
{
  GObjectClass parent_class;

  void (*polaroid_close)(CapaImagePolaroid *polaroid);
};


GType capa_image_polaroid_get_type(void) G_GNUC_CONST;

CapaImagePolaroid* capa_image_polaroid_new(void);

void capa_image_polaroid_show(CapaImagePolaroid *polaroid,
			      GtkWindow *parent,
			      int x, int y);
void capa_image_polaroid_hide(CapaImagePolaroid *polaroid);

G_END_DECLS

#endif /* __CAPA_IMAGE_POLAROID_H__ */

