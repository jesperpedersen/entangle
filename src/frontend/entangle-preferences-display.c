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

#include <config.h>

#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <unistd.h>
#include <math.h>

#include <libpeas-gtk/peas-gtk.h>

#include "entangle-debug.h"
#include "entangle-preferences-display.h"
#include "entangle-camera-picker.h"
#include "entangle-camera-manager.h"
#include "entangle-image-display.h"
#include "entangle-window.h"


#define ENTANGLE_PREFERENCES_DISPLAY_GET_PRIVATE(obj)                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PREFERENCES_DISPLAY, EntanglePreferencesDisplayPrivate))

static void entangle_preferences_display_refresh(EntanglePreferencesDisplay *preferences);

struct _EntanglePreferencesDisplayPrivate {
    PeasGtkPluginManager *pluginManager;
    gulong prefsID;
    GtkBuilder *builder;
};

static void entangle_preferences_display_window_interface_init(gpointer g_iface,
                                                               gpointer iface_data);

G_DEFINE_TYPE_EXTENDED(EntanglePreferencesDisplay, entangle_preferences_display, GTK_TYPE_DIALOG, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_WINDOW, entangle_preferences_display_window_interface_init));

enum {
    PROP_0,
};

void do_preferences_close(GtkButton *src, EntanglePreferencesDisplay *preferences);
gboolean do_preferences_delete(GtkWidget *src,
                               GdkEvent *ev,
                               EntanglePreferencesDisplay *preferences);
void do_page_changed(GtkTreeSelection *selection,
                     EntanglePreferencesDisplay *preferences);
void do_cms_enabled_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_cms_rgb_profile_file_set(GtkFileChooserButton *src, EntanglePreferencesDisplay *display);
void do_cms_monitor_profile_file_set(GtkFileChooserButton *src, EntanglePreferencesDisplay *display);
void do_cms_detect_system_profile_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_cms_rendering_intent_changed(GtkComboBox *src, EntanglePreferencesDisplay *display);

void do_interface_auto_connect_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_interface_screen_blank_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_interface_histogram_linear_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);

void do_capture_filename_pattern_changed(GtkEntry *src, EntanglePreferencesDisplay *display);
void do_capture_continuous_preview_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_capture_electronic_shutter(GtkToggleButton *src, EntanglePreferencesDisplay *preferences);
void do_capture_delete_file_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_capture_sync_clock_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);

void do_img_mask_enabled_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_img_aspect_ratio_changed(GtkComboBox *src, EntanglePreferencesDisplay *display);
void do_img_mask_opacity_changed(GtkSpinButton *src, EntanglePreferencesDisplay *display);
void do_img_focus_point_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_img_grid_lines_changed(GtkComboBox *src, EntanglePreferencesDisplay *display);
void do_img_embedded_preview_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_img_onion_skin_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_img_onion_layers_changed(GtkSpinButton *src, EntanglePreferencesDisplay *display);
void do_img_background_changed(GtkColorButton *src, EntanglePreferencesDisplay *display);
void do_img_highlight_changed(GtkColorButton *src, EntanglePreferencesDisplay *display);

static EntanglePreferences *entangle_preferences_display_get_preferences(EntanglePreferencesDisplay *preferences)
{
    GtkApplication *gapp = gtk_window_get_application(GTK_WINDOW(preferences));
    EntangleApplication *app = ENTANGLE_APPLICATION(gapp);
    return entangle_application_get_preferences(app);
}


static void entangle_preferences_display_notify(GObject *object,
                                                GParamSpec *spec,
                                                gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(data));

    EntanglePreferencesDisplay *preferences = ENTANGLE_PREFERENCES_DISPLAY(data);
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, spec->name));

    ENTANGLE_DEBUG("Internal display Set %p %s", object, spec->name);
    if (strcmp(spec->name, "cms-enabled") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "cms-detect-system-profile") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "cms-rgb-profile") == 0) {
        EntangleColourProfile *profile;
        const gchar *oldvalue;
        const gchar *newvalue;

        g_object_get(object, spec->name, &profile, NULL);

        newvalue = profile ? entangle_colour_profile_filename(profile) : NULL;
        oldvalue = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(tmp));

        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), newvalue);

        if (profile)
            g_object_unref(profile);
    } else if (strcmp(spec->name, "cms-monitor-profile") == 0) {
        EntangleColourProfile *profile;
        const gchar *oldvalue;
        const gchar *newvalue;

        g_object_get(object, spec->name, &profile, NULL);

        newvalue = profile ? entangle_colour_profile_filename(profile) : NULL;
        oldvalue = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(tmp));

        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), newvalue);

        if (profile)
            g_object_unref(profile);
    } else if (strcmp(spec->name, "cms-rendering-intent") == 0) {
        int newvalue;
        int oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_combo_box_get_active(GTK_COMBO_BOX(tmp));

        if (oldvalue != newvalue)
            gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), newvalue);
    } else if (strcmp(spec->name, "interface-auto-connect") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "interface-screen-blank") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "interface-histogram-linear") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "capture-filename-pattern") == 0) {
        gchar *newvalue;
        const gchar *oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);

        oldvalue = gtk_entry_get_text(GTK_ENTRY(tmp));
        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_entry_set_text(GTK_ENTRY(tmp), newvalue);

        g_free(newvalue);
    } else if (strcmp(spec->name, "capture-continuous-preview") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "capture-electronic-shutter") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "capture-delete-file") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "capture-sync-clock") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "img-mask-enabled") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "img-aspect-ratio") == 0) {
        gchar *newvalue;
        const gchar *oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);

        oldvalue = gtk_combo_box_get_active_id(GTK_COMBO_BOX(tmp));
        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_combo_box_set_active_id(GTK_COMBO_BOX(tmp), newvalue);

        g_free(newvalue);
    } else if (strcmp(spec->name, "img-mask-opacity") == 0) {
        GtkAdjustment *adjust = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmp));
        gint newvalue;
        gfloat oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_adjustment_get_value(adjust);

        if (fabs(newvalue - oldvalue)  > 0.0005)
            gtk_adjustment_set_value(adjust, newvalue);
    } else if (strcmp(spec->name, "img-focus-point") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "img-grid-lines") == 0) {
        gint newvalue;
        gint oldvalue;
        const gchar *oldid;
        GEnumClass *enum_class;
        GEnumValue *enum_value;

        enum_class = g_type_class_ref(ENTANGLE_TYPE_IMAGE_DISPLAY_GRID);

        g_object_get(object, spec->name, &newvalue, NULL);
        oldid = gtk_combo_box_get_active_id(GTK_COMBO_BOX(tmp));

        oldvalue = ENTANGLE_IMAGE_DISPLAY_GRID_NONE;
        if (oldid) {
            enum_value = g_enum_get_value_by_nick(enum_class, oldid);
            if (enum_value != NULL)
                oldvalue = enum_value->value;
        }

        if (newvalue != oldvalue) {
            enum_value = g_enum_get_value(enum_class, newvalue);
            if (enum_value != NULL)
                gtk_combo_box_set_active_id(GTK_COMBO_BOX(tmp), enum_value->value_nick);
            else
                gtk_combo_box_set_active_id(GTK_COMBO_BOX(tmp), "none");
        }

        g_type_class_unref(enum_class);
    } else if (strcmp(spec->name, "img-embedded-preview") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "img-onion-skin") == 0) {
        gboolean newvalue;
        gboolean oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "img-onion-layers") == 0) {
        GtkAdjustment *adjust = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmp));
        gint newvalue;
        gfloat oldvalue;

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_adjustment_get_value(adjust);

        if (fabs(newvalue - oldvalue)  > 0.0005)
            gtk_adjustment_set_value(adjust, newvalue);
    }
}

static void do_entangle_preferences_display_set_app(GObject *object,
                                                    GParamSpec *spec G_GNUC_UNUSED)
{
    EntanglePreferencesDisplay *preferences = ENTANGLE_PREFERENCES_DISPLAY(object);
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    EntanglePreferences *prefs;
    PeasEngine *pluginEngine;
    GtkWidget *panel;
    EntangleApplication *app;

    app = ENTANGLE_APPLICATION(gtk_window_get_application(GTK_WINDOW(preferences)));

    panel = GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugins-panel"));
    entangle_preferences_display_refresh(preferences);
    prefs = entangle_preferences_display_get_preferences(preferences);
    priv->prefsID = g_signal_connect(prefs,
                                     "notify",
                                     G_CALLBACK(entangle_preferences_display_notify),
                                     object);
    pluginEngine = entangle_application_get_plugin_engine(app);
    priv->pluginManager = PEAS_GTK_PLUGIN_MANAGER(peas_gtk_plugin_manager_new(pluginEngine));
    gtk_container_add(GTK_CONTAINER(panel), GTK_WIDGET(priv->pluginManager));
    gtk_widget_show_all(GTK_WIDGET(priv->pluginManager));
}


static void entangle_preferences_display_refresh(EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *tmp;
    EntangleColourProfile *profile;
    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    const gchar *ratio;
    GtkAdjustment *adjust;
    gboolean hasContinuousPreview;
    gboolean hasRatio;
    gboolean hasOnion;

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-enabled"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_cms_get_enabled(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-detect-system-profile"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_cms_get_detect_system_profile(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-rgb-profile"));
    profile = entangle_preferences_cms_get_rgb_profile(prefs);
    if (profile) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), entangle_colour_profile_filename(profile));
        g_object_unref(profile);
    }

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-monitor-profile"));
    profile = entangle_preferences_cms_get_monitor_profile(prefs);
    if (profile) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), entangle_colour_profile_filename(profile));
        g_object_unref(profile);
    }

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-rendering-intent"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), entangle_preferences_cms_get_rendering_intent(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "interface-auto-connect"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_interface_get_auto_connect(prefs));
    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "interface-screen-blank"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_interface_get_screen_blank(prefs));
    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "interface-histogram-linear"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp),
                                 entangle_preferences_interface_get_histogram_linear(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-filename-pattern"));
    gtk_entry_set_text(GTK_ENTRY(tmp), entangle_preferences_capture_get_filename_pattern(prefs));

    hasContinuousPreview = entangle_preferences_capture_get_continuous_preview(prefs);
    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-continuous-preview"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), hasContinuousPreview);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-electronic-shutter"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_capture_get_electronic_shutter(prefs));
    gtk_widget_set_sensitive(tmp, hasContinuousPreview);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-delete-file"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_capture_get_delete_file(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-sync-clock"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_capture_get_sync_clock(prefs));

    ratio = entangle_preferences_img_get_aspect_ratio(prefs);
    hasRatio = entangle_preferences_img_get_mask_enabled(prefs);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-mask-enabled"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), hasRatio);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-aspect-ratio"));
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(tmp), ratio);
    gtk_widget_set_sensitive(tmp, hasRatio);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-aspect-ratio-label"));
    gtk_widget_set_sensitive(tmp, hasRatio);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-mask-opacity"));
    gtk_widget_set_sensitive(tmp, hasRatio);
    adjust = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmp));
    gtk_adjustment_set_value(adjust,
                             entangle_preferences_img_get_mask_opacity(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-mask-opacity-label"));
    gtk_widget_set_sensitive(tmp, hasRatio);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-focus-point"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp),
                                 entangle_preferences_img_get_focus_point(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-grid-lines"));
    GEnumClass *enum_class = g_type_class_ref(ENTANGLE_TYPE_IMAGE_DISPLAY_GRID);
    GEnumValue *enum_value = g_enum_get_value(enum_class,
                                              entangle_preferences_img_get_grid_lines(prefs));
    g_type_class_unref(enum_class);

    if (enum_value != NULL)
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(tmp), enum_value->value_nick);
    else
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(tmp), NULL);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-embedded-preview"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp),
                                 entangle_preferences_img_get_embedded_preview(prefs));

    hasOnion = entangle_preferences_img_get_onion_skin(prefs);
    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-onion-skin"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), hasOnion);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-onion-layers"));
    gtk_widget_set_sensitive(tmp, hasOnion);
    adjust = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmp));
    gtk_adjustment_set_value(adjust,
                             entangle_preferences_img_get_onion_layers(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-onion-layers-label"));
    gtk_widget_set_sensitive(tmp, hasOnion);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-background"));
    GdkRGBA gbg;
    gchar *bg = entangle_preferences_img_get_background(prefs);
    gdk_rgba_parse(&gbg, bg);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(tmp), &gbg);
    g_free(bg);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-highlight"));
    GdkRGBA ghl;
    gchar *hl = entangle_preferences_img_get_highlight(prefs);
    gdk_rgba_parse(&ghl, hl);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(tmp), &ghl);
    g_free(hl);
}


static void entangle_preferences_display_finalize(GObject *object)
{
    EntanglePreferencesDisplay *preferences = ENTANGLE_PREFERENCES_DISPLAY(object);
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);

    ENTANGLE_DEBUG("Finalize preferences");

    g_signal_handler_disconnect(prefs, priv->prefsID);

    if (priv->builder)
        g_object_unref(priv->builder);

    G_OBJECT_CLASS(entangle_preferences_display_parent_class)->finalize(object);
}

static void do_entangle_preferences_display_set_builder(EntangleWindow *window,
                                                        GtkBuilder *builder);
static GtkBuilder *do_entangle_preferences_display_get_builder(EntangleWindow *window);

static void entangle_preferences_display_window_interface_init(gpointer g_iface,
                                                               gpointer iface_data G_GNUC_UNUSED)
{
    EntangleWindowInterface *iface = g_iface;
    iface->set_builder = do_entangle_preferences_display_set_builder;
    iface->get_builder = do_entangle_preferences_display_get_builder;
}

static void entangle_preferences_display_class_init(EntanglePreferencesDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_preferences_display_finalize;

    g_type_class_add_private(klass, sizeof(EntanglePreferencesDisplayPrivate));
}


EntanglePreferencesDisplay *entangle_preferences_display_new(void)
{
    return ENTANGLE_PREFERENCES_DISPLAY(entangle_window_new(ENTANGLE_TYPE_PREFERENCES_DISPLAY,
                                                            GTK_TYPE_DIALOG,
                                                            "preferences-display"));
}

void do_preferences_close(GtkButton *src G_GNUC_UNUSED, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    gtk_widget_hide(GTK_WIDGET(preferences));
}


gboolean do_preferences_delete(GtkWidget *src,
                               GdkEvent *ev G_GNUC_UNUSED,
                               EntanglePreferencesDisplay *preferences)
{
    g_return_val_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences), TRUE);

    ENTANGLE_DEBUG("preferences delete");
    gtk_widget_hide(src);
    return TRUE;
}

void do_page_changed(GtkTreeSelection *selection,
                     EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *list;
    GtkTreeIter iter;
    gboolean selected;
    GValue val;
    int page;
    GtkWidget *notebook;

    ENTANGLE_DEBUG("select page");

    list = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences-switch"));

    selected = gtk_tree_selection_get_selected(selection, NULL, &iter);
    if (!selected)
        return;

    memset(&val, 0, sizeof val);
    gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(list)),
                             &iter, 0, &val);

    notebook = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences-notebook"));
    page = g_value_get_int(&val);
    if (page >= 0)
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
}


void do_cms_enabled_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *rgbProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-rgb-profile"));
    GtkWidget *monitorProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-monitor-profile"));
    GtkWidget *systemProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-detect-system-profile"));
    GtkWidget *renderIntent = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-rendering-intent"));
    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);

    entangle_preferences_cms_set_enabled(prefs, enabled);
    gtk_widget_set_sensitive(rgbProfile, enabled);
    gtk_widget_set_sensitive(systemProfile, enabled);
    gtk_widget_set_sensitive(renderIntent, enabled);
    gtk_widget_set_sensitive(monitorProfile, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(systemProfile)));
}

void do_cms_rgb_profile_file_set(GtkFileChooserButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    EntangleColourProfile *profile = entangle_colour_profile_new_file(filename);

    entangle_preferences_cms_set_rgb_profile(prefs, profile);
    g_free(filename);
    g_object_unref(profile);
}

void do_cms_monitor_profile_file_set(GtkFileChooserButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    EntangleColourProfile *profile = entangle_colour_profile_new_file(filename);

    entangle_preferences_cms_set_monitor_profile(prefs, profile);
    g_free(filename);
    g_object_unref(profile);
}

void do_cms_detect_system_profile_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *monitorProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-monitor-profile"));

    entangle_preferences_cms_set_detect_system_profile(prefs, enabled);
    gtk_widget_set_sensitive(monitorProfile, !enabled);
}

void do_cms_rendering_intent_changed(GtkComboBox *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    int option = gtk_combo_box_get_active(src);

    if (option < 0)
        option = ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL;
    entangle_preferences_cms_set_rendering_intent(prefs, option);
}


void do_capture_filename_pattern_changed(GtkEntry *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    const char *text = gtk_entry_get_text(src);

    entangle_preferences_capture_set_filename_pattern(prefs, text);
}


void do_capture_continuous_preview_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-electronic-shutter"));

    gtk_widget_set_sensitive(widget, enabled);

    entangle_preferences_capture_set_continuous_preview(prefs, enabled);
}


void do_capture_electronic_shutter(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_capture_set_electronic_shutter(prefs, enabled);
}


void do_interface_auto_connect_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_interface_set_auto_connect(prefs, enabled);
}


void do_interface_screen_blank_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_interface_set_screen_blank(prefs, enabled);
}


void do_interface_histogram_linear_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_interface_set_histogram_linear(prefs, enabled);
}


void do_capture_delete_file_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_capture_set_delete_file(prefs, enabled);
}


void do_capture_sync_clock_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_capture_set_sync_clock(prefs, enabled);
}


void do_img_mask_enabled_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *aspect = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-aspect-ratio"));
    GtkWidget *aspectLbl = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-aspect-ratio-label"));
    GtkWidget *opacity = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-mask-opacity"));
    GtkWidget *opacityLbl = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-mask-opacity-label"));

    gtk_widget_set_sensitive(aspect, enabled);
    gtk_widget_set_sensitive(aspectLbl, enabled);
    gtk_widget_set_sensitive(opacity, enabled);
    gtk_widget_set_sensitive(opacityLbl, enabled);

    entangle_preferences_img_set_mask_enabled(prefs, enabled);
}


void do_img_aspect_ratio_changed(GtkComboBox *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    const gchar *ratio = gtk_combo_box_get_active_id(src);

    if (ratio == NULL)
        ratio = "";

    entangle_preferences_img_set_aspect_ratio(prefs, ratio);
}


void do_img_mask_opacity_changed(GtkSpinButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    GtkAdjustment *adjust = gtk_spin_button_get_adjustment(src);

    entangle_preferences_img_set_mask_opacity(prefs,
                                              gtk_adjustment_get_value(adjust));
}


void do_img_focus_point_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_img_set_focus_point(prefs, enabled);
}


void do_img_grid_lines_changed(GtkComboBox *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    const gchar *id = gtk_combo_box_get_active_id(src);
    EntangleImageDisplayGrid grid = ENTANGLE_IMAGE_DISPLAY_GRID_NONE;

    if (id) {
        GEnumClass *enum_class = g_type_class_ref(ENTANGLE_TYPE_IMAGE_DISPLAY_GRID);
        GEnumValue *enum_value = g_enum_get_value_by_nick(enum_class, id);
        g_type_class_unref(enum_class);

        if (enum_value != NULL)
            grid = enum_value->value;
    }

    entangle_preferences_img_set_grid_lines(prefs, grid);
}


void do_img_embedded_preview_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);

    entangle_preferences_img_set_embedded_preview(prefs, enabled);
}


void do_img_onion_skin_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *layers = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-onion-layers"));
    GtkWidget *layersLbl = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-onion-layers-label"));

    gtk_widget_set_sensitive(layers, enabled);
    gtk_widget_set_sensitive(layersLbl, enabled);

    entangle_preferences_img_set_onion_skin(prefs, enabled);
}


void do_img_onion_layers_changed(GtkSpinButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);
    GtkAdjustment *adjust = gtk_spin_button_get_adjustment(src);

    entangle_preferences_img_set_onion_layers(prefs,
                                              gtk_adjustment_get_value(adjust));
}

void do_img_background_changed(GtkColorButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);

    GdkRGBA gbg;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(src), &gbg);
    gchar *bg = gdk_rgba_to_string(&gbg);
    entangle_preferences_img_set_background(prefs, bg);
    g_free(bg);
}

void do_img_highlight_changed(GtkColorButton *src, EntanglePreferencesDisplay *preferences)
{
    g_return_if_fail(ENTANGLE_IS_PREFERENCES_DISPLAY(preferences));

    EntanglePreferences *prefs = entangle_preferences_display_get_preferences(preferences);

    GdkRGBA ghl;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(src), &ghl);
    gchar *hl = gdk_rgba_to_string(&ghl);
    entangle_preferences_img_set_highlight(prefs, hl);
    g_free(hl);
}


static void do_entangle_preferences_display_set_builder(EntangleWindow *win,
                                                        GtkBuilder *builder)
{
    EntanglePreferencesDisplay *preferences = ENTANGLE_PREFERENCES_DISPLAY(win);
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *notebook;
    GtkWidget *box;
    GtkWidget *image;
    GtkListStore *list;
    GtkWidget *tree;
    GtkTreeIter iter;
    GtkCellRenderer *cellImage;
    GtkCellRenderer *cellText;
    GtkTreeViewColumn *colImage;
    GtkTreeViewColumn *colText;
    GtkTreeSelection *selection;
    GtkFileChooser *chooser;
    GtkFileFilter *allFilter;
    GtkFileFilter *iccFilter;
    GtkComboBox *aspectRatio;
    GtkComboBox *gridLines;
    GtkIconTheme *theme = gtk_icon_theme_get_default();

    priv->builder = g_object_ref(builder);

    g_signal_connect(preferences, "delete-event",
                     G_CALLBACK(do_preferences_delete), preferences);

    notebook = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences-notebook"));
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

    //gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), 2);

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "interface-box"));
    gtk_widget_set_state_flags(box, GTK_STATE_FLAG_SELECTED, TRUE);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "interface-image"));
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "entangle-interface", GTK_ICON_SIZE_DIALOG);

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-box"));
    gtk_widget_set_state_flags(box, GTK_STATE_FLAG_SELECTED, TRUE);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-image"));
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "entangle-color-management", GTK_ICON_SIZE_DIALOG);

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-box"));
    gtk_widget_set_state_flags(box, GTK_STATE_FLAG_SELECTED, TRUE);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "capture-image"));
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "entangle-capture", GTK_ICON_SIZE_DIALOG);

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugins-box"));
    gtk_widget_set_state_flags(box, GTK_STATE_FLAG_SELECTED, TRUE);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugins-image"));
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "entangle-plugins", GTK_ICON_SIZE_DIALOG);

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-box"));
    gtk_widget_set_state_flags(box, GTK_STATE_FLAG_SELECTED, TRUE);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "img-image"));
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "entangle-imageviewer", GTK_ICON_SIZE_DIALOG);

    list = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF, -1);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, 3,
                       1, _("Interface"),
                       2, gtk_icon_theme_load_icon(theme, "entangle-interface", 22, 0, NULL),
                       -1);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, 4,
                       1, _("Image Viewer"),
                       2, gtk_icon_theme_load_icon(theme, "entangle-imageviewer", 22, 0, NULL),
                       -1);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, 0,
                       1, _("Capture"),
                       2, gtk_icon_theme_load_icon(theme, "entangle-capture", 22, 0, NULL),
                       -1);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, 1,
                       1, _("Color Management"),
                       2, gtk_icon_theme_load_icon(theme, "entangle-color-management", 22, 0, NULL),
                       -1);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, 2,
                       1, _("Plugins"),
                       2, gtk_icon_theme_load_icon(theme, "entangle-plugins", 22, 0, NULL),
                       -1);

    cellText = gtk_cell_renderer_text_new();
    cellImage = gtk_cell_renderer_pixbuf_new();

    colText = gtk_tree_view_column_new_with_attributes("Label", cellText, "text", 1, NULL);
    colImage = gtk_tree_view_column_new_with_attributes("Icon", cellImage, "pixbuf", 2, NULL);

    g_object_set(colText, "expand", TRUE, NULL);
    g_object_set(colImage, "expand", FALSE, NULL);

    tree = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences-switch"));
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(list));
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), colImage);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), colText);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

    iccFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(iccFilter, _("ICC profiles (*.icc, *.icm)"));
    gtk_file_filter_add_pattern(iccFilter, "*.[Ii][Cc][Cc]");
    gtk_file_filter_add_pattern(iccFilter, "*.[Ii][Cc][Mm]");

    allFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(allFilter, _("All files (*.*)"));
    gtk_file_filter_add_pattern(allFilter, "*");

    chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(priv->builder, "cms-rgb-profile"));
    g_object_ref(allFilter);
    gtk_file_chooser_add_filter(chooser, allFilter);
    g_object_ref(iccFilter);
    gtk_file_chooser_add_filter(chooser, iccFilter);
    gtk_file_chooser_set_filter(chooser, iccFilter);

    chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(priv->builder, "cms-monitor-profile"));
    g_object_ref(allFilter);
    gtk_file_chooser_add_filter(chooser, allFilter);
    g_object_ref(iccFilter);
    gtk_file_chooser_add_filter(chooser, iccFilter);
    gtk_file_chooser_set_filter(chooser, iccFilter);

    g_object_unref(iccFilter);
    g_object_unref(allFilter);

    aspectRatio = GTK_COMBO_BOX(gtk_builder_get_object(priv->builder, "img-aspect-ratio"));
    list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING, -1);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1",
                       1, _("1:1 - Square / MF 6x6"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.15",
                       1, _("1.15:1 - Movietone"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.33",
                       1, _("1.33:1 (4:3, 12:9) - Super 35mm / DSLR / MF 645"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.37",
                       1, _("1.37:1 - 35mm movie"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.44",
                       1, _("1.44:1 - IMAX"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.5",
                       1, _("1.50:1 (3:2, 15:10)- 35mm SLR"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.6",
                       1, _("1.6:1 (8:5, 16:10) - Widescreen"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.66",
                       1, _("1.66:1 (5:3, 15:9) - Super 16mm"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.75",
                       1, _("1.75:1 (7:4) - Widescreen"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.77",
                       1, _("1.77:1 (16:9) - APS-H / HDTV / Widescreen"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "1.85",
                       1, _("1.85:1 - 35mm Widescreen"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.0",
                       1, _("2.00:1 - SuperScope"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.10",
                       1, _("2.10:1 (21:10) - Planned HDTV"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.20",
                       1, _("2.20:1 (11:5, 22:10) - 70mm movie"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.35",
                       1, _("2.35:1 - CinemaScope"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.37",
                       1, _("2.37:1 (64:27)- HDTV cinema"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.39",
                       1, _("2.39:1 (12:5)- Panavision"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.55",
                       1, _("2.55:1 (23:9)- CinemaScope 55"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.59",
                       1, _("2.59:1 (13:5)- Cinerama"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.66",
                       1, _("2.66:1 (8:3, 24:9)- Super 16mm"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.76",
                       1, _("2.76:1 (11:4) - Ultra Panavision"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "2.93",
                       1, _("2.93:1 - MGM Camera 65"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "3.0",
                       1, _("3:1 APS Panorama"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "4.0",
                       1, _("4.00:1 - Polyvision"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "12.0",
                       1, _("12.00:1 - Circle-Vision 360"),
                       -1);

    gtk_combo_box_set_model(GTK_COMBO_BOX(aspectRatio), GTK_TREE_MODEL(list));
    gtk_combo_box_set_id_column(GTK_COMBO_BOX(aspectRatio), 0);

    cellText = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(aspectRatio), cellText, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(aspectRatio),
                                   cellText, "text", 1, NULL);


    gridLines = GTK_COMBO_BOX(gtk_builder_get_object(priv->builder, "img-grid-lines"));
    list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING, -1);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "none",
                       1, _("None"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "center-lines",
                       1, _("Center lines"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "rule-of-3rds",
                       1, _("Rule of 3rds"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "quarters",
                       1, _("Quarters"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "rule-of-5ths",
                       1, _("Rule of 5ths"),
                       -1);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
                       0, "golden-sections",
                       1, _("Golden sections"),
                       -1);
    gtk_combo_box_set_model(GTK_COMBO_BOX(gridLines), GTK_TREE_MODEL(list));
    gtk_combo_box_set_id_column(GTK_COMBO_BOX(gridLines), 0);

    cellText = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(gridLines), cellText, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(gridLines),
                                   cellText, "text", 1, NULL);

    g_signal_connect(selection, "changed", G_CALLBACK(do_page_changed), preferences);
}


static GtkBuilder *do_entangle_preferences_display_get_builder(EntangleWindow *window)
{
    EntanglePreferencesDisplay *display = ENTANGLE_PREFERENCES_DISPLAY(window);
    EntanglePreferencesDisplayPrivate *priv = display->priv;

    return priv->builder;
}


static void entangle_preferences_display_init(EntanglePreferencesDisplay *preferences)
{
    preferences->priv = ENTANGLE_PREFERENCES_DISPLAY_GET_PRIVATE(preferences);

    g_signal_connect(preferences,
                     "notify::builder",
                     G_CALLBACK(do_entangle_preferences_display_set_builder),
                     NULL);
    g_signal_connect(preferences,
                     "notify::application",
                     G_CALLBACK(do_entangle_preferences_display_set_app),
                     NULL);

}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
