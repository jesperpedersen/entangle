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

#include "internal.h"
#include "control-date.h"

#define CAPA_CONTROL_DATE_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_DATE, CapaControlDatePrivate))

struct _CapaControlDatePrivate {
};

G_DEFINE_TYPE(CapaControlDate, capa_control_date, CAPA_TYPE_CONTROL);


static void capa_control_date_finalize (GObject *object)
{
  G_OBJECT_CLASS (capa_control_date_parent_class)->finalize (object);
}

static void capa_control_date_class_init(CapaControlDateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_control_date_finalize;

  g_type_class_add_private(klass, sizeof(CapaControlDatePrivate));
}


CapaControlDate *capa_control_date_new(const char *path,
				       int id,
				       const char *label,
				       const char *info)
{
  return CAPA_CONTROL_DATE(g_object_new(CAPA_TYPE_CONTROL_DATE,
					"path", path,
					"id", id,
					"label", label,
					"info", info,
					NULL));
}


static void capa_control_date_init(CapaControlDate *picker)
{
  CapaControlDatePrivate *priv;

  priv = picker->priv = CAPA_CONTROL_DATE_GET_PRIVATE(picker);
}

