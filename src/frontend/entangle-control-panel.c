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

#include <config.h>

#include <string.h>

#include "entangle-debug.h"
#include "entangle-control-panel.h"
#include "entangle-control-button.h"
#include "entangle-control-choice.h"
#include "entangle-control-date.h"
#include "entangle-control-group.h"
#include "entangle-control-range.h"
#include "entangle-control-text.h"
#include "entangle-control-toggle.h"

#define ENTANGLE_CONTROL_PANEL_GET_PRIVATE(obj)                             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_PANEL, EntangleControlPanelPrivate))

struct _EntangleControlPanelPrivate {
    EntangleCamera *camera;
    EntangleCameraScheduler *cameraScheduler;

    gboolean hasControls;
};

G_DEFINE_TYPE(EntangleControlPanel, entangle_control_panel, GTK_TYPE_VBOX);

enum {
    PROP_O,
    PROP_CAMERA,
    PROP_HAS_CONTROLS,
};


static void do_scheduler_pause(EntangleControlPanel *panel)
{
    EntangleControlPanelPrivate *priv = panel->priv;
    gdk_threads_leave();
    if (priv->cameraScheduler)
        entangle_camera_scheduler_pause(priv->cameraScheduler);
    gdk_threads_enter();
}

static void do_scheduler_resume(EntangleControlPanel *panel)
{
    EntangleControlPanelPrivate *priv = panel->priv;
    gdk_threads_leave();
    if (priv->cameraScheduler)
        entangle_camera_scheduler_resume(priv->cameraScheduler);
    gdk_threads_enter();
}

static void do_control_remove(GtkWidget *widget,
                              gpointer data)
{
    EntangleControlPanel *panel = data;

    gtk_container_remove(GTK_CONTAINER(panel), widget);
}

static void do_update_control_entry(GtkWidget *widget,
                                    GdkEventFocus *ev G_GNUC_UNUSED,
                                    gpointer data)
{
    EntangleControlPanel *panel = data;
    EntangleControlText *control = g_object_get_data(G_OBJECT(widget), "control");
    const char *text;

    text = gtk_entry_get_text(GTK_ENTRY(widget));

    ENTANGLE_DEBUG("entry [%s]", text);
    do_scheduler_pause(panel);
    g_object_set(control, "value", text, NULL);
    do_scheduler_resume(panel);
}

static void do_update_control_range(GtkRange *widget G_GNUC_UNUSED,
                                    GtkScrollType scroll G_GNUC_UNUSED,
                                    gdouble value,
                                    gpointer data)
{
    EntangleControlPanel *panel = data;
    EntangleControlText *control = g_object_get_data(G_OBJECT(widget), "control");

    ENTANGLE_DEBUG("range [%lf]", value);
    do_scheduler_pause(panel);
    g_object_set(control, "value", (float)value, NULL);
    do_scheduler_resume(panel);
}

static void do_update_control_combo(GtkComboBox *widget,
                                    gpointer data)
{
    EntangleControlPanel *panel = data;
    EntangleControlChoice *control = g_object_get_data(G_OBJECT(widget), "control");
    GtkTreeIter iter;
    char *text = NULL;
    GtkTreeModel *model = gtk_combo_box_get_model(widget);

    if (gtk_combo_box_get_active_iter(widget, &iter))
        gtk_tree_model_get(model, &iter, 0, &text, -1);

    ENTANGLE_DEBUG("combo [%s]", text);
    do_scheduler_pause(panel);
    g_object_set(control, "value", text, NULL);
    do_scheduler_resume(panel);

    g_free(text);
}

static void do_update_control_toggle(GtkToggleButton *widget,
                                     gpointer data)
{
    EntangleControlPanel *panel = data;
    EntangleControlChoice *control = g_object_get_data(G_OBJECT(widget), "control");
    gboolean active;

    active = gtk_toggle_button_get_active(widget);
    ENTANGLE_DEBUG("toggle [%d]", active);
    do_scheduler_pause(panel);
    g_object_set(control, "value", active, NULL);
    do_scheduler_resume(panel);
}

static void do_setup_control_group(EntangleControlPanel *panel,
                                   GtkVBox *box,
                                   EntangleControlGroup *grp)
{
    EntangleControlPanelPrivate *priv = panel->priv;
    int i;

    for (i = 0 ; i < entangle_control_group_count(grp) ; i++) {
        EntangleControl *control = entangle_control_group_get(grp, i);

        priv->hasControls = TRUE;

        ENTANGLE_DEBUG("Build control %d %s",
                   entangle_control_id(control),
                   entangle_control_label(control));

        if (ENTANGLE_IS_CONTROL_GROUP(control)) {
            GtkWidget *frame = gtk_expander_new(entangle_control_label(control));
            //GtkWidget *frame = gtk_frame_new(entangle_control_label(control));
            GtkWidget *subbox = gtk_vbox_new(FALSE, 6);

            gtk_container_add(GTK_CONTAINER(box), frame);
            //gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
            gtk_expander_set_expanded(GTK_EXPANDER(frame), TRUE);
            gtk_container_set_border_width(GTK_CONTAINER(subbox), 6);
            //g_object_unref(frame);

            gtk_container_add(GTK_CONTAINER(frame), subbox);
            //g_object_unref(subbox);

            do_setup_control_group(panel, GTK_VBOX(subbox), ENTANGLE_CONTROL_GROUP(control));
        } else if (ENTANGLE_IS_CONTROL_BUTTON(control)) {
            GtkWidget *value;

            value = gtk_button_new_with_label(entangle_control_label(control));
            if (entangle_control_get_readonly(control))
                gtk_widget_set_sensitive(value, FALSE);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (ENTANGLE_IS_CONTROL_CHOICE(control)) {
            GtkCellRenderer *cell;
            GtkListStore *store;
            GtkWidget *label;
            GtkWidget *value;
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

            label = gtk_label_new(entangle_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, entangle_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            store = gtk_list_store_new(1, G_TYPE_STRING);
            value = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
            g_object_unref (store);

            cell = gtk_cell_renderer_text_new();
            gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(value), cell, TRUE);
            gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(value), cell,
                                           "text", 0,
                                           NULL);
            
            g_object_get(control, "value", &text, NULL);
            for (int n = 0 ; n < entangle_control_choice_entry_count(ENTANGLE_CONTROL_CHOICE(control)) ; n++) {
                GtkTreeIter iter;
                if (strcmp(text, entangle_control_choice_entry_get(ENTANGLE_CONTROL_CHOICE(control), n)) == 0)
                    active = n;
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0,
                                   entangle_control_choice_entry_get(ENTANGLE_CONTROL_CHOICE(control), n),
                                   -1);
            }

            if (entangle_control_get_readonly(control))
                gtk_widget_set_sensitive(value, FALSE);
            gtk_combo_box_set_active(GTK_COMBO_BOX(value), active);

            g_object_set_data(G_OBJECT(value), "control", control);
            g_signal_connect(value, "changed",
                             G_CALLBACK(do_update_control_combo), panel);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (ENTANGLE_IS_CONTROL_DATE(control)) {
            GtkWidget *label;
            GtkWidget *value;
            int date;

            label = gtk_label_new(entangle_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, entangle_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            value = gtk_entry_new();
            g_object_get(control, "value", &date, NULL);
            if (entangle_control_get_readonly(control))
                gtk_widget_set_sensitive(value, FALSE);
            //gtk_entry_set_text(GTK_ENTRY(value), text);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (ENTANGLE_IS_CONTROL_RANGE(control)) {
            GtkWidget *label;
            GtkWidget *value;
            float offset;

            label = gtk_label_new(entangle_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, entangle_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            value = gtk_hscale_new_with_range(entangle_control_range_get_min(ENTANGLE_CONTROL_RANGE(control)),
                                              entangle_control_range_get_max(ENTANGLE_CONTROL_RANGE(control)),
                                              entangle_control_range_get_step(ENTANGLE_CONTROL_RANGE(control)));
            g_object_get(control, "value", &offset, NULL);
            gtk_range_set_value(GTK_RANGE(value), offset);
            if (entangle_control_get_readonly(control))
                gtk_widget_set_sensitive(value, FALSE);
            g_object_set_data(G_OBJECT(value), "control", control);
            g_signal_connect(value, "change-value",
                             G_CALLBACK(do_update_control_range), panel);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (ENTANGLE_IS_CONTROL_TEXT(control)) {
            GtkWidget *label;
            GtkWidget *value;
            const char *text;

            label = gtk_label_new(entangle_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, entangle_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            value = gtk_entry_new();
            g_object_get(control, "value", &text, NULL);
            gtk_entry_set_text(GTK_ENTRY(value), text);
            if (entangle_control_get_readonly(control))
                gtk_widget_set_sensitive(value, FALSE);
            g_object_set_data(G_OBJECT(value), "control", control);
            g_signal_connect(value, "focus-out-event",
                             G_CALLBACK(do_update_control_entry), panel);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (ENTANGLE_IS_CONTROL_TOGGLE(control)) {
            GtkWidget *value;
            gboolean active;

            value = gtk_check_button_new_with_label(entangle_control_label(control));
            g_object_get(control, "value", &active, NULL);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(value), active);
            if (entangle_control_get_readonly(control))
                gtk_widget_set_sensitive(value, FALSE);
            g_object_set_data(G_OBJECT(value), "control", control);
            g_signal_connect(value, "toggled",
                             G_CALLBACK(do_update_control_toggle), panel);
            gtk_container_add(GTK_CONTAINER(box), value);
        }
    }
}

static void do_setup_camera(EntangleControlPanel *panel)
{
    EntangleControlPanelPrivate *priv = panel->priv;
    EntangleControlGroup *grp;

    gtk_container_foreach(GTK_CONTAINER(panel), do_control_remove, panel);

    if (!priv->camera)
        return;

    grp = entangle_camera_get_controls(priv->camera);

    if (!grp)
        return;

    do_setup_control_group(panel, GTK_VBOX(panel), grp);
    gtk_widget_show_all(GTK_WIDGET(panel));
    g_object_unref(grp);
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

    ENTANGLE_DEBUG("Set prop on control panel %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            entangle_control_panel_set_camera(panel, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_control_panel_finalize (GObject *object)
{
    EntangleControlPanel *panel = ENTANGLE_CONTROL_PANEL(object);
    EntangleControlPanelPrivate *priv = panel->priv;

    if (priv->camera)
        g_object_unref(priv->camera);
    if (priv->cameraScheduler)
        g_object_unref(priv->cameraScheduler);

    G_OBJECT_CLASS (entangle_control_panel_parent_class)->finalize (object);
}


static void entangle_control_panel_class_init(EntangleControlPanelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_control_panel_finalize;
    object_class->get_property = entangle_control_panel_get_property;
    object_class->set_property = entangle_control_panel_set_property;

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to managed",
                                                        ENTANGLE_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
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

EntangleControlPanel *entangle_control_panel_new(void)
{
    return ENTANGLE_CONTROL_PANEL(g_object_new(ENTANGLE_TYPE_CONTROL_PANEL, NULL));
}


static void entangle_control_panel_init(EntangleControlPanel *panel)
{
    EntangleControlPanelPrivate *priv;

    priv = panel->priv = ENTANGLE_CONTROL_PANEL_GET_PRIVATE(panel);
    memset(priv, 0, sizeof *priv);

    //gtk_container_set_border_width(GTK_CONTAINER(panel), 6);
}


void entangle_control_panel_set_camera(EntangleControlPanel *panel,
                                       EntangleCamera *cam)
{
    EntangleControlPanelPrivate *priv = panel->priv;

    if (priv->camera)
        g_object_unref(priv->camera);
    priv->camera = cam;
    if (priv->camera)
        g_object_ref(priv->camera);
    do_setup_camera(panel);
}


EntangleCamera *entangle_control_panel_get_camera(EntangleControlPanel *panel)
{
    EntangleControlPanelPrivate *priv = panel->priv;

    return priv->camera;
}

void entangle_control_panel_set_camera_scheduler(EntangleControlPanel *panel,
                                                 EntangleCameraScheduler *sched)
{
    EntangleControlPanelPrivate *priv = panel->priv;

    if (priv->cameraScheduler)
        g_object_unref(priv->cameraScheduler);
    priv->cameraScheduler = sched;
    if (priv->cameraScheduler)
        g_object_ref(priv->cameraScheduler);
}

EntangleCameraScheduler *entangle_control_panel_get_camera_scheduler(EntangleControlPanel *panel)
{
    EntangleControlPanelPrivate *priv = panel->priv;

    return priv->cameraScheduler;
}

gboolean entangle_control_panel_get_has_controls(EntangleControlPanel *panel)
{
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
