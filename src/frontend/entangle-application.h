/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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

#ifndef __ENTANGLE_APPLICATION_H__
#define __ENTANGLE_APPLICATION_H__

#include <gtk/gtk.h>
#include <libpeas/peas.h>

#include "entangle-preferences.h"
#include "entangle-camera-list.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_APPLICATION            (entangle_application_get_type ())
#define ENTANGLE_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_APPLICATION, EntangleApplication))
#define ENTANGLE_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_APPLICATION, EntangleApplicationClass))
#define ENTANGLE_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_APPLICATION))
#define ENTANGLE_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_APPLICATION))
#define ENTANGLE_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_APPLICATION, EntangleApplicationClass))


typedef struct _EntangleApplication EntangleApplication;
typedef struct _EntangleApplicationPrivate EntangleApplicationPrivate;
typedef struct _EntangleApplicationClass EntangleApplicationClass;

struct _EntangleApplication
{
    GtkApplication parent;

    EntangleApplicationPrivate *priv;
};

struct _EntangleApplicationClass
{
    GtkApplicationClass parent_class;
};

GType entangle_application_get_type(void) G_GNUC_CONST;

EntangleApplication *entangle_application_new(void);

EntangleCameraList *entangle_application_get_cameras(EntangleApplication *application);
EntanglePreferences *entangle_application_get_preferences(EntangleApplication *application);
PeasEngine *entangle_application_get_plugin_engine(EntangleApplication *application);

G_END_DECLS

#endif /* __ENTANGLE_APPLICATION_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
