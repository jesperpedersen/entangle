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

#include <config.h>

#include <string.h>

#include "capa-debug.h"
#include "capa-control-panel.h"
#include "capa-control-button.h"
#include "capa-control-choice.h"
#include "capa-control-date.h"
#include "capa-control-group.h"
#include "capa-control-range.h"
#include "capa-control-text.h"
#include "capa-control-toggle.h"

#define CAPA_CONTROL_PANEL_GET_PRIVATE(obj)                             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CONTROL_PANEL, CapaControlPanelPrivate))

struct _CapaControlPanelPrivate {
    CapaCamera *camera;
};

G_DEFINE_TYPE(CapaControlPanel, capa_control_panel, GTK_TYPE_VBOX);

enum {
    PROP_O,
    PROP_CAMERA,
};


static void do_control_remove(GtkWidget *widget,
                              gpointer data)
{
    CapaControlPanel *panel = data;

    gtk_container_remove(GTK_CONTAINER(panel), widget);
}

static void do_update_control_entry(GtkWidget *widget,
                                    GdkEventFocus *ev G_GNUC_UNUSED,
                                    gpointer data)
{
    CapaControlText *control = data;
    const char *text;

    text = gtk_entry_get_text(GTK_ENTRY(widget));

    CAPA_DEBUG("entry [%s]", text);
    g_object_set(G_OBJECT(control), "value", text, NULL);
}

static void do_update_control_range(GtkRange *widget G_GNUC_UNUSED,
                                    GtkScrollType scroll G_GNUC_UNUSED,
                                    gdouble value,
                                    gpointer data)
{
    CapaControlText *control = data;

    CAPA_DEBUG("range [%lf]", value);
    g_object_set(G_OBJECT(control), "value", (float)value, NULL);
}

static void do_update_control_combo(GtkComboBox *widget,
                                    gpointer data)
{
    CapaControlChoice *control = data;
    char *text;

    text = gtk_combo_box_get_active_text(widget);
    CAPA_DEBUG("combo [%s]", text);
    g_object_set(G_OBJECT(control), "value", text, NULL);

    g_free(text);
}

static void do_update_control_toggle(GtkToggleButton *widget,
                                     gpointer data)
{
    CapaControlChoice *control = data;
    gboolean active;

    active = gtk_toggle_button_get_active(widget);
    CAPA_DEBUG("toggle [%d]", active);
    g_object_set(G_OBJECT(control), "value", active, NULL);
}

static void do_setup_control_group(CapaControlPanel *panel,
                                   GtkVBox *box,
                                   CapaControlGroup *grp)
{
    int i;

    for (i = 0 ; i < capa_control_group_count(grp) ; i++) {
        CapaControl *control = capa_control_group_get(grp, i);

        if (CAPA_IS_CONTROL_GROUP(control)) {
            GtkWidget *frame = gtk_expander_new(capa_control_label(control));
            //GtkWidget *frame = gtk_frame_new(capa_control_label(control));
            GtkWidget *subbox = gtk_vbox_new(FALSE, 6);

            gtk_container_add(GTK_CONTAINER(box), frame);
            //gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
            gtk_expander_set_expanded(GTK_EXPANDER(frame), TRUE);
            gtk_container_set_border_width(GTK_CONTAINER(subbox), 6);
            //g_object_unref(G_OBJECT(frame));

            gtk_container_add(GTK_CONTAINER(frame), subbox);
            //g_object_unref(G_OBJECT(subbox));

            do_setup_control_group(panel, GTK_VBOX(subbox), CAPA_CONTROL_GROUP(control));
        } else if (CAPA_IS_CONTROL_BUTTON(control)) {
            GtkWidget *value;

            value = gtk_button_new_with_label(capa_control_label(control));
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (CAPA_IS_CONTROL_CHOICE(control)) {
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

            label = gtk_label_new(capa_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, capa_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            value = gtk_combo_box_new_text();
            g_object_get(G_OBJECT(control), "value", &text, NULL);
            for (int n = 0 ; n < capa_control_choice_entry_count(CAPA_CONTROL_CHOICE(control)) ; n++) {
                if (strcmp(text, capa_control_choice_entry_get(CAPA_CONTROL_CHOICE(control), n)) == 0)
                    active = n;
                gtk_combo_box_append_text(GTK_COMBO_BOX(value),
                                          capa_control_choice_entry_get(CAPA_CONTROL_CHOICE(control), n));
            }

            gtk_combo_box_set_active(GTK_COMBO_BOX(value), active);
            g_signal_connect(G_OBJECT(value), "changed",
                             G_CALLBACK(do_update_control_combo), control);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (CAPA_IS_CONTROL_DATE(control)) {
            GtkWidget *label;
            GtkWidget *value;
            int date;

            label = gtk_label_new(capa_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, capa_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            value = gtk_entry_new();
            g_object_get(G_OBJECT(control), "value", &date, NULL);
            //gtk_entry_set_text(GTK_ENTRY(value), text);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (CAPA_IS_CONTROL_RANGE(control)) {
            GtkWidget *label;
            GtkWidget *value;
            float offset;

            label = gtk_label_new(capa_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, capa_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            value = gtk_hscale_new_with_range(capa_control_range_get_min(CAPA_CONTROL_RANGE(control)),
                                              capa_control_range_get_max(CAPA_CONTROL_RANGE(control)),
                                              capa_control_range_get_step(CAPA_CONTROL_RANGE(control)));
            g_object_get(G_OBJECT(control), "value", &offset, NULL);
            gtk_range_set_value(GTK_RANGE(value), offset);
            g_signal_connect(G_OBJECT(value), "change-value",
                             G_CALLBACK(do_update_control_range), control);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (CAPA_IS_CONTROL_TEXT(control)) {
            GtkWidget *label;
            GtkWidget *value;
            const char *text;

            label = gtk_label_new(capa_control_label(control));
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_widget_set_tooltip_text(label, capa_control_info(control));
            gtk_container_add(GTK_CONTAINER(box), label);

            value = gtk_entry_new();
            g_object_get(G_OBJECT(control), "value", &text, NULL);
            gtk_entry_set_text(GTK_ENTRY(value), text);
            g_signal_connect(G_OBJECT(value), "focus-out-event",
                             G_CALLBACK(do_update_control_entry), control);
            gtk_container_add(GTK_CONTAINER(box), value);
        } else if (CAPA_IS_CONTROL_TOGGLE(control)) {
            GtkWidget *value;
            gboolean active;

            value = gtk_check_button_new_with_label(capa_control_label(control));
            g_object_get(G_OBJECT(control), "value", &active, NULL);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(value), active);
            g_signal_connect(G_OBJECT(value), "toggled",
                             G_CALLBACK(do_update_control_toggle), control);
            gtk_container_add(GTK_CONTAINER(box), value);
        }
    }
}

static void do_setup_camera(CapaControlPanel *panel)
{
    CapaControlPanelPrivate *priv = panel->priv;
    CapaControlGroup *grp;

    gtk_container_foreach(GTK_CONTAINER(panel), do_control_remove, panel);

    if (!priv->camera)
        return;

    grp = capa_camera_controls(priv->camera);

    do_setup_control_group(panel, GTK_VBOX(panel), grp);
    gtk_widget_show_all(GTK_WIDGET(panel));
}

static void capa_control_panel_get_property(GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
    CapaControlPanel *panel = CAPA_CONTROL_PANEL(object);
    CapaControlPanelPrivate *priv = panel->priv;

    switch (prop_id)
        {
        case PROP_CAMERA:
            g_value_set_object(value, priv->camera);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_control_panel_set_property(GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
    CapaControlPanel *panel = CAPA_CONTROL_PANEL(object);
    CapaControlPanelPrivate *priv = panel->priv;

    CAPA_DEBUG("Set prop on control panel %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            if (priv->camera)
                g_object_unref(G_OBJECT(priv->camera));
            priv->camera = g_value_get_object(value);
            if (priv->camera)
                g_object_ref(G_OBJECT(priv->camera));
            do_setup_camera(panel);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_control_panel_finalize (GObject *object)
{
    CapaControlPanel *panel = CAPA_CONTROL_PANEL(object);
    CapaControlPanelPrivate *priv = panel->priv;

    if (priv->camera)
        g_object_unref(G_OBJECT(priv->camera));

    G_OBJECT_CLASS (capa_control_panel_parent_class)->finalize (object);
}


static void capa_control_panel_class_init(CapaControlPanelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = capa_control_panel_finalize;
    object_class->get_property = capa_control_panel_get_property;
    object_class->set_property = capa_control_panel_set_property;

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to managed",
                                                        CAPA_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(CapaControlPanelPrivate));
}

CapaControlPanel *capa_control_panel_new(void)
{
    return CAPA_CONTROL_PANEL(g_object_new(CAPA_TYPE_CONTROL_PANEL, NULL));
}


static void capa_control_panel_init(CapaControlPanel *panel)
{
    CapaControlPanelPrivate *priv;

    priv = panel->priv = CAPA_CONTROL_PANEL_GET_PRIVATE(panel);
    memset(priv, 0, sizeof *priv);

    //gtk_container_set_border_width(GTK_CONTAINER(panel), 6);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
