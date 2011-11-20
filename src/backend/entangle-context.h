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

#ifndef __ENTANGLE_CONTEXT_H__
#define __ENTANGLE_CONTEXT_H__

#include <glib-object.h>
#include <libpeas/peas.h>

#include "entangle-preferences.h"
#include "entangle-camera-list.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONTEXT            (entangle_context_get_type ())
#define ENTANGLE_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONTEXT, EntangleContext))
#define ENTANGLE_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONTEXT, EntangleContextClass))
#define ENTANGLE_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONTEXT))
#define ENTANGLE_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONTEXT))
#define ENTANGLE_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONTEXT, EntangleContextClass))


typedef struct _EntangleContext EntangleContext;
typedef struct _EntangleContextPrivate EntangleContextPrivate;
typedef struct _EntangleContextClass EntangleContextClass;

struct _EntangleContext
{
    GObject parent;

    EntangleContextPrivate *priv;
};

struct _EntangleContextClass
{
    GObjectClass parent_class;
};

GType entangle_context_get_type(void) G_GNUC_CONST;

EntangleContext *entangle_context_new(GApplication *context);

void entangle_context_refresh_cameras(EntangleContext *context);

GApplication *entangle_context_get_application(EntangleContext *context);
EntangleCameraList *entangle_context_get_cameras(EntangleContext *context);
EntanglePreferences *entangle_context_get_preferences(EntangleContext *context);
PeasEngine *entangle_context_get_plugin_engine(EntangleContext *context);

G_END_DECLS

#endif /* __ENTANGLE_CONTEXT_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
