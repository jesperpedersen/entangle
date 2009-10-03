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

#ifndef __CAPA_SESSION_BROWSER__
#define __CAPA_SESSION_BROWSER__

#include <gtk/gtk.h>

#include "image.h"

G_BEGIN_DECLS

#define CAPA_TYPE_SESSION_BROWSER            (capa_session_browser_get_type ())
#define CAPA_SESSION_BROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_SESSION_BROWSER, CapaSessionBrowser))
#define CAPA_SESSION_BROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_SESSION_BROWSER, CapaSessionBrowserClass))
#define CAPA_IS_SESSION_BROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_SESSION_BROWSER))
#define CAPA_IS_SESSION_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_SESSION_BROWSER))
#define CAPA_SESSION_BROWSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_SESSION_BROWSER, CapaSessionBrowserClass))


typedef struct _CapaSessionBrowser CapaSessionBrowser;
typedef struct _CapaSessionBrowserPrivate CapaSessionBrowserPrivate;
typedef struct _CapaSessionBrowserClass CapaSessionBrowserClass;

struct _CapaSessionBrowser
{
  GtkIconView parent;

  CapaSessionBrowserPrivate *priv;
};

struct _CapaSessionBrowserClass
{
  GtkIconViewClass parent_class;

};

GType capa_session_browser_get_type(void) G_GNUC_CONST;

CapaSessionBrowser* capa_session_browser_new(void);

CapaImage *capa_session_browser_selected_image(CapaSessionBrowser *browser);

G_END_DECLS

#endif /* __CAPA_SESSION_BROWSER__ */

