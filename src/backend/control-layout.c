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

#include "control-layout.h"

#define CAPA_CONTROL_LAYOUT_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_LAYOUT, CapaControlLayoutPrivate))

struct _CapaControlLayoutPrivate {
  size_t ngroup;
  CapaControlGroup **groups;
};

G_DEFINE_TYPE(CapaControlLayout, capa_control_layout, G_TYPE_OBJECT);


static void capa_control_layout_finalize (GObject *object)
{
  CapaControlLayout *picker = CAPA_CONTROL_LAYOUT(object);
  CapaControlLayoutPrivate *priv = picker->priv;

  for (int i = 0 ; i < priv->ngroup ; i++) {
    g_object_unref(priv->groups[i]);
  }
  g_free(priv->groups);

  G_OBJECT_CLASS (capa_control_layout_parent_class)->finalize (object);
}

static void capa_control_layout_class_init(CapaControlLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_control_layout_finalize;

  g_type_class_add_private(klass, sizeof(CapaControlLayoutPrivate));
}


CapaControlLayout *capa_control_layout_new(void)
{
  return CAPA_CONTROL_LAYOUT(g_object_new(CAPA_TYPE_CONTROL_LAYOUT, NULL));
}


static void capa_control_layout_init(CapaControlLayout *picker)
{
  CapaControlLayoutPrivate *priv;

  priv = picker->priv = CAPA_CONTROL_LAYOUT_GET_PRIVATE(picker);
}

void capa_control_layout_add_group(CapaControlLayout *layout,
				   CapaControlGroup *group)
{
  CapaControlLayoutPrivate *priv = layout->priv;

  priv->groups = g_renew(CapaControlGroup *, priv->groups, priv->ngroup+1);
  priv->groups[priv->ngroup++] = group;
  g_object_ref(G_OBJECT(group));
}
