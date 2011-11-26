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
#include <gtk/gtk.h>
#include <unistd.h>

#include <libpeas-gtk/peas-gtk.h>

#include "entangle-debug.h"
#include "entangle-preferences-display.h"
#include "entangle-camera-picker.h"
#include "entangle-camera-manager.h"

#define ENTANGLE_PREFERENCES_DISPLAY_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_PREFERENCES_DISPLAY, EntanglePreferencesDisplayPrivate))

static void entangle_preferences_display_refresh(EntanglePreferencesDisplay *preferences);

struct _EntanglePreferencesDisplayPrivate {
    GtkBuilder *builder;

    EntangleContext *context;

    PeasGtkPluginManager *pluginManager;
    gulong prefsID;
};

G_DEFINE_TYPE(EntanglePreferencesDisplay, entangle_preferences_display, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_CONTEXT,
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
void do_cms_system_profile_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display);
void do_cms_render_intent_changed(GtkComboBox *src, EntanglePreferencesDisplay *display);
void do_filename_pattern_changed(GtkEntry *src, EntanglePreferencesDisplay *display);


static void entangle_preferences_display_get_property(GObject *object,
                                                      guint prop_id,
                                                      GValue *value,
                                                      GParamSpec *pspec)
{
    EntanglePreferencesDisplay *display = ENTANGLE_PREFERENCES_DISPLAY(object);
    EntanglePreferencesDisplayPrivate *priv = display->priv;

    switch (prop_id)
        {
        case PROP_CONTEXT:
            g_value_set_object(value, priv->context);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_preferences_display_notify(GObject *object,
                                                GParamSpec *spec,
                                                gpointer opaque)
{
    EntanglePreferencesDisplay *preferences = ENTANGLE_PREFERENCES_DISPLAY(opaque);
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *tmp;

    ENTANGLE_DEBUG("Internal display Set %p %s", object, spec->name);
    if (strcmp(spec->name, "cms-enabled") == 0) {
        gboolean newvalue;
        gboolean oldvalue;
        tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-enabled"));

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "cms-detect-system-profile") == 0) {
        gboolean newvalue;
        gboolean oldvalue;
        tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-system-profile"));

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "cms-rgb-profile") == 0) {
        EntangleColourProfile *profile;
        const gchar *oldvalue;
        const gchar *newvalue;
        tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-rgb-profile"));

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
        tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-monitor-profile"));

        g_object_get(object, spec->name, &profile, NULL);

        newvalue = profile ? entangle_colour_profile_filename(profile) : NULL;
        oldvalue = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(tmp));

        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), newvalue);

        if (profile)
            g_object_unref(profile);
    } else if (strcmp(spec->name, "capture-filename-pattern") == 0) {
        gchar *newvalue;
        const gchar *oldvalue;
        tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "filename-pattern"));

        g_object_get(object, spec->name, &newvalue, NULL);

        oldvalue = gtk_entry_get_text(GTK_ENTRY(tmp));
        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_entry_set_text(GTK_ENTRY(tmp), newvalue);

        g_free(newvalue);
    } else if (strcmp(spec->name, "cms-rendering-intent") == 0) {
        int newvalue;
        int oldvalue;
        tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-render-intent"));

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_combo_box_get_active(GTK_COMBO_BOX(tmp));

        if (oldvalue != newvalue)
            gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), newvalue);
    }
}


static void entangle_preferences_display_set_property(GObject *object,
                                                      guint prop_id,
                                                      const GValue *value,
                                                      GParamSpec *pspec)
{
    EntanglePreferencesDisplay *display = ENTANGLE_PREFERENCES_DISPLAY(object);
    EntanglePreferencesDisplayPrivate *priv = display->priv;
    EntanglePreferences *prefs;

    ENTANGLE_DEBUG("Set prop on preferences display %d", prop_id);

    switch (prop_id)
        {
        case PROP_CONTEXT:
            priv->context = g_value_get_object(value);
            g_object_ref(priv->context);
            entangle_preferences_display_refresh(display);
            prefs = entangle_context_get_preferences(priv->context);
            priv->prefsID = g_signal_connect(prefs,
                                             "notify",
                                             G_CALLBACK(entangle_preferences_display_notify),
                                             object);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_preferences_display_refresh(EntanglePreferencesDisplay *preferences)
{
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *tmp;
    EntangleColourProfile *profile;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-enabled"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), entangle_preferences_cms_get_enabled(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-system-profile"));
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

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-render-intent"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), entangle_preferences_cms_get_rendering_intent(prefs));

    tmp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "filename-pattern"));
    gtk_entry_set_text(GTK_ENTRY(tmp), entangle_preferences_capture_get_filename_pattern(prefs));
}

static void entangle_preferences_display_finalize (GObject *object)
{
    EntanglePreferencesDisplay *preferences = ENTANGLE_PREFERENCES_DISPLAY(object);
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);

    ENTANGLE_DEBUG("Finalize preferences");

    g_signal_handler_disconnect(prefs, priv->prefsID);
    g_object_unref(priv->context);
    g_object_unref(priv->builder);
}

static void entangle_preferences_display_class_init(EntanglePreferencesDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_preferences_display_finalize;
    object_class->get_property = entangle_preferences_display_get_property;
    object_class->set_property = entangle_preferences_display_set_property;

    g_object_class_install_property(object_class,
                                    PROP_CONTEXT,
                                    g_param_spec_object("context",
                                                        "Context",
                                                        "Application context",
                                                        ENTANGLE_TYPE_CONTEXT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(EntanglePreferencesDisplayPrivate));
}


EntanglePreferencesDisplay *entangle_preferences_display_new(EntangleContext *context)
{
    return ENTANGLE_PREFERENCES_DISPLAY(g_object_new(ENTANGLE_TYPE_PREFERENCES_DISPLAY,
                                                     "context", context,
                                                     NULL));
}

void do_preferences_close(GtkButton *src G_GNUC_UNUSED, EntanglePreferencesDisplay *preferences)
{
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences"));
    gtk_widget_hide(win);
}


gboolean do_preferences_delete(GtkWidget *src,
                               GdkEvent *ev G_GNUC_UNUSED,
                               EntanglePreferencesDisplay *preferences G_GNUC_UNUSED)
{
    ENTANGLE_DEBUG("preferences delete");
    gtk_widget_hide(src);
    return TRUE;
}

void do_page_changed(GtkTreeSelection *selection,
                     EntanglePreferencesDisplay *preferences)
{
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


void do_cms_enabled_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display)
{
    EntanglePreferencesDisplayPrivate *priv = display->priv;
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *rgbProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-rgb-profile"));
    GtkWidget *monitorProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-monitor-profile"));
    GtkWidget *systemProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-system-profile"));
    GtkWidget *renderIntent = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-render-intent"));
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);

    entangle_preferences_cms_set_enabled(prefs, enabled);
    gtk_widget_set_sensitive(rgbProfile, enabled);
    gtk_widget_set_sensitive(systemProfile, enabled);
    gtk_widget_set_sensitive(renderIntent, enabled);
    gtk_widget_set_sensitive(monitorProfile, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(systemProfile)));
}

void do_cms_rgb_profile_file_set(GtkFileChooserButton *src, EntanglePreferencesDisplay *display)
{
    EntanglePreferencesDisplayPrivate *priv = display->priv;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    EntangleColourProfile *profile = entangle_colour_profile_new_file(filename);

    entangle_preferences_cms_set_rgb_profile(prefs, profile);
    g_free(filename);
    g_object_unref(profile);
}

void do_cms_monitor_profile_file_set(GtkFileChooserButton *src, EntanglePreferencesDisplay *display)
{
    EntanglePreferencesDisplayPrivate *priv = display->priv;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    EntangleColourProfile *profile = entangle_colour_profile_new_file(filename);

    entangle_preferences_cms_set_monitor_profile(prefs, profile);
    g_free(filename);
    g_object_unref(profile);
}

void do_cms_system_profile_toggled(GtkToggleButton *src, EntanglePreferencesDisplay *display)
{
    EntanglePreferencesDisplayPrivate *priv = display->priv;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *monitorProfile = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-monitor-profile"));

    entangle_preferences_cms_set_detect_system_profile(prefs, enabled);
    gtk_widget_set_sensitive(monitorProfile, !enabled);
}

void do_cms_render_intent_changed(GtkComboBox *src, EntanglePreferencesDisplay *display)
{
    EntanglePreferencesDisplayPrivate *priv = display->priv;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    int option = gtk_combo_box_get_active(src);

    if (option < 0)
        option = ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL;
    entangle_preferences_cms_set_rendering_intent(prefs, option);
}


void do_filename_pattern_changed(GtkEntry *src, EntanglePreferencesDisplay *display)
{
    EntanglePreferencesDisplayPrivate *priv = display->priv;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    const char *text = gtk_entry_get_text(src);

    entangle_preferences_capture_set_filename_pattern(prefs, text);
}

static void entangle_preferences_display_init(EntanglePreferencesDisplay *preferences)
{
    EntanglePreferencesDisplayPrivate *priv;
    GtkWidget *win;
    GtkWidget *notebook;
    GtkWidget *box;
    GtkWidget *image;
    gboolean local = FALSE;
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
    GError *error = NULL;

    priv = preferences->priv = ENTANGLE_PREFERENCES_DISPLAY_GET_PRIVATE(preferences);

    priv->builder = gtk_builder_new();

    if (access("./entangle", R_OK) == 0) {
        gtk_builder_add_from_file(priv->builder, "frontend/entangle-preferences.xml", &error);
        local = TRUE;
    } else {
        gtk_builder_add_from_file(priv->builder, PKGDATADIR "/frontend/entangle-preferences.xml", &error);
    }

    if (error)
        g_error("Couldn't load builder file: %s", error->message);

    gtk_builder_connect_signals(priv->builder, preferences);

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences"));
    g_signal_connect(win, "delete-event", G_CALLBACK(do_preferences_delete), preferences);

    notebook = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences-notebook"));
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

    //gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), 2);

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-box"));
    gtk_widget_set_state(box, GTK_STATE_SELECTED);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-image"));
    if (local)
        gtk_image_set_from_file(GTK_IMAGE(image), "./color-management.png");
    else
        gtk_image_set_from_file(GTK_IMAGE(image), PKGDATADIR "/color-management.png");

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "folders-box"));
    gtk_widget_set_state(box, GTK_STATE_SELECTED);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "folders-image"));
    if (local)
        gtk_image_set_from_file(GTK_IMAGE(image), "./folders.png");
    else
        gtk_image_set_from_file(GTK_IMAGE(image), PKGDATADIR "/folders.png");

    box = GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugins-box"));
    gtk_widget_set_state(box, GTK_STATE_SELECTED);
    image = GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugins-image"));
    if (local)
        gtk_image_set_from_file(GTK_IMAGE(image), "./plugins.png");
    else
        gtk_image_set_from_file(GTK_IMAGE(image), PKGDATADIR "/plugins.png");

    list = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF, -1);

    gtk_list_store_append(list, &iter);
    if (local)
        gtk_list_store_set(list, &iter,
                           0, 0,
                           1, "Capture",
                           2, gdk_pixbuf_new_from_file("./folders-22.png", NULL),
                           -1);
    else
        gtk_list_store_set(list, &iter,
                           0, 0,
                           1, "Capture",
                           2, gdk_pixbuf_new_from_file(PKGDATADIR "/folders-22.png", NULL),
                           -1);

    gtk_list_store_append(list, &iter);
    if (local)
        gtk_list_store_set(list, &iter,
                           0, 1,
                           1, "Color Management",
                           2, gdk_pixbuf_new_from_file("./color-management-22.png", NULL),
                           -1);
    else
        gtk_list_store_set(list, &iter,
                           0, 1,
                           1, "Color Management",
                           2, gdk_pixbuf_new_from_file(PKGDATADIR "/color-management-22.png", NULL),
                           -1);

    gtk_list_store_append(list, &iter);
    if (local)
        gtk_list_store_set(list, &iter,
                           0, 2,
                           1, "Plugins",
                           2, gdk_pixbuf_new_from_file("./plugins-22.png", NULL),
                           -1);
    else
        gtk_list_store_set(list, &iter,
                           0, 2,
                           1, "Plugins",
                           2, gdk_pixbuf_new_from_file(PKGDATADIR "/plugins-22.png", NULL),
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
    gtk_file_filter_set_name(iccFilter, "ICC profiles (*.icc, *.icm)");
    gtk_file_filter_add_pattern(iccFilter, "*.[Ii][Cc][Cc]");
    gtk_file_filter_add_pattern(iccFilter, "*.[Ii][Cc][Mm]");

    allFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(allFilter, "All files (*.*)");
    gtk_file_filter_add_pattern(allFilter, "*");

    chooser = GTK_FILE_CHOOSER(GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-rgb-profile")));
    g_object_ref(allFilter);
    gtk_file_chooser_add_filter(chooser, allFilter);
    g_object_ref(iccFilter);
    gtk_file_chooser_add_filter(chooser, iccFilter);
    gtk_file_chooser_set_filter(chooser, iccFilter);

    chooser = GTK_FILE_CHOOSER(GTK_WIDGET(gtk_builder_get_object(priv->builder, "cms-monitor-profile")));
    g_object_ref(allFilter);
    gtk_file_chooser_add_filter(chooser, allFilter);
    g_object_ref(iccFilter);
    gtk_file_chooser_add_filter(chooser, iccFilter);
    gtk_file_chooser_set_filter(chooser, iccFilter);

    g_object_unref(iccFilter);
    g_object_unref(allFilter);

    g_signal_connect(selection, "changed", G_CALLBACK(do_page_changed), preferences);
}


GtkWindow *entangle_preferences_display_get_window(EntanglePreferencesDisplay *preferences)
{
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;

    return GTK_WINDOW(gtk_builder_get_object(priv->builder, "preferences"));
}


void entangle_preferences_display_show(EntanglePreferencesDisplay *preferences)
{
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "preferences"));

    gtk_widget_show_all(win);
}



EntangleContext *entangle_camera_preferences_get_context(EntanglePreferencesDisplay *preferences)
{
    EntanglePreferencesDisplayPrivate *priv = preferences->priv;

    return priv->context;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
