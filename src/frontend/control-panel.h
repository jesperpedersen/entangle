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

#ifndef __CAPA_CONTROL_PANEL_H__
#define __CAPA_CONTROL_PANEL_H__

#include <gtk/gtk.h>

#include "camera.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL_PANEL            (capa_control_panel_get_type ())
#define CAPA_CONTROL_PANEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL_PANEL, CapaControlPanel))
#define CAPA_CONTROL_PANEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL_PANEL, CapaControlPanelClass))
#define CAPA_IS_CONTROL_PANEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL_PANEL))
#define CAPA_IS_CONTROL_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL_PANEL))
#define CAPA_CONTROL_PANEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL_PANEL, CapaControlPanelClass))


typedef struct _CapaControlPanel CapaControlPanel;
typedef struct _CapaControlPanelPrivate CapaControlPanelPrivate;
typedef struct _CapaControlPanelClass CapaControlPanelClass;

struct _CapaControlPanel
{
  GtkVBox parent;

  CapaControlPanelPrivate *priv;
};

struct _CapaControlPanelClass
{
  GtkVBoxClass parent_class;

};

GType capa_control_panel_get_type(void) G_GNUC_CONST;

CapaControlPanel* capa_control_panel_new(void);


G_END_DECLS

#endif /* __CAPA_CONTROL_PANEL_H__ */

