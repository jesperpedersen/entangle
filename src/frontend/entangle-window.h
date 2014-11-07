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

#include <gtk/gtk.h>

#ifndef __ENTANGLE_WINDOW_H__
# define __ENTANGLE_WINDOW_H__

#include <gtk/gtk.h>

#include "entangle-camera.h"
#include "entangle-application.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_WINDOW                (entangle_window_get_type ())
#define ENTANGLE_WINDOW(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_WINDOW, EntangleWindow))
#define ENTANGLE_IS_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_WINDOW))
#define ENTANGLE_WINDOW_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), ENTANGLE_TYPE_WINDOW, EntangleWindowInterface))

typedef struct _EntangleWindow EntangleWindow; /* dummy object */
typedef struct _EntangleWindowInterface EntangleWindowInterface;

struct _EntangleWindowInterface
{
    GTypeInterface parent;

    void (*set_builder)(EntangleWindow *win,
                        GtkBuilder *builder);
    GtkBuilder *(*get_builder)(EntangleWindow *win);
};


GType entangle_window_get_type(void) G_GNUC_CONST;

EntangleWindow *entangle_window_new(GType newwintype,
                                    GType oldwintype,
                                    const gchar *winname);

void entangle_window_set_builder(EntangleWindow *win,
                                 GtkBuilder *builder);

GtkBuilder *entangle_window_get_builder(EntangleWindow *win);


G_END_DECLS

#endif /* __ENTANGLE_WINDOW_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
