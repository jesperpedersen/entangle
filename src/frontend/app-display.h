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

#ifndef __CAPA_APP_DISPLAY_H__
#define __CAPA_APP_DISPLAY_H__

#include <glib-object.h>

#include "app.h"

G_BEGIN_DECLS

#define CAPA_TYPE_APP_DISPLAY            (capa_app_display_get_type ())
#define CAPA_APP_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_APP_DISPLAY, CapaAppDisplay))
#define CAPA_APP_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_APP_DISPLAY, CapaAppDisplayClass))
#define CAPA_IS_APP_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_APP_DISPLAY))
#define CAPA_IS_APP_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_APP_DISPLAY))
#define CAPA_APP_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_APP_DISPLAY, CapaAppDisplayClass))


typedef struct _CapaAppDisplay CapaAppDisplay;
typedef struct _CapaAppDisplayPrivate CapaAppDisplayPrivate;
typedef struct _CapaAppDisplayClass CapaAppDisplayClass;

struct _CapaAppDisplay
{
    CapaApp parent;

    CapaAppDisplayPrivate *priv;
};

struct _CapaAppDisplayClass
{
    CapaAppClass parent_class;

    void (*app_closed)(CapaAppDisplay *app);
};


GType capa_app_display_get_type(void) G_GNUC_CONST;
CapaAppDisplay* capa_app_display_new(void);

gboolean capa_app_display_show(CapaAppDisplay *display);

G_END_DECLS

#endif /* __CAPA_APP_DISPLAY_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
