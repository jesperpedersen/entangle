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

#ifndef __CAPA_CONTROL_H__
#define __CAPA_CONTROL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL            (capa_control_get_type ())
#define CAPA_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL, CapaControl))
#define CAPA_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL, CapaControlClass))
#define CAPA_IS_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL))
#define CAPA_IS_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL))
#define CAPA_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL, CapaControlClass))


typedef struct _CapaControl CapaControl;
typedef struct _CapaControlPrivate CapaControlPrivate;
typedef struct _CapaControlClass CapaControlClass;

struct _CapaControl
{
  GObject parent;

  CapaControlPrivate *priv;
};

struct _CapaControlClass
{
  GObjectClass parent_class;
};


GType capa_control_get_type(void) G_GNUC_CONST;
CapaControl* capa_control_new(const char *path,
			      int id,
			      const char *label,
			      const char *info);


int capa_control_id(CapaControl *control);
const char *capa_control_label(CapaControl *control);
const char *capa_control_info(CapaControl *control);

G_END_DECLS

#endif /* __CAPA_CONTROL_H__ */

