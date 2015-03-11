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

#include "entangle-script.h"
#include "entangle-debug.h"

#define ENTANGLE_SCRIPT_GET_PRIVATE(obj)                         \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_SCRIPT, EntangleScriptPrivate))

struct _EntangleScriptPrivate
{
    char *title;
};

G_DEFINE_ABSTRACT_TYPE(EntangleScript, entangle_script, G_TYPE_OBJECT);

enum {
    PROP_O,
    PROP_TITLE,
};

static void entangle_script_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
    EntangleScript *display = ENTANGLE_SCRIPT(object);
    EntangleScriptPrivate *priv = display->priv;

    switch (prop_id)
        {
        case PROP_TITLE:
            g_value_set_string(value, priv->title);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_script_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
    EntangleScript *display = ENTANGLE_SCRIPT(object);
    EntangleScriptPrivate *priv = display->priv;

    ENTANGLE_DEBUG("Set prop on image display %d", prop_id);

    switch (prop_id)
        {
        case PROP_TITLE:
            g_free(priv->title);
            priv->title = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_script_class_init(EntangleScriptClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->get_property = entangle_script_get_property;
    object_class->set_property = entangle_script_set_property;

    g_object_class_install_property(object_class,
                                    PROP_TITLE,
                                    g_param_spec_string("title",
                                                        "Title",
                                                        "Script title",
                                                        _("Untitled script"),
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleScriptPrivate));
}


static void entangle_script_init(EntangleScript *script)
{
    script->priv = ENTANGLE_SCRIPT_GET_PRIVATE(script);
}


/**
 * entangle_script_get_title:
 * @script: (transfer none): the script object
 *
 * Get the title of the script
 *
 * Returns: (transfer none): the title of the script
 */
const gchar *entangle_script_get_title(EntangleScript *script)
{
    g_return_val_if_fail(ENTANGLE_IS_SCRIPT(script), NULL);

    EntangleScriptPrivate *priv = script->priv;

    return priv->title;
}


/**
 * entangle_script_get_config_widget:
 * @script: (transfer none): the script object
 *
 * Get the configuration controls for the script
 *
 * Returns: (transfer full): the config widget
 */
GtkWidget *entangle_script_get_config_widget(EntangleScript *script)
{
    g_return_val_if_fail(ENTANGLE_IS_SCRIPT(script), NULL);
    g_return_val_if_fail(ENTANGLE_SCRIPT_GET_CLASS(script)->get_config_widget != NULL, NULL);

    return ENTANGLE_SCRIPT_GET_CLASS(script)->get_config_widget(script);
}


/**
 * entangle_script_execute_async:
 * @script: (transfer none): the script object
 * @automata: (transfer none): the camera automata
 * @cancel: (transfer none)(allow-none): cancellation handler
 * @callback: (scope async): a #GAsyncReadyCallback to call when the script has finished
 * @data: (closure): the data to pass to the callback function
 */
void entangle_script_execute_async(EntangleScript *script,
                                   EntangleCameraAutomata *automata,
                                   GCancellable *cancel,
                                   GAsyncReadyCallback callback,
                                   gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_SCRIPT(script));
    g_return_if_fail(ENTANGLE_SCRIPT_GET_CLASS(script)->execute_async != NULL);

    ENTANGLE_SCRIPT_GET_CLASS(script)->execute_async(script,
                                                     automata,
                                                     cancel,
                                                     callback,
                                                     data);
}


/**
 * entangle_script_execute_finish:
 * @script: (transfer none): the script object
 * @result: a GAsyncResult
 * @error: (allow-none): a GError
 *
 * Returns: TRUE on success, false otherwise
 */
gboolean entangle_script_execute_finish(EntangleScript *script,
                                        GAsyncResult *result,
                                        GError **error)
{
    g_return_val_if_fail(ENTANGLE_IS_SCRIPT(script), FALSE);
    g_return_val_if_fail(ENTANGLE_SCRIPT_GET_CLASS(script)->execute_finish != NULL, FALSE);

    return ENTANGLE_SCRIPT_GET_CLASS(script)->execute_finish(script,
                                                             result,
                                                             error);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
