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

#include <config.h>
#include <glib/gi18n.h>

#include "entangle-script-simple.h"

#define ENTANGLE_SCRIPT_SIMPLE_GET_PRIVATE(obj)                         \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_SCRIPT_SIMPLE, EntangleScriptSimplePrivate))

struct _EntangleScriptSimplePrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(EntangleScriptSimple, entangle_script_simple, ENTANGLE_TYPE_SCRIPT);

static void entangle_script_simple_finalize(GObject *object)
{
    G_OBJECT_CLASS(entangle_script_simple_parent_class)->finalize(object);
}


static void entangle_script_simple_execute_async(EntangleScript *script,
                                                 EntangleCameraAutomata *automata,
                                                 GCancellable *cancel,
                                                 GAsyncReadyCallback callback,
                                                 gpointer user_data)
{
    EntangleScriptSimpleClass *klass =
        ENTANGLE_SCRIPT_SIMPLE_GET_CLASS(script);
    GTask *result = g_task_new(script,
                               cancel, callback, user_data);
    GObject *data = entangle_script_simple_init_task_data(ENTANGLE_SCRIPT_SIMPLE(script));
    if (data)
        g_task_set_task_data(result, data, g_object_unref);

    if (klass->execute) {
        klass->execute(ENTANGLE_SCRIPT_SIMPLE(script),
                       automata,
                       cancel,
                       result);
    } else {
        g_task_return_new_error(result,
                                g_quark_from_string("entangle-script-simple"), 0,
                                "%s", _("Missing 'execute' method implementation"));
    }
}


static gboolean entangle_script_simple_execute_finish(EntangleScript *script G_GNUC_UNUSED,
                                                      GAsyncResult *result,
                                                      GError **error)
{
    GTask *task = G_TASK(result);

    return g_task_propagate_boolean(task, error);
}

void entangle_script_simple_return_task_error(EntangleScriptSimple *script G_GNUC_UNUSED,
                                              GTask *result,
                                              const gchar *message)
{
    g_task_return_new_error(result, g_quark_from_string("entangle-script-simple"), 0,
                            "%s", message);
}

/**
 * entangle_script_simple_init_task_data:
 * @script: the script object
 *
 * Returns: (transfer full): the data
 */
GObject *entangle_script_simple_init_task_data(EntangleScriptSimple *script)
{
    EntangleScriptSimpleClass *klass =
        ENTANGLE_SCRIPT_SIMPLE_GET_CLASS(script);

    if (klass->init_task_data) {
        return klass->init_task_data(ENTANGLE_SCRIPT_SIMPLE(script));
    }
    return NULL;
}

/**
 * entangle_script_simple_get_task_data:
 * @result: the task object
 *
 * Returns: (transfer full): the data
 */
GObject *entangle_script_simple_get_task_data(EntangleScriptSimple *script G_GNUC_UNUSED,
                                              GTask *result)
{
    GObject *obj = g_task_get_task_data(result);
    if (obj)
        g_object_ref(obj);
    return obj;
}


static void entangle_script_simple_class_init(EntangleScriptSimpleClass *klass)
{
    GObjectClass *object_klass = G_OBJECT_CLASS(klass);
    EntangleScriptClass *script_klass = ENTANGLE_SCRIPT_CLASS(klass);

    object_klass->finalize = entangle_script_simple_finalize;
    script_klass->execute_async = entangle_script_simple_execute_async;
    script_klass->execute_finish = entangle_script_simple_execute_finish;
}

static void entangle_script_simple_init(EntangleScriptSimple *script)
{
    script->priv = ENTANGLE_SCRIPT_SIMPLE_GET_PRIVATE(script);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
