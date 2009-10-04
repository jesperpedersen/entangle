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

#include <stdio.h>

#include "control-group.h"

#define CAPA_CONTROL_GROUP_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_GROUP, CapaControlGroupPrivate))

struct _CapaControlGroupPrivate {
  size_t ncontrol;
  CapaControl **controls;
};

G_DEFINE_TYPE(CapaControlGroup, capa_control_group, CAPA_TYPE_CONTROL);


static void capa_control_group_finalize (GObject *object)
{
  CapaControlGroup *picker = CAPA_CONTROL_GROUP(object);
  CapaControlGroupPrivate *priv = picker->priv;

  for (int i = 0 ; i < priv->ncontrol ; i++) {
    g_object_unref(priv->controls[i]);
  }
  g_free(priv->controls);

  G_OBJECT_CLASS (capa_control_group_parent_class)->finalize (object);
}

static void capa_control_group_class_init(CapaControlGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_control_group_finalize;

  g_type_class_add_private(klass, sizeof(CapaControlGroupPrivate));
}


CapaControlGroup *capa_control_group_new(const char *path,
					 int id,
					 const char *label,
					 const char *info)
{
  return CAPA_CONTROL_GROUP(g_object_new(CAPA_TYPE_CONTROL_GROUP,
					 "path", path,
					 "id", id,
					 "label", label,
					 "info", info,
					 NULL));
}


static void capa_control_group_init(CapaControlGroup *picker)
{
  CapaControlGroupPrivate *priv;

  priv = picker->priv = CAPA_CONTROL_GROUP_GET_PRIVATE(picker);
}

void capa_control_group_add(CapaControlGroup *group,
			    CapaControl *control)
{
  CapaControlGroupPrivate *priv = group->priv;

  priv->controls = g_renew(CapaControl *, priv->controls, priv->ncontrol+1);
  priv->controls[priv->ncontrol++] = control;
  g_object_ref(G_OBJECT(control));
}


int capa_control_group_count(CapaControlGroup *group)
{
  CapaControlGroupPrivate *priv = group->priv;

  return priv->ncontrol;
}


CapaControl *capa_control_group_get(CapaControlGroup *group, int idx)
{
  CapaControlGroupPrivate *priv = group->priv;

  if (idx < 0 || idx >= priv->ncontrol)
    return NULL;

  return priv->controls[idx];
}
