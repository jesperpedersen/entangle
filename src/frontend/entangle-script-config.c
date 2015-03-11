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

#include "entangle-debug.h"
#include "entangle-script-config.h"

#define ENTANGLE_SCRIPT_CONFIG_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_SCRIPT_CONFIG, EntangleScriptConfigPrivate))

struct _EntangleScriptConfigPrivate {
    GtkWidget *scriptBox;
    GtkListStore *scriptModel;
    GtkWidget *scriptConfig;
};

G_DEFINE_TYPE(EntangleScriptConfig, entangle_script_config, GTK_TYPE_BOX);


static void entangle_script_config_finalize(GObject *object)
{
    EntangleScriptConfig *config = ENTANGLE_SCRIPT_CONFIG(object);
    EntangleScriptConfigPrivate *priv = config->priv;

    g_object_unref(priv->scriptModel);

    G_OBJECT_CLASS(entangle_script_config_parent_class)->finalize(object);
}


static void entangle_script_config_class_init(EntangleScriptConfigClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_script_config_finalize;

    g_type_class_add_private(klass, sizeof(EntangleScriptConfigPrivate));
}


EntangleScriptConfig *entangle_script_config_new(void)
{
    return ENTANGLE_SCRIPT_CONFIG(g_object_new(ENTANGLE_TYPE_SCRIPT_CONFIG, NULL));
}


static void
entangle_script_config_data_func(GtkCellLayout *layout G_GNUC_UNUSED,
                                 GtkCellRenderer *cell,
                                 GtkTreeModel *model,
                                 GtkTreeIter *iter,
                                 gpointer data G_GNUC_UNUSED)
{
    EntangleScript *script;

    gtk_tree_model_get(model, iter, 0, &script, -1);

    if (script)
        g_object_set(cell, "text", entangle_script_get_title(script), NULL);
    else
        g_object_set(cell, "text", _("No script"), NULL);
}


static void
entangle_script_config_changed(GtkWidget *src G_GNUC_UNUSED,
                               gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_SCRIPT_CONFIG(data));

    EntangleScriptConfig *config = ENTANGLE_SCRIPT_CONFIG(data);
    EntangleScriptConfigPrivate *priv = config->priv;
    GtkTreeIter iter;
    EntangleScript *script;
    GtkWidget *widget;
    gint page;

    if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->scriptBox), &iter)) {
        gtk_widget_hide(priv->scriptConfig);
    } else {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->scriptModel), &iter, 0, &script, 1, &widget, -1);

        page = gtk_notebook_page_num(GTK_NOTEBOOK(priv->scriptConfig), widget);
        if (page == -1) {
            gtk_widget_hide(priv->scriptConfig);
        } else {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->scriptConfig), page);
            gtk_widget_show(priv->scriptConfig);
        }
    }
}

static void entangle_script_config_init(EntangleScriptConfig *config)
{
    EntangleScriptConfigPrivate *priv;
    GtkCellRenderer *cell;
    GtkTreeIter iter;

    priv = config->priv = ENTANGLE_SCRIPT_CONFIG_GET_PRIVATE(config);

    gtk_orientable_set_orientation(GTK_ORIENTABLE(config), GTK_ORIENTATION_VERTICAL);

    priv->scriptModel = gtk_list_store_new(2, ENTANGLE_TYPE_SCRIPT, GTK_TYPE_WIDGET);
    priv->scriptBox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(priv->scriptModel));
    priv->scriptConfig = gtk_notebook_new();

    g_signal_connect(priv->scriptBox, "changed",
                     G_CALLBACK(entangle_script_config_changed), config);

    g_object_set(priv->scriptConfig, "show-border", FALSE, "show-tabs", FALSE, NULL);

    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->scriptBox), cell, TRUE);
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(priv->scriptBox), cell,
                                       entangle_script_config_data_func,
                                       NULL, NULL);

    gtk_list_store_append(priv->scriptModel, &iter);
    gtk_list_store_set(priv->scriptModel, &iter,
                       0, NULL, 1, NULL, -1);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->scriptBox), &iter);

    gtk_container_add(GTK_CONTAINER(config), priv->scriptBox);
    gtk_container_add(GTK_CONTAINER(config), priv->scriptConfig);
    gtk_widget_show(priv->scriptBox);

    g_object_set(config, "spacing", 6, NULL);
}


void entangle_script_config_add_script(EntangleScriptConfig *config,
                                       EntangleScript *script)
{
    g_return_if_fail(ENTANGLE_IS_SCRIPT_CONFIG(config));
    g_return_if_fail(ENTANGLE_IS_SCRIPT(script));

    EntangleScriptConfigPrivate *priv = config->priv;
    GtkTreeIter iter;
    GtkWidget *widget;

    widget = entangle_script_get_config_widget(script);
    if (widget == NULL)
        widget = gtk_label_new(_("No config options"));
    gtk_container_add(GTK_CONTAINER(priv->scriptConfig), widget);
    gtk_widget_show(widget);

    gtk_list_store_append(priv->scriptModel, &iter);
    gtk_list_store_set(priv->scriptModel, &iter,
                       0, script, 1, widget, -1);
}


void entangle_script_config_remove_script(EntangleScriptConfig *config,
                                          EntangleScript *script)
{
    g_return_if_fail(ENTANGLE_IS_SCRIPT_CONFIG(config));
    g_return_if_fail(ENTANGLE_IS_SCRIPT(script));

    EntangleScriptConfigPrivate *priv = config->priv;
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->scriptModel), &iter))
        return;

    do {
        EntangleScript *val;
        GtkWidget *widget;
        gtk_tree_model_get(GTK_TREE_MODEL(priv->scriptModel), &iter,
                           0, &val, 1, &widget ,-1);

        if (val == script) {
            gtk_container_remove(GTK_CONTAINER(priv->scriptConfig), widget);
            gtk_widget_destroy(widget);
            gtk_list_store_remove(priv->scriptModel, &iter);
            break;
        }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->scriptModel), &iter));

    if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->scriptBox), &iter)) {
        if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->scriptModel), &iter))
            return;
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->scriptBox), &iter);
    }
}

gboolean entangle_script_config_has_scripts(EntangleScriptConfig *config)
{
    g_return_val_if_fail(ENTANGLE_IS_SCRIPT_CONFIG(config), FALSE);

    EntangleScriptConfigPrivate *priv = config->priv;

    return gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->scriptModel), NULL) > 1;
}

/**
 * entangle_script_config_get_selected:
 * @config: (transfer none): the config widget
 *
 * Retrieve the script that the config is displayed for
 *
 * Returns: (transfer none): the script displayed
 */
EntangleScript *entangle_script_config_get_selected(EntangleScriptConfig *config)
{
    g_return_val_if_fail(ENTANGLE_IS_SCRIPT_CONFIG(config), NULL);

    EntangleScriptConfigPrivate *priv = config->priv;
    GtkTreeIter iter;
    EntangleScript *script;

    if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->scriptBox), &iter))
        return NULL;

    gtk_tree_model_get(GTK_TREE_MODEL(priv->scriptModel), &iter,
                       0, &script, -1);

    return script;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
