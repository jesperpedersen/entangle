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

#include <string.h>
#include <glib/gi18n.h>
#include <math.h>

#include "entangle-debug.h"
#include "entangle-control-panel.h"
#include "entangle-control-button.h"
#include "entangle-control-choice.h"
#include "entangle-control-date.h"
#include "entangle-control-group.h"
#include "entangle-control-range.h"
#include "entangle-control-text.h"
#include "entangle-control-toggle.h"

#define ENTANGLE_CONTROL_PANEL_GET_PRIVATE(obj)                         \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_PANEL, EntangleControlPanelPrivate))

struct _EntangleControlPanelPrivate {
    EntangleCameraPreferences *cameraPrefs;
    EntangleCamera *camera;

    gulong sigCamera;
    gboolean hasControls;
    gboolean inUpdate;

    GtkWidget *grid;
};

G_DEFINE_TYPE(EntangleControlPanel, entangle_control_panel, GTK_TYPE_EXPANDER);

enum {
    PROP_O,
    PROP_CAMERA,
    PROP_CAMERA_PREFS,
    PROP_HAS_CONTROLS,
};

static void do_control_remove(GtkWidget *widget,
                              gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(data));

    EntangleControlPanel *panel = data;
    EntangleControlPanelPrivate *priv = panel->priv;

    gtk_container_remove(GTK_CONTAINER(priv->grid), widget);
}


static void do_update_control_finish(GObject *src,
                                     GAsyncResult *res,
                                     gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(data));

    GError *error = NULL;

    if (!entangle_camera_save_controls_finish(ENTANGLE_CAMERA(src), res, &error)) {
        GtkWidget *msg = gtk_message_dialog_new(NULL,
                                                0,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Camera control update failed"));
        gtk_window_set_title(GTK_WINDOW(msg),
                             _("Entangle: Camera control update failed"));
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg),
                                                 "%s",
                                                 error->message);
        g_signal_connect_swapped(msg,
                                 "response",
                                 G_CALLBACK(gtk_widget_destroy),
                                 msg);
        gtk_widget_show_all(msg);
        g_error_free(error);
    }
}


static gboolean do_refresh_control_entry_idle(gpointer data)
{
    GtkWidget *widget = GTK_WIDGET(data);
    EntangleControlPanel *panel = g_object_get_data(G_OBJECT(widget), "panel");
    GObject *control = g_object_get_data(G_OBJECT(widget), "control");
    gchar *text;

    panel->priv->inUpdate = TRUE;
    g_object_get(control, "value", &text, NULL);
    ENTANGLE_DEBUG("Notified control entry '%s' ('%s') with '%s'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   text);

    if (GTK_IS_LABEL(widget))
        gtk_label_set_text(GTK_LABEL(widget), text);
    else
        gtk_entry_set_text(GTK_ENTRY(widget), text);
    g_free(text);
    panel->priv->inUpdate = FALSE;
    return FALSE;
}

static void do_refresh_control_entry(GObject *object G_GNUC_UNUSED,
                                     GParamSpec *pspec G_GNUC_UNUSED,
                                     gpointer data)
{
    g_idle_add(do_refresh_control_entry_idle, data);
}

static void do_update_control_entry(GtkWidget *widget,
                                    GdkEventFocus *ev G_GNUC_UNUSED,
                                    gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(data));

    EntangleControlText *control = g_object_get_data(G_OBJECT(widget), "control");
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(data);
    EntangleControlPanelPrivate *priv = panel->priv;
    const char *text;

    if (panel->priv->inUpdate)
        return;

    text = gtk_entry_get_text(GTK_ENTRY(widget));

    ENTANGLE_DEBUG("Updated control entry '%s' ('%s') with '%s'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   text);
    g_object_set(control, "value", text, NULL);

    entangle_camera_save_controls_async(priv->camera,
                                        NULL,
                                        do_update_control_finish,
                                        panel);
}


static gboolean do_refresh_control_range_idle(gpointer data)
{
    GtkWidget *widget = GTK_WIDGET(data);
    EntangleControlPanel *panel = g_object_get_data(G_OBJECT(widget), "panel");
    GObject *control = g_object_get_data(G_OBJECT(widget), "control");
    gfloat val;

    panel->priv->inUpdate = TRUE;
    g_object_get(control, "value", &val, NULL);
    ENTANGLE_DEBUG("Notified control range '%s' ('%s') with '%lf'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   (double)val);

    if (GTK_IS_LABEL(widget)) {
        gchar *text = g_strdup_printf("%0.02f", (double)val);
        gtk_label_set_text(GTK_LABEL(widget), text);
        g_free(text);
    } else {
        gtk_range_set_value(GTK_RANGE(widget), val);
    }
    panel->priv->inUpdate = FALSE;
    return FALSE;
}


static void do_refresh_control_range(GObject *object G_GNUC_UNUSED,
                                     GParamSpec *pspec G_GNUC_UNUSED,
                                     gpointer data)
{
    g_idle_add(do_refresh_control_range_idle, data);
}


static void do_update_control_range(GtkRange *widget G_GNUC_UNUSED,
                                    GtkScrollType scroll G_GNUC_UNUSED,
                                    gdouble value,
                                    gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(data));

    EntangleControlRange *control = g_object_get_data(G_OBJECT(widget), "control");
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(data);
    EntangleControlPanelPrivate *priv = panel->priv;

    if (panel->priv->inUpdate)
        return;

    ENTANGLE_DEBUG("Updated control range '%s' ('%s') with '%lf'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   value);
    g_object_set(control, "value", (double)value, NULL);

    entangle_camera_save_controls_async(priv->camera,
                                        NULL,
                                        do_update_control_finish,
                                        panel);
}


static gboolean do_refresh_control_combo_idle(gpointer data)
{
    GtkWidget *widget = GTK_WIDGET(data);
    EntangleControlPanel *panel = g_object_get_data(G_OBJECT(widget), "panel");
    GObject *control = g_object_get_data(G_OBJECT(widget), "control");
    gchar *text;

    panel->priv->inUpdate = TRUE;
    g_object_get(control, "value", &text, NULL);
    ENTANGLE_DEBUG("Notified control combo '%s' ('%s') with '%s'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   text);

    if (GTK_IS_LABEL(widget)) {
        gtk_label_set_text(GTK_LABEL(widget), text);
    } else {
        GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)));
        int active = 0;
        gtk_list_store_clear(store);
        for (int n = 0; n < entangle_control_choice_entry_count(ENTANGLE_CONTROL_CHOICE(control)); n++) {
            GtkTreeIter iter;
            if (g_strcmp0(text, entangle_control_choice_entry_get(ENTANGLE_CONTROL_CHOICE(control), n)) == 0)
                active = n;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0,
                               entangle_control_choice_entry_get(ENTANGLE_CONTROL_CHOICE(control), n),
                               -1);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(widget), active);
    }
    g_free(text);
    panel->priv->inUpdate = FALSE;

    return FALSE;
}


static void do_refresh_control_combo(GObject *object G_GNUC_UNUSED,
                                     GParamSpec *pspec G_GNUC_UNUSED,
                                     gpointer data)
{
    g_idle_add(do_refresh_control_combo_idle, data);
}


static void do_update_control_combo(GtkComboBox *widget,
                                    gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(data));

    EntangleControlChoice *control = g_object_get_data(G_OBJECT(widget), "control");
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(data);
    EntangleControlPanelPrivate *priv = panel->priv;
    GtkTreeIter iter;
    char *text = NULL;
    GtkTreeModel *model = gtk_combo_box_get_model(widget);

    if (panel->priv->inUpdate)
        return;

    if (gtk_combo_box_get_active_iter(widget, &iter))
        gtk_tree_model_get(model, &iter, 0, &text, -1);

    ENTANGLE_DEBUG("Updated control combo '%s' ('%s') with '%s'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   text);
    g_object_set(control, "value", text, NULL);

    g_free(text);

    entangle_camera_save_controls_async(priv->camera,
                                        NULL,
                                        do_update_control_finish,
                                        panel);
}


static gboolean do_refresh_control_toggle_idle(gpointer data)
{
    GtkWidget *widget = GTK_WIDGET(data);
    EntangleControlPanel *panel = g_object_get_data(G_OBJECT(widget), "panel");
    GObject *control = g_object_get_data(G_OBJECT(widget), "control");
    gboolean state;

    panel->priv->inUpdate = TRUE;
    g_object_get(control, "value", &state, NULL);
    ENTANGLE_DEBUG("Notified control toggle '%s' ('%s') with '%d'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   state);

    if (GTK_IS_LABEL(widget))
        gtk_label_set_text(GTK_LABEL(widget), state ? _("On") : _("Off"));
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                     state);
    panel->priv->inUpdate = FALSE;
    return FALSE;
}


static void do_refresh_control_toggle(GObject *object G_GNUC_UNUSED,
                                      GParamSpec *pspec G_GNUC_UNUSED,
                                      gpointer data)
{
    g_idle_add(do_refresh_control_toggle_idle, data);
}


static void do_update_control_toggle(GtkToggleButton *widget,
                                     gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(data));

    EntangleControlChoice *control = g_object_get_data(G_OBJECT(widget), "control");
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(data);
    EntangleControlPanelPrivate *priv = panel->priv;
    gboolean active;

    if (panel->priv->inUpdate)
        return;

    active = gtk_toggle_button_get_active(widget);
    ENTANGLE_DEBUG("Updated control toggle '%s' ('%s') with '%d'",
                   entangle_control_get_path(ENTANGLE_CONTROL(control)),
                   entangle_control_get_label(ENTANGLE_CONTROL(control)),
                   active);
    g_object_set(control, "value", active, NULL);

    entangle_camera_save_controls_async(priv->camera,
                                        NULL,
                                        do_update_control_finish,
                                        panel);
}

static gboolean do_update_control_readonly_idle(gpointer data)
{
    GtkWidget *widget = GTK_WIDGET(data);
    GObject *control = g_object_get_data(G_OBJECT(widget), "control");
    gboolean state;

    g_object_get(control, "readonly", &state, NULL);
    gtk_widget_set_sensitive(widget, !state);

    return FALSE;
}


static void do_update_control_readonly(GObject *object G_GNUC_UNUSED,
                                       GParamSpec *pspec G_GNUC_UNUSED,
                                       gpointer data)
{
    g_idle_add(do_update_control_readonly_idle, data);
}


static void do_setup_control(EntangleControlPanel *panel,
                             EntangleControl *control,
                             GtkContainer *box,
                             gint row)
{
    GtkWidget *label = NULL;
    GtkWidget *value = NULL;
    gboolean needLabel = TRUE;

    ENTANGLE_DEBUG("Build control %d %s",
                   entangle_control_get_id(control),
                   entangle_control_get_label(control));

    if (ENTANGLE_IS_CONTROL_BUTTON(control)) {
        needLabel = FALSE;
        value = gtk_button_new_with_label(entangle_control_get_label(control));
        if (entangle_control_get_readonly(control))
            gtk_widget_set_sensitive(value, FALSE);
        g_signal_connect(control, "notify::readonly",
                         G_CALLBACK(do_update_control_readonly), value);
    } else if (ENTANGLE_IS_CONTROL_CHOICE(control)) {
        GtkCellRenderer *cell;
        GtkListStore *store;
        char *text;
        int active = -1;

        /*
         * Need todo better here
         *
         *  If there's only two entries 0/1, turn into toggle
         *  If there's a continuous sequence of numbers turn
         *      into a spinbutton
         *
         *  Some sequences of numbers are nonsene, and need to
         *  be turned in to real labels.
         *
         *   eg Shutter speed 0.00025 should be presented 1/4000
         */

        store = gtk_list_store_new(1, G_TYPE_STRING);
        value = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
        g_object_unref(store);

        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(value), cell, TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(value), cell,
                                       "text", 0,
                                       NULL);

        g_object_get(control, "value", &text, NULL);
        for (int n = 0; n < entangle_control_choice_entry_count(ENTANGLE_CONTROL_CHOICE(control)); n++) {
            GtkTreeIter iter;
            if (g_strcmp0(text, entangle_control_choice_entry_get(ENTANGLE_CONTROL_CHOICE(control), n)) == 0)
                active = n;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0,
                               entangle_control_choice_entry_get(ENTANGLE_CONTROL_CHOICE(control), n),
                               -1);
        }

        if (entangle_control_get_readonly(control))
            gtk_widget_set_sensitive(value, FALSE);
        gtk_combo_box_set_active(GTK_COMBO_BOX(value), active);

        g_object_set_data(G_OBJECT(value), "panel", panel);
        g_object_set_data(G_OBJECT(value), "control", control);
        g_signal_connect(value, "changed",
                         G_CALLBACK(do_update_control_combo), panel);
        g_signal_connect(control, "notify::value",
                         G_CALLBACK(do_refresh_control_combo), value);
        g_signal_connect(control, "notify::readonly",
                         G_CALLBACK(do_update_control_readonly), value);
    } else if (ENTANGLE_IS_CONTROL_DATE(control)) {
        int date;

        value = gtk_entry_new();
        g_object_get(control, "value", &date, NULL);
        if (entangle_control_get_readonly(control))
            gtk_widget_set_sensitive(value, FALSE);
        //gtk_entry_set_text(GTK_ENTRY(value), text);
    } else if (ENTANGLE_IS_CONTROL_RANGE(control)) {
        gfloat offset;
        gdouble min = entangle_control_range_get_min(ENTANGLE_CONTROL_RANGE(control));
        gdouble max = entangle_control_range_get_max(ENTANGLE_CONTROL_RANGE(control));
        gboolean forceReadonly = FALSE;

        if (fabs(min-max) < 0.005) {
            forceReadonly = TRUE;
            max += 1;
        }

        value = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                         min, max,
                                         entangle_control_range_get_step(ENTANGLE_CONTROL_RANGE(control)));
        g_object_get(control, "value", &offset, NULL);
        gtk_range_set_value(GTK_RANGE(value), offset);
        if (entangle_control_get_readonly(control) || forceReadonly)
            gtk_widget_set_sensitive(value, FALSE);
        g_object_set_data(G_OBJECT(value), "panel", panel);
        g_object_set_data(G_OBJECT(value), "control", control);
        g_signal_connect(value, "change-value",
                         G_CALLBACK(do_update_control_range), panel);
        g_signal_connect(control, "notify::value",
                         G_CALLBACK(do_refresh_control_range), value);
        g_signal_connect(control, "notify::readonly",
                         G_CALLBACK(do_update_control_readonly), value);
    } else if (ENTANGLE_IS_CONTROL_TEXT(control)) {
        const char *text;

        value = gtk_entry_new();
        g_object_get(control, "value", &text, NULL);
        gtk_entry_set_text(GTK_ENTRY(value), text);
        if (entangle_control_get_readonly(control))
            gtk_widget_set_sensitive(value, FALSE);
        g_object_set_data(G_OBJECT(value), "panel", panel);
        g_object_set_data(G_OBJECT(value), "control", control);
        g_signal_connect(value, "focus-out-event",
                         G_CALLBACK(do_update_control_entry), panel);
        g_signal_connect(control, "notify::value",
                         G_CALLBACK(do_refresh_control_entry), value);
        g_signal_connect(control, "notify::readonly",
                         G_CALLBACK(do_update_control_readonly), value);
    } else if (ENTANGLE_IS_CONTROL_TOGGLE(control)) {
        gboolean active;
        needLabel = FALSE;
        value = gtk_check_button_new_with_label(entangle_control_get_label(control));
        g_object_get(control, "value", &active, NULL);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(value), active);
        if (entangle_control_get_readonly(control))
            gtk_widget_set_sensitive(value, FALSE);
        g_object_set_data(G_OBJECT(value), "panel", panel);
        g_object_set_data(G_OBJECT(value), "control", control);
        g_signal_connect(value, "toggled",
                         G_CALLBACK(do_update_control_toggle), panel);
        g_signal_connect(control, "notify::value",
                         G_CALLBACK(do_refresh_control_toggle), value);
        g_signal_connect(control, "notify::readonly",
                         G_CALLBACK(do_update_control_readonly), value);
    }

    if (needLabel) {
        label = gtk_label_new(entangle_control_get_label(control));
        gtk_widget_set_tooltip_text(label, entangle_control_get_info(control));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_widget_set_halign(label, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(box), label, 0, row, 1, 1);

        gtk_widget_set_hexpand(value, TRUE);
        gtk_widget_set_halign(value, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(box), value, 1, row, 1, 1);
    } else {
        gtk_widget_set_hexpand(value, TRUE);
        gtk_widget_set_halign(value, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(box), value, 0, row, 2, 1);
    }
}


static gchar **entangle_control_panel_get_default_controls(EntangleControlGroup *root)
{
    gchar **controls = NULL;
    gsize ncontrols = 0;

    if (entangle_control_group_get_by_path(root,
                                           "/main/capturesettings/f-number")) {
        controls = g_renew(gchar *, controls, ncontrols + 1);
        controls[ncontrols++] = g_strdup("/main/capturesettings/f-number");
    }

    if (entangle_control_group_get_by_path(root,
                                           "/main/capturesettings/shutterspeed2")) {
        controls = g_renew(gchar *, controls, ncontrols + 1);
        controls[ncontrols++] = g_strdup("/main/capturesettings/shutterspeed2");
    } else if (entangle_control_group_get_by_path(root,
                                                  "/main/capturesettings/shutterspeed")) {
        controls = g_renew(gchar *, controls, ncontrols + 1);
        controls[ncontrols++] = g_strdup("/main/capturesettings/shutterspeed");
    }

    if (entangle_control_group_get_by_path(root,
                                           "/main/imgsettings/iso")) {
        controls = g_renew(gchar *, controls, ncontrols + 1);
        controls[ncontrols++] = g_strdup("/main/imgsettings/iso");
    }

    if (entangle_control_group_get_by_path(root,
                                           "/main/imgsettings/whitebalance")) {
        controls = g_renew(gchar *, controls, ncontrols + 1);
        controls[ncontrols++] = g_strdup("/main/imgsettings/whitebalance");
    }

    if (entangle_control_group_get_by_path(root,
                                           "/main/capturesettings/imagequality")) {
        controls = g_renew(gchar *, controls, ncontrols + 1);
        controls[ncontrols++] = g_strdup("/main/capturesettings/imagequality");
    }

    if (entangle_control_group_get_by_path(root,
                                           "/main/imgsettings/imagesize")) {
        controls = g_renew(gchar *, controls, ncontrols + 1);
        controls[ncontrols++] = g_strdup("/main/imgsettings/imagesize");
    }

    controls = g_renew(gchar *, controls, ncontrols + 1);
    controls[ncontrols++] = NULL;

    return controls;
}


static void do_setup_camera(EntangleControlPanel *panel)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(panel));

    EntangleControlPanelPrivate *priv = panel->priv;
    EntangleControlGroup *root;
    EntangleControl *control;
    gint row = 0;
    gchar **controls;
    gsize i;

    gtk_container_foreach(GTK_CONTAINER(priv->grid), do_control_remove, panel);

    if (!priv->camera) {
        GtkWidget *label = gtk_label_new(_("No camera connected"));

        gtk_widget_set_hexpand(label, TRUE);
        gtk_widget_set_halign(label, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(priv->grid), label, 0, row, 2, 1);
        gtk_widget_show_all(GTK_WIDGET(panel));
        return;
    }

    root = entangle_camera_get_controls(priv->camera, NULL);

    if (!root) {
        GtkWidget *label = gtk_label_new(_("No controls available"));
        gtk_widget_set_hexpand(label, TRUE);
        gtk_widget_set_halign(label, GTK_ALIGN_FILL);
        gtk_grid_attach(GTK_GRID(priv->grid), label, 0, row, 2, 1);
        gtk_widget_show_all(GTK_WIDGET(panel));
        return;
    }

    controls = entangle_camera_preferences_get_controls(priv->cameraPrefs);
    if (!controls || !controls[0]) {
        controls = entangle_control_panel_get_default_controls(root);
        entangle_camera_preferences_set_controls(priv->cameraPrefs,
                                                 (const char *const *)controls);
    }

    for (i = 0; controls[i] != NULL; i++) {
        if ((control = entangle_control_group_get_by_path(root,
                                                          controls[i])))
            do_setup_control(panel, control, GTK_CONTAINER(priv->grid), row++);
    }

    gtk_widget_show_all(GTK_WIDGET(panel));
    g_object_unref(root);
    g_strfreev(controls);
}


static void do_update_camera(GObject *object G_GNUC_UNUSED,
                             GParamSpec *pspec G_GNUC_UNUSED,
                             gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CONTROL_PANEL(data));

    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(data);
    EntangleControlPanelPrivate *priv = panel->priv;

    if (priv->camera) {
        g_object_unref(priv->camera);
        priv->camera = NULL;
    }
    priv->camera = entangle_camera_preferences_get_camera(priv->cameraPrefs);
    if (priv->camera) {
        g_object_ref(priv->camera);
        do_setup_camera(panel);
    }
}


static void entangle_control_panel_get_property(GObject *object,
                                                guint prop_id,
                                                GValue *value,
                                                GParamSpec *pspec)
{
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(object);
    EntangleControlPanelPrivate *priv = panel->priv;

    switch (prop_id)
        {
        case PROP_CAMERA_PREFS:
            g_value_set_object(value, priv->cameraPrefs);
            break;

        case PROP_CAMERA:
            g_value_set_object(value, priv->camera);
            break;

        case PROP_HAS_CONTROLS:
            g_value_set_boolean(value, priv->hasControls);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_control_panel_set_property(GObject *object,
                                                guint prop_id,
                                                const GValue *value,
                                                GParamSpec *pspec)
{
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(object);
    EntangleControlPanelPrivate *priv = panel->priv;

    ENTANGLE_DEBUG("Set prop on control panel %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA_PREFS:
            priv->cameraPrefs = g_value_get_object(value);
            priv->sigCamera = g_signal_connect(priv->cameraPrefs, "notify::camera",
                                               G_CALLBACK(do_update_camera), panel);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_control_panel_finalize(GObject *object)
{
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(object);
    EntangleControlPanelPrivate *priv = panel->priv;

    if (priv->camera)
        g_object_unref(priv->camera);

    if (priv->cameraPrefs) {
        g_signal_handler_disconnect(priv->cameraPrefs, priv->sigCamera);
        g_object_unref(priv->cameraPrefs);
    }

    G_OBJECT_CLASS(entangle_control_panel_parent_class)->finalize(object);
}


static void entangle_control_panel_class_init(EntangleControlPanelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_control_panel_finalize;
    object_class->get_property = entangle_control_panel_get_property;
    object_class->set_property = entangle_control_panel_set_property;

    g_object_class_install_property(object_class,
                                    PROP_CAMERA_PREFS,
                                    g_param_spec_object("camera-prefs",
                                                        "Camera prefs",
                                                        "Camera preferences to manage",
                                                        ENTANGLE_TYPE_CAMERA_PREFERENCES,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to manage",
                                                        ENTANGLE_TYPE_CAMERA,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_HAS_CONTROLS,
                                    g_param_spec_boolean("has-controls",
                                                         "Has Controls",
                                                         "Has Controls",
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleControlPanelPrivate));
}


EntangleControlPanel *entangle_control_panel_new(EntangleCameraPreferences *prefs)
{
    return ENTANGLE_CONTROL_PANEL(g_object_new(ENTANGLE_TYPE_CONTROL_PANEL,
                                               "camera-prefs", prefs,
                                               "label", "Camera settings",
                                               "expanded", TRUE,
                                               NULL));
}


static void entangle_control_panel_init(EntangleControlPanel *panel)
{
    EntangleControlPanelPrivate *priv = panel->priv;

    priv = panel->priv = ENTANGLE_CONTROL_PANEL_GET_PRIVATE(panel);

    gtk_container_set_border_width(GTK_CONTAINER(panel), 0);

    priv->grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(priv->grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(priv->grid), 6);
    gtk_container_set_border_width(GTK_CONTAINER(priv->grid), 6);

    gtk_container_add(GTK_CONTAINER(panel), priv->grid);

    do_setup_camera(panel);
}


/**
 * entangle_control_panel_get_camera_preferences:
 * @panel: the control widget
 *
 * Get the camera preferences whose controls are displayed
 *
 * Returns: (transfer none): the camera preferences or NULL
 */
EntangleCameraPreferences *entangle_control_panel_get_camera_preferences(EntangleControlPanel *panel)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL_PANEL(panel), NULL);

    EntangleControlPanelPrivate *priv = panel->priv;

    return priv->cameraPrefs;
}


gboolean entangle_control_panel_get_has_controls(EntangleControlPanel *panel)
{
    g_return_val_if_fail(ENTANGLE_IS_CONTROL_PANEL(panel), FALSE);

    EntangleControlPanelPrivate *priv = panel->priv;

    return priv->hasControls;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
