/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2014 Daniel P. Berrange
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

#ifndef __ENTANGLE_CONTROL_PANEL_H__
#define __ENTANGLE_CONTROL_PANEL_H__

#include <gtk/gtk.h>

#include "entangle-camera-preferences.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONTROL_PANEL            (entangle_control_panel_get_type ())
#define ENTANGLE_CONTROL_PANEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONTROL_PANEL, EntangleControlPanel))
#define ENTANGLE_CONTROL_PANEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONTROL_PANEL, EntangleControlPanelClass))
#define ENTANGLE_IS_CONTROL_PANEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONTROL_PANEL))
#define ENTANGLE_IS_CONTROL_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONTROL_PANEL))
#define ENTANGLE_CONTROL_PANEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONTROL_PANEL, EntangleControlPanelClass))


typedef struct _EntangleControlPanel EntangleControlPanel;
typedef struct _EntangleControlPanelPrivate EntangleControlPanelPrivate;
typedef struct _EntangleControlPanelClass EntangleControlPanelClass;

struct _EntangleControlPanel
{
    GtkExpander parent;

    EntangleControlPanelPrivate *priv;
};

struct _EntangleControlPanelClass
{
    GtkExpanderClass parent_class;

};

GType entangle_control_panel_get_type(void) G_GNUC_CONST;

EntangleControlPanel* entangle_control_panel_new(EntangleCameraPreferences *prefs);

EntangleCameraPreferences *entangle_control_panel_get_camera_preferences(EntangleControlPanel *panel);

gboolean entangle_control_panel_get_has_controls(EntangleControlPanel *panel);

G_END_DECLS

#endif /* __ENTANGLE_CONTROL_PANEL_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
