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

#ifndef __CAPA_PREFERENCES_H__
#define __CAPA_PREFERENCES_H__

#include <glib-object.h>

#include "capa-control-group.h"
#include "capa-colour-profile.h"

G_BEGIN_DECLS

#define CAPA_TYPE_PREFERENCES            (capa_preferences_get_type ())
#define CAPA_PREFERENCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_PREFERENCES, CapaPreferences))
#define CAPA_PREFERENCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_PREFERENCES, CapaPreferencesClass))
#define CAPA_IS_PREFERENCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_PREFERENCES))
#define CAPA_IS_PREFERENCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_PREFERENCES))
#define CAPA_PREFERENCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_PREFERENCES, CapaPreferencesClass))


typedef struct _CapaPreferences CapaPreferences;
typedef struct _CapaPreferencesPrivate CapaPreferencesPrivate;
typedef struct _CapaPreferencesClass CapaPreferencesClass;

struct _CapaPreferences
{
    GObject parent;

    CapaPreferencesPrivate *priv;
};

struct _CapaPreferencesClass
{
    GObjectClass parent_class;
};


GType capa_preferences_get_type(void) G_GNUC_CONST;

CapaPreferences *capa_preferences_new(void);

const char *capa_preferences_picture_dir(CapaPreferences *prefs);

const char *capa_preferences_filename_pattern(CapaPreferences *prefs);

CapaColourProfile *capa_preferences_rgb_profile(CapaPreferences *prefs);
CapaColourProfile *capa_preferences_monitor_profile(CapaPreferences *prefs);
gboolean capa_preferences_detect_monitor_profile(CapaPreferences *prefs);
gboolean capa_preferences_enable_color_management(CapaPreferences *prefs);
CapaColourProfileIntent capa_preferences_profile_rendering_intent(CapaPreferences *prefs);



G_END_DECLS

#endif /* __CAPA_PREFERENCES_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
