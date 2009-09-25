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

#ifndef __CONTROL_LAYOUT__
#define __CONTROL_LAYOUT__

#include <glib-object.h>

#include "control-group.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL_LAYOUT            (capa_control_layout_get_type ())
#define CAPA_CONTROL_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL_LAYOUT, CapaControlLayout))
#define CAPA_CONTROL_LAYOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL_LAYOUT, CapaControlLayoutClass))
#define CAPA_IS_CONTROL_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL_LAYOUT))
#define CAPA_IS_CONTROL_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL_LAYOUT))
#define CAPA_CONTROL_LAYOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL_LAYOUT, CapaControlLayoutClass))


typedef struct _CapaControlLayout CapaControlLayout;
typedef struct _CapaControlLayoutPrivate CapaControlLayoutPrivate;
typedef struct _CapaControlLayoutClass CapaControlLayoutClass;

struct _CapaControlLayout
{
  GObject parent;

  CapaControlLayoutPrivate *priv;
};

struct _CapaControlLayoutClass
{
  GObjectClass parent_class;
};


GType capa_control_layout_get_type(void) G_GNUC_CONST;
CapaControlLayout* capa_control_layout_new(void);

void capa_control_layout_add_group(CapaControlLayout *layout,
				   CapaControlGroup *group);

G_END_DECLS

#endif /* __CONTROL_LAYOUT__ */

