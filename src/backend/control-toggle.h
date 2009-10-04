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

#ifndef __CONTROL_TOGGLE__
#define __CONTROL_TOGGLE__

#include <glib-object.h>

#include "control.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL_TOGGLE            (capa_control_toggle_get_type ())
#define CAPA_CONTROL_TOGGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL_TOGGLE, CapaControlToggle))
#define CAPA_CONTROL_TOGGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL_TOGGLE, CapaControlToggleClass))
#define CAPA_IS_CONTROL_TOGGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL_TOGGLE))
#define CAPA_IS_CONTROL_TOGGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL_TOGGLE))
#define CAPA_CONTROL_TOGGLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL_TOGGLE, CapaControlToggleClass))


typedef struct _CapaControlToggle CapaControlToggle;
typedef struct _CapaControlTogglePrivate CapaControlTogglePrivate;
typedef struct _CapaControlToggleClass CapaControlToggleClass;

struct _CapaControlToggle
{
  CapaControl parent;

  CapaControlTogglePrivate *priv;
};

struct _CapaControlToggleClass
{
  CapaControlClass parent_class;
};


GType capa_control_toggle_get_type(void) G_GNUC_CONST;
CapaControlToggle* capa_control_toggle_new(const char *path,
					   int id,
					   const char *label,
					   const char *info);

G_END_DECLS

#endif /* __CONTROL_TOGGLE__ */

