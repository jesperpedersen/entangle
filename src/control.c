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
  char *path;
  int id;
  char *label;
};

G_DEFINE_TYPE(CapaControl, capa_control, G_TYPE_OBJECT);

enum {
  PROP_0,
  PROP_PATH,
  PROP_ID,
  PROP_LABEL,
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
    case PROP_PATH:
      g_value_set_string(value, priv->path);
      break;

    case PROP_ID:
      g_value_set_int(value, priv->id);
      break;

    case PROP_LABEL:
      g_value_set_string(value, priv->label);
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

  switch (prop_id)
    {
    case PROP_PATH:
      g_free(priv->path);
      priv->path = g_value_dup_string(value);
      break;

    case PROP_ID:
      priv->id = g_value_get_int(value);
      break;

    case PROP_LABEL:
      g_free(priv->label);
      priv->label = g_value_dup_string(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_control_finalize (GObject *object)
{
  CapaControl *picker = CAPA_CONTROL(object);
  CapaControlPrivate *priv = picker->priv;

  g_free(priv->path);
  g_free(priv->label);

  G_OBJECT_CLASS (capa_control_parent_class)->finalize (object);
}

static void capa_control_class_init(CapaControlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_control_finalize;
  object_class->get_property = capa_control_get_property;
  object_class->set_property = capa_control_set_property;

  g_object_class_install_property(object_class,
				  PROP_PATH,
				  g_param_spec_string("path",
						      "Control path",
						      "Path of the control",
						      NULL,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT_ONLY |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_object_class_install_property(object_class,
				  PROP_ID,
				  g_param_spec_string("id",
						      "Control id",
						      "Id of the control",
						      NULL,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT_ONLY |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_object_class_install_property(object_class,
				  PROP_LABEL,
				  g_param_spec_string("label",
						      "Control label",
						      "Label of the control",
						      NULL,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT_ONLY |
						      G_PARAM_STATIC_NAME |
						      G_PARAM_STATIC_NICK |
						      G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaControlPrivate));
}


CapaControl *capa_control_new(const char *path, int id, const char *label)
{
  return CAPA_CONTROL(g_object_new(CAPA_TYPE_CONTROL,
				   "path", path,
				   "id", id,
				   "label", label,
				   NULL));
}


static void capa_control_init(CapaControl *picker)
{
  CapaControlPrivate *priv;

  priv = picker->priv = CAPA_CONTROL_GET_PRIVATE(picker);
}

