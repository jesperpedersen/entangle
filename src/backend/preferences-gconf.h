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

#ifndef __CAPA_PREFERENCES_GCONF_H__
#define __CAPA_PREFERENCES_GCONF_H__

#include <glib-object.h>

#include "preferences.h"

G_BEGIN_DECLS

#define CAPA_TYPE_PREFERENCES_GCONF            (capa_preferences_gconf_get_type ())
#define CAPA_PREFERENCES_GCONF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PREFERENCES_GCONF, CapaPreferencesGConf))
#define CAPA_PREFERENCES_GCONF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_PREFERENCES_GCONF, CapaPreferencesGConfClass))
#define CAPA_IS_PREFERENCES_GCONF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PREFERENCES_GCONF))
#define CAPA_IS_PREFERENCES_GCONF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_PREFERENCES_GCONF))
#define CAPA_PREFERENCES_GCONF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_PREFERENCES_GCONF, CapaPreferencesGConfClass))


typedef struct _CapaPreferencesGConf CapaPreferencesGConf;
typedef struct _CapaPreferencesGConfPrivate CapaPreferencesGConfPrivate;
typedef struct _CapaPreferencesGConfClass CapaPreferencesGConfClass;

struct _CapaPreferencesGConf
{
    CapaPreferences parent;

    CapaPreferencesGConfPrivate *priv;
};

struct _CapaPreferencesGConfClass
{
    CapaPreferencesClass parent_class;
};


GType capa_preferences_gconf_get_type(void) G_GNUC_CONST;

CapaPreferences *capa_preferences_gconf_new(void);

G_END_DECLS

#endif /* __CAPA_PREFERENCES_GCONF_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
