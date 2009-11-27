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

#ifndef __CAPA_PREFERENCES_DISPLAY_H__
#define __CAPA_PREFERENCES_DISPLAY_H__

#include <glib-object.h>

#include "app.h"
#include "preferences.h"

G_BEGIN_DECLS

#define CAPA_TYPE_PREFERENCES_DISPLAY            (capa_preferences_display_get_type ())
#define CAPA_PREFERENCES_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PREFERENCES_DISPLAY, CapaPreferencesDisplay))
#define CAPA_PREFERENCES_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_PREFERENCES_DISPLAY, CapaPreferencesDisplayClass))
#define CAPA_IS_PREFERENCES_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PREFERENCES_DISPLAY))
#define CAPA_IS_PREFERENCES_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_PREFERENCES_DISPLAY))
#define CAPA_PREFERENCES_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_PREFERENCES_DISPLAY, CapaPreferencesDisplayClass))


typedef struct _CapaPreferencesDisplay CapaPreferencesDisplay;
typedef struct _CapaPreferencesDisplayPrivate CapaPreferencesDisplayPrivate;
typedef struct _CapaPreferencesDisplayClass CapaPreferencesDisplayClass;

struct _CapaPreferencesDisplay
{
    GObject parent;

    CapaPreferencesDisplayPrivate *priv;
};

struct _CapaPreferencesDisplayClass
{
    GObjectClass parent_class;

};


GType capa_preferences_display_get_type(void) G_GNUC_CONST;
CapaPreferencesDisplay* capa_preferences_display_new(CapaPreferences *prefs);

void capa_preferences_display_show(CapaPreferencesDisplay *preferences);

G_END_DECLS

#endif /* __CAPA_PREFERENCES_DISPLAY_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
