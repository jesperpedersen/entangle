/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2014 Daniel P. Berrange
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

#ifndef __ENTANGLE_SCRIPT_H__
#define __ENTANGLE_SCRIPT_H__

#include <gtk/gtk.h>

#include "entangle-camera-automata.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_SCRIPT            (entangle_script_get_type ())
#define ENTANGLE_SCRIPT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_SCRIPT, EntangleScript))
#define ENTANGLE_SCRIPT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_SCRIPT, EntangleScriptClass))
#define ENTANGLE_IS_SCRIPT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_SCRIPT))
#define ENTANGLE_IS_SCRIPT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_SCRIPT))
#define ENTANGLE_SCRIPT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_SCRIPT, EntangleScriptClass))

typedef struct _EntangleScript EntangleScript;
typedef struct _EntangleScriptPrivate EntangleScriptPrivate;
typedef struct _EntangleScriptClass EntangleScriptClass;

struct _EntangleScript {
    GObject parent;

    EntangleScriptPrivate *priv;
};

struct _EntangleScriptClass
{
    GObjectClass parent_class;

    GtkWidget * (*get_config_widget)(EntangleScript *script);
    void (*execute_async)(EntangleScript *script,
                          EntangleCameraAutomata *automata,
                          GCancellable *cancel,
                          GAsyncReadyCallback callback,
                          gpointer data);
    gboolean (*execute_finish)(EntangleScript *script,
                               GAsyncResult *result,
                               GError **error);
};


GType entangle_script_get_type(void) G_GNUC_CONST;

const gchar *entangle_script_get_title(EntangleScript *script);
GtkWidget *entangle_script_get_config_widget(EntangleScript *script);
void entangle_script_execute_async(EntangleScript *script,
                                   EntangleCameraAutomata *automata,
                                   GCancellable *cancel,
                                   GAsyncReadyCallback callback,
                                   gpointer data);
gboolean entangle_script_execute_finish(EntangleScript *script,
                                        GAsyncResult *result,
                                        GError **error);

G_END_DECLS

#endif /* __ENTANGLE_SCRIPT_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
