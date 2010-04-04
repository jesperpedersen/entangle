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

#ifndef __ENTANGLE_CONTROL_GROUP_H__
#define __ENTANGLE_CONTROL_GROUP_H__

#include <glib-object.h>

#include "entangle-control.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONTROL_GROUP            (entangle_control_group_get_type ())
#define ENTANGLE_CONTROL_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONTROL_GROUP, EntangleControlGroup))
#define ENTANGLE_CONTROL_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONTROL_GROUP, EntangleControlGroupClass))
#define ENTANGLE_IS_CONTROL_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONTROL_GROUP))
#define ENTANGLE_IS_CONTROL_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONTROL_GROUP))
#define ENTANGLE_CONTROL_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONTROL_GROUP, EntangleControlGroupClass))


typedef struct _EntangleControlGroup EntangleControlGroup;
typedef struct _EntangleControlGroupPrivate EntangleControlGroupPrivate;
typedef struct _EntangleControlGroupClass EntangleControlGroupClass;

struct _EntangleControlGroup
{
    EntangleControl parent;

    EntangleControlGroupPrivate *priv;
};

struct _EntangleControlGroupClass
{
    EntangleControlClass parent_class;
};


GType entangle_control_group_get_type(void) G_GNUC_CONST;
EntangleControlGroup* entangle_control_group_new(const char *path,
                                                 int id,
                                                 const char *label,
                                                 const char *info,
                                                 gboolean readonly);

void entangle_control_group_add(EntangleControlGroup *group,
                            EntangleControl *control);

int entangle_control_group_count(EntangleControlGroup *group);
EntangleControl *entangle_control_group_get(EntangleControlGroup *group, int idx);


G_END_DECLS

#endif /* __ENTANGLE_CONTROL_GROUP_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
