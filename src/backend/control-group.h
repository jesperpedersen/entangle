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

#ifndef __CONTROL_GROUP__
#define __CONTROL_GROUP__

#include <glib-object.h>

#include "control.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL_GROUP            (capa_control_group_get_type ())
#define CAPA_CONTROL_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL_GROUP, CapaControlGroup))
#define CAPA_CONTROL_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL_GROUP, CapaControlGroupClass))
#define CAPA_IS_CONTROL_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL_GROUP))
#define CAPA_IS_CONTROL_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL_GROUP))
#define CAPA_CONTROL_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL_GROUP, CapaControlGroupClass))


typedef struct _CapaControlGroup CapaControlGroup;
typedef struct _CapaControlGroupPrivate CapaControlGroupPrivate;
typedef struct _CapaControlGroupClass CapaControlGroupClass;

struct _CapaControlGroup
{
  CapaControl parent;

  CapaControlGroupPrivate *priv;
};

struct _CapaControlGroupClass
{
  CapaControlClass parent_class;
};


GType capa_control_group_get_type(void) G_GNUC_CONST;
CapaControlGroup* capa_control_group_new(const char *path,
					 int id,
					 const char *label);

void capa_control_group_add(CapaControlGroup *group,
			    CapaControl *control);

int capa_control_group_count(CapaControlGroup *group);
CapaControl *capa_control_group_get(CapaControlGroup *group, int idx);


G_END_DECLS

#endif /* __CONTROL_GROUP__ */

