/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2015 Daniel P. Berrange
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

#ifndef __ENTANGLE_PREFERENCES_H__
#define __ENTANGLE_PREFERENCES_H__

#include <glib-object.h>

#include "entangle-control-group.h"
#include "entangle-colour-profile.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_PREFERENCES            (entangle_preferences_get_type ())
#define ENTANGLE_PREFERENCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PREFERENCES, EntanglePreferences))
#define ENTANGLE_PREFERENCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_PREFERENCES, EntanglePreferencesClass))
#define ENTANGLE_IS_PREFERENCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PREFERENCES))
#define ENTANGLE_IS_PREFERENCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_PREFERENCES))
#define ENTANGLE_PREFERENCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_PREFERENCES, EntanglePreferencesClass))


typedef struct _EntanglePreferences EntanglePreferences;
typedef struct _EntanglePreferencesPrivate EntanglePreferencesPrivate;
typedef struct _EntanglePreferencesClass EntanglePreferencesClass;

struct _EntanglePreferences
{
    GObject parent;

    EntanglePreferencesPrivate *priv;
};

struct _EntanglePreferencesClass
{
    GObjectClass parent_class;
};


GType entangle_preferences_get_type(void) G_GNUC_CONST;

EntanglePreferences *entangle_preferences_new(void);

gboolean entangle_preferences_interface_get_auto_connect(EntanglePreferences *prefs);
void entangle_preferences_interface_set_auto_connect(EntanglePreferences *prefs, gboolean autoconn);
gboolean entangle_preferences_interface_get_screen_blank(EntanglePreferences *prefs);
void entangle_preferences_interface_set_screen_blank(EntanglePreferences *prefs, gboolean blank);
gchar **entangle_preferences_interface_get_plugins(EntanglePreferences *prefs);
void entangle_preferences_interface_add_plugin(EntanglePreferences *prefs, const char *name);
void entangle_preferences_interface_remove_plugin(EntanglePreferences *prefs, const char *name);
gboolean entangle_preferences_interface_get_histogram_linear(EntanglePreferences *prefs);
void entangle_preferences_interface_set_histogram_linear(EntanglePreferences *prefs, gboolean enabled);

char *entangle_preferences_capture_get_last_session(EntanglePreferences *prefs);
void entangle_preferences_capture_set_last_session(EntanglePreferences *prefs, const gchar *dir);
char *entangle_preferences_capture_get_filename_pattern(EntanglePreferences *prefs);
void entangle_preferences_capture_set_filename_pattern(EntanglePreferences *prefs, const gchar *pattern);
gboolean entangle_preferences_capture_get_continuous_preview(EntanglePreferences *prefs);
void entangle_preferences_capture_set_continuous_preview(EntanglePreferences *prefs, gboolean enabled);
gboolean entangle_preferences_capture_get_electronic_shutter(EntanglePreferences *prefs);
void entangle_preferences_capture_set_electronic_shutter(EntanglePreferences *prefs, gboolean enabled);
gboolean entangle_preferences_capture_get_delete_file(EntanglePreferences *prefs);
void entangle_preferences_capture_set_delete_file(EntanglePreferences *prefs, gboolean enabled);
gboolean entangle_preferences_capture_get_sync_clock(EntanglePreferences *prefs);
void entangle_preferences_capture_set_sync_clock(EntanglePreferences *prefs, gboolean enabled);

gboolean entangle_preferences_cms_get_enabled(EntanglePreferences *prefs);
void entangle_preferences_cms_set_enabled(EntanglePreferences *prefs, gboolean enabled);
EntangleColourProfile *entangle_preferences_cms_get_rgb_profile(EntanglePreferences *prefs);
void entangle_preferences_cms_set_rgb_profile(EntanglePreferences *prefs, EntangleColourProfile *prof);
EntangleColourProfile *entangle_preferences_cms_get_monitor_profile(EntanglePreferences *prefs);
void entangle_preferences_cms_set_monitor_profile(EntanglePreferences *prefs, EntangleColourProfile *prof);
gboolean entangle_preferences_cms_get_detect_system_profile(EntanglePreferences *prefs);
void entangle_preferences_cms_set_detect_system_profile(EntanglePreferences *prefs, gboolean enabled);
EntangleColourProfileIntent entangle_preferences_cms_get_rendering_intent(EntanglePreferences *prefs);
void entangle_preferences_cms_set_rendering_intent(EntanglePreferences *prefs, EntangleColourProfileIntent intent);


gchar *entangle_preferences_img_get_aspect_ratio(EntanglePreferences *prefs);
void entangle_preferences_img_set_aspect_ratio(EntanglePreferences *prefs, const gchar *aspect);
gint entangle_preferences_img_get_mask_opacity(EntanglePreferences *prefs);
void entangle_preferences_img_set_mask_opacity(EntanglePreferences *prefs, gint opacity);
gboolean entangle_preferences_img_get_mask_enabled(EntanglePreferences *prefs);
void entangle_preferences_img_set_mask_enabled(EntanglePreferences *prefs, gboolean enabled);
gboolean entangle_preferences_img_get_focus_point(EntanglePreferences *prefs);
void entangle_preferences_img_set_focus_point(EntanglePreferences *prefs, gboolean enabled);
gint entangle_preferences_img_get_grid_lines(EntanglePreferences *prefs);
void entangle_preferences_img_set_grid_lines(EntanglePreferences *prefs, gint gridLines);
gboolean entangle_preferences_img_get_embedded_preview(EntanglePreferences *prefs);
void entangle_preferences_img_set_embedded_preview(EntanglePreferences *prefs, gboolean enabled);
gboolean entangle_preferences_img_get_onion_skin(EntanglePreferences *prefs);
void entangle_preferences_img_set_onion_skin(EntanglePreferences *prefs, gboolean enabled);
gint entangle_preferences_img_get_onion_layers(EntanglePreferences *prefs);
void entangle_preferences_img_set_onion_layers(EntanglePreferences *prefs, gint layers);
gchar *entangle_preferences_img_get_background(EntanglePreferences *prefs);
void entangle_preferences_img_set_background(EntanglePreferences *prefs, const gchar *bkg);
gchar *entangle_preferences_img_get_highlight(EntanglePreferences *prefs);
void entangle_preferences_img_set_highlight(EntanglePreferences *prefs, const gchar *bkg);

G_END_DECLS

#endif /* __ENTANGLE_PREFERENCES_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
