/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#ifndef __ENTANGLE_APP_H__
#define __ENTANGLE_APP_H__

#include <glib-object.h>
#if HAVE_PLUGINS
#include <libpeas/peas.h>
#endif

#include "entangle-preferences.h"
#include "entangle-camera-list.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_APP            (entangle_app_get_type ())
#define ENTANGLE_APP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_APP, EntangleApp))
#define ENTANGLE_APP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_APP, EntangleAppClass))
#define ENTANGLE_IS_APP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_APP))
#define ENTANGLE_IS_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_APP))
#define ENTANGLE_APP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_APP, EntangleAppClass))


typedef struct _EntangleApp EntangleApp;
typedef struct _EntangleAppPrivate EntangleAppPrivate;
typedef struct _EntangleAppClass EntangleAppClass;

struct _EntangleApp
{
    GObject parent;

    EntangleAppPrivate *priv;
};

struct _EntangleAppClass
{
    GObjectClass parent_class;
};

GType entangle_app_get_type(void) G_GNUC_CONST;

EntangleApp *entangle_app_new(void);

void entangle_app_refresh_cameras(EntangleApp *app);

EntangleCameraList *entangle_app_get_cameras(EntangleApp *app);
EntanglePreferences *entangle_app_get_preferences(EntangleApp *app);
#if HAVE_PLUGINS
PeasEngine *entangle_app_get_plugin_engine(EntangleApp *app);
#endif

G_END_DECLS

#endif /* __ENTANGLE_APP_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
