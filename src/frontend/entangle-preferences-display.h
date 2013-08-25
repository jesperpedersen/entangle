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

#ifndef __ENTANGLE_PREFERENCES_DISPLAY_H__
#define __ENTANGLE_PREFERENCES_DISPLAY_H__

#include <gtk/gtk.h>

#include "entangle-application.h"


G_BEGIN_DECLS

#define ENTANGLE_TYPE_PREFERENCES_DISPLAY            (entangle_preferences_display_get_type ())
#define ENTANGLE_PREFERENCES_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PREFERENCES_DISPLAY, EntanglePreferencesDisplay))
#define ENTANGLE_PREFERENCES_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_PREFERENCES_DISPLAY, EntanglePreferencesDisplayClass))
#define ENTANGLE_IS_PREFERENCES_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PREFERENCES_DISPLAY))
#define ENTANGLE_IS_PREFERENCES_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_PREFERENCES_DISPLAY))
#define ENTANGLE_PREFERENCES_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_PREFERENCES_DISPLAY, EntanglePreferencesDisplayClass))


typedef struct _EntanglePreferencesDisplay EntanglePreferencesDisplay;
typedef struct _EntanglePreferencesDisplayPrivate EntanglePreferencesDisplayPrivate;
typedef struct _EntanglePreferencesDisplayClass EntanglePreferencesDisplayClass;

struct _EntanglePreferencesDisplay
{
    GtkDialog parent;

    EntanglePreferencesDisplayPrivate *priv;
};

struct _EntanglePreferencesDisplayClass
{
    GtkDialogClass parent_class;

};


GType entangle_preferences_display_get_type(void) G_GNUC_CONST;
EntanglePreferencesDisplay *entangle_preferences_display_new(void);

G_END_DECLS

#endif /* __ENTANGLE_PREFERENCES_DISPLAY_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
