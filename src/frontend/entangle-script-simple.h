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

#ifndef __ENTANGLE_SCRIPT_SIMPLE_H__
#define __ENTANGLE_SCRIPT_SIMPLE_H__

#include "entangle-script.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_SCRIPT_SIMPLE            (entangle_script_simple_get_type ())
#define ENTANGLE_SCRIPT_SIMPLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_SCRIPT_SIMPLE, EntangleScriptSimple))
#define ENTANGLE_SCRIPT_SIMPLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_SCRIPT_SIMPLE, EntangleScriptSimpleClass))
#define ENTANGLE_IS_SCRIPT_SIMPLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_SCRIPT_SIMPLE))
#define ENTANGLE_IS_SCRIPT_SIMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_SCRIPT_SIMPLE))
#define ENTANGLE_SCRIPT_SIMPLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_SCRIPT_SIMPLE, EntangleScriptSimpleClass))

typedef struct _EntangleScriptSimple EntangleScriptSimple;
typedef struct _EntangleScriptSimplePrivate EntangleScriptSimplePrivate;
typedef struct _EntangleScriptSimpleClass EntangleScriptSimpleClass;

struct _EntangleScriptSimple
{
    EntangleScript parent;

    EntangleScriptSimplePrivate *priv;
};

struct _EntangleScriptSimpleClass
{
    EntangleScriptClass parent_class;

    void (*execute)(EntangleScriptSimple *script,
                    EntangleCameraAutomata *automata,
                    GCancellable *cancel,
                    GTask *result);

    GObject *(*init_task_data)(EntangleScriptSimple *script);
};


GType entangle_script_simple_get_type(void) G_GNUC_CONST;

void entangle_script_simple_return_task_error(EntangleScriptSimple *script,
                                              GTask *result,
                                              const gchar *message);

GObject *entangle_script_simple_init_task_data(EntangleScriptSimple *script);
GObject *entangle_script_simple_get_task_data(EntangleScriptSimple *script,
                                              GTask *result);

G_END_DECLS

#endif /* __ENTANGLE_SCRIPT_SIMPLE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
