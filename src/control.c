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

#include "control.h"

#define CAPA_CONTROL_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL, CapaControlPrivate))

struct _CapaControlPrivate {
  char *name;
};

G_DEFINE_TYPE(CapaControl, capa_control, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_NAME,
};

static void capa_control_get_property(GObject *object,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *pspec)
{
  CapaControl *picker = CAPA_CONTROL(object);
  CapaControlPrivate *priv = picker->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string(value, priv->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_control_set_property(GObject *object,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *pspec)
{
  CapaControl *picker = CAPA_CONTROL(object);
  CapaControlPrivate *priv = picker->priv;

  fprintf(stderr, "Set prop %d\n", prop_id);

  switch (prop_id)
    {
    case PROP_NAME:
      g_free(priv->name);
      priv->name = g_value_dup_string(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_control_finalize (GObject *object)
{
  CapaControl *picker = CAPA_CONTROL(object);
  CapaControlPrivate *priv = picker->priv;

  g_free(priv->name);

  G_OBJECT_CLASS (capa_control_parent_class)->finalize (object);
}

static void capa_control_class_init(CapaControlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_control_finalize;
  object_class->get_property = capa_control_get_property;
  object_class->set_property = capa_control_set_property;

  g_object_class_install_property(object_class,
				  PROP_NAME,
				  g_param_spec_string("name",
						      "Control name",
						      "Name of the control",
						      NULL,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT_ONLY |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaControlPrivate));
}


CapaControl *capa_control_new(const char *name)
{
  return CAPA_CONTROL(g_object_new(CAPA_TYPE_CONTROL, name, NULL));
}


static void capa_control_init(CapaControl *picker)
{
  CapaControlPrivate *priv;

  priv = picker->priv = CAPA_CONTROL_GET_PRIVATE(picker);
}

