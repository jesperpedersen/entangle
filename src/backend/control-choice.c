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
#include <string.h>

#include "control-choice.h"

#define CAPA_CONTROL_CHOICE_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_CHOICE, CapaControlChoicePrivate))

struct _CapaControlChoicePrivate {
  size_t nchoices;
  char **choices;
};

G_DEFINE_TYPE(CapaControlChoice, capa_control_choice, CAPA_TYPE_CONTROL);


static void capa_control_choice_finalize (GObject *object)
{
  CapaControlChoice *picker = CAPA_CONTROL_CHOICE(object);
  CapaControlChoicePrivate *priv = picker->priv;

  for (int i = 0 ; i < priv->nchoices ; i++)
    g_free(priv->choices[i]);
  g_free(priv->choices);

  G_OBJECT_CLASS (capa_control_choice_parent_class)->finalize (object);
}

static void capa_control_choice_class_init(CapaControlChoiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_control_choice_finalize;

  g_type_class_add_private(klass, sizeof(CapaControlChoicePrivate));
}


CapaControlChoice *capa_control_choice_new(const char *path,
					   int id,
					   const char *label,
					   const char *info)
{
  return CAPA_CONTROL_CHOICE(g_object_new(CAPA_TYPE_CONTROL_CHOICE,
					  "path", path,
					  "id", id,
					  "label", label,
					  "info", info,
					  NULL));
}


static void capa_control_choice_init(CapaControlChoice *picker)
{
  CapaControlChoicePrivate *priv;

  priv = picker->priv = CAPA_CONTROL_CHOICE_GET_PRIVATE(picker);
  memset(priv, 0, sizeof *priv);
}

void capa_control_choice_add_value(CapaControlChoice *choice,
				   const char *value)
{
  CapaControlChoicePrivate *priv = choice->priv;

  priv->choices = g_renew(char *, priv->choices, priv->nchoices+1);
  priv->choices[priv->nchoices++] = g_strdup(value);
}

int capa_control_choice_value_count(CapaControlChoice *choice)
{
  CapaControlChoicePrivate *priv = choice->priv;

  return priv->nchoices;
}

const char *capa_control_choice_value_get(CapaControlChoice *choice,
					  int idx)
{
  CapaControlChoicePrivate *priv = choice->priv;

  if (idx < 0 || idx >= priv->nchoices)
    return NULL;

  return priv->choices[idx];
}
