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

#ifndef __ENTANGLE_APP_DISPLAY_H__
#define __ENTANGLE_APP_DISPLAY_H__

#include <glib-object.h>

#include "entangle-app.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_APP_DISPLAY            (entangle_app_display_get_type ())
#define ENTANGLE_APP_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_APP_DISPLAY, EntangleAppDisplay))
#define ENTANGLE_APP_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_APP_DISPLAY, EntangleAppDisplayClass))
#define ENTANGLE_IS_APP_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_APP_DISPLAY))
#define ENTANGLE_IS_APP_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_APP_DISPLAY))
#define ENTANGLE_APP_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_APP_DISPLAY, EntangleAppDisplayClass))


typedef struct _EntangleAppDisplay EntangleAppDisplay;
typedef struct _EntangleAppDisplayPrivate EntangleAppDisplayPrivate;
typedef struct _EntangleAppDisplayClass EntangleAppDisplayClass;

struct _EntangleAppDisplay
{
    EntangleApp parent;

    EntangleAppDisplayPrivate *priv;
};

struct _EntangleAppDisplayClass
{
    EntangleAppClass parent_class;

    void (*app_closed)(EntangleAppDisplay *app);
};


GType entangle_app_display_get_type(void) G_GNUC_CONST;
EntangleAppDisplay* entangle_app_display_new(void);

gboolean entangle_app_display_show(EntangleAppDisplay *display);

G_END_DECLS

#endif /* __ENTANGLE_APP_DISPLAY_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
