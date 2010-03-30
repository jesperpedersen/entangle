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

#include <config.h>

#include <stdio.h>

#include "entangle-debug.h"
#include "entangle-control-group.h"

#define ENTANGLE_CONTROL_GROUP_GET_PRIVATE(obj)                             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_GROUP, EntangleControlGroupPrivate))

struct _EntangleControlGroupPrivate {
    size_t ncontrol;
    EntangleControl **controls;
};

G_DEFINE_TYPE(EntangleControlGroup, entangle_control_group, ENTANGLE_TYPE_CONTROL);


static void entangle_control_group_finalize (GObject *object)
{
    EntangleControlGroup *picker = ENTANGLE_CONTROL_GROUP(object);
    EntangleControlGroupPrivate *priv = picker->priv;

    for (int i = 0 ; i < priv->ncontrol ; i++) {
        g_object_unref(priv->controls[i]);
    }
    g_free(priv->controls);

    G_OBJECT_CLASS (entangle_control_group_parent_class)->finalize (object);
}

static void entangle_control_group_class_init(EntangleControlGroupClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_control_group_finalize;

    g_type_class_add_private(klass, sizeof(EntangleControlGroupPrivate));
}


EntangleControlGroup *entangle_control_group_new(const char *path,
                                         int id,
                                         const char *label,
                                         const char *info)
{
    return ENTANGLE_CONTROL_GROUP(g_object_new(ENTANGLE_TYPE_CONTROL_GROUP,
                                           "path", path,
                                           "id", id,
                                           "label", label,
                                           "info", info,
                                           NULL));
}


static void entangle_control_group_init(EntangleControlGroup *picker)
{
    EntangleControlGroupPrivate *priv;

    priv = picker->priv = ENTANGLE_CONTROL_GROUP_GET_PRIVATE(picker);
}

void entangle_control_group_add(EntangleControlGroup *group,
                            EntangleControl *control)
{
    EntangleControlGroupPrivate *priv = group->priv;

    priv->controls = g_renew(EntangleControl *, priv->controls, priv->ncontrol+1);
    priv->controls[priv->ncontrol++] = control;
    g_object_ref(control);
}


int entangle_control_group_count(EntangleControlGroup *group)
{
    EntangleControlGroupPrivate *priv = group->priv;

    return priv->ncontrol;
}


EntangleControl *entangle_control_group_get(EntangleControlGroup *group, int idx)
{
    EntangleControlGroupPrivate *priv = group->priv;

    if (idx < 0 || idx >= priv->ncontrol)
        return NULL;

    return priv->controls[idx];
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
