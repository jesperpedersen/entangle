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

#ifndef __CAPA_APP_H__
#define __CAPA_APP_H__

#include <glib-object.h>

#include "capa-preferences.h"
#include "capa-camera-list.h"
#include "capa-plugin-manager.h"

G_BEGIN_DECLS

#define CAPA_TYPE_APP            (capa_app_get_type ())
#define CAPA_APP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_APP, CapaApp))
#define CAPA_APP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_APP, CapaAppClass))
#define CAPA_IS_APP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_APP))
#define CAPA_IS_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_APP))
#define CAPA_APP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_APP, CapaAppClass))


typedef struct _CapaApp CapaApp;
typedef struct _CapaAppPrivate CapaAppPrivate;
typedef struct _CapaAppClass CapaAppClass;

struct _CapaApp
{
    GObject parent;

    CapaAppPrivate *priv;
};

struct _CapaAppClass
{
    GObjectClass parent_class;
};

GType capa_app_get_type(void) G_GNUC_CONST;

CapaApp *capa_app_new(void);

void capa_app_refresh_cameras(CapaApp *app);

CapaCameraList *capa_app_get_cameras(CapaApp *app);
CapaPreferences *capa_app_get_preferences(CapaApp *app);
CapaPluginManager *capa_app_get_plugin_manager(CapaApp *app);

G_END_DECLS

#endif /* __CAPA_APP_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
