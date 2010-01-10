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
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <unistd.h>

#include "capa-debug.h"
#include "capa-preferences-display.h"
#include "capa-camera-picker.h"
#include "capa-camera-manager.h"
#include "capa-plugin.h"

#define CAPA_PREFERENCES_DISPLAY_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_PREFERENCES_DISPLAY, CapaPreferencesDisplayPrivate))

static void capa_preferences_display_refresh(CapaPreferencesDisplay *preferences);

struct _CapaPreferencesDisplayPrivate {
    GladeXML *glade;

    GtkListStore *plugins;
    CapaPluginManager *pluginManager;
    CapaPreferences *prefs;
    gulong prefsID;
};

G_DEFINE_TYPE(CapaPreferencesDisplay, capa_preferences_display, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_PREFERENCES,
    PROP_PLUGIN_MANAGER,
};

static void capa_preferences_display_get_property(GObject *object,
                                                  guint prop_id,
                                                  GValue *value,
                                                  GParamSpec *pspec)
{
    CapaPreferencesDisplay *display = CAPA_PREFERENCES_DISPLAY(object);
    CapaPreferencesDisplayPrivate *priv = display->priv;

    switch (prop_id)
        {
        case PROP_PREFERENCES:
            g_value_set_object(value, priv->prefs);
            break;

        case PROP_PLUGIN_MANAGER:
            g_value_set_object(value, priv->pluginManager);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_preferences_display_setup_plugins(CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    GtkWidget *tree;
    GtkCellRenderer *cellText;
    GtkCellRenderer *cellImage;
    GtkCellRenderer *cellEnabled;
    GtkTreeViewColumn *colText;
    GtkTreeViewColumn *colImage;
    GtkTreeViewColumn *colEnabled;

    priv->plugins = gtk_list_store_new(5, CAPA_TYPE_PLUGIN, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN, G_TYPE_STRING, -1);

    cellText = gtk_cell_renderer_text_new();
    cellImage = gtk_cell_renderer_pixbuf_new();
    cellEnabled = gtk_cell_renderer_toggle_new();

    colText = gtk_tree_view_column_new_with_attributes("Description", cellText, "text", 1, NULL);
    colImage = gtk_tree_view_column_new_with_attributes("", cellImage, "pixbuf", 2, NULL);
    colEnabled = gtk_tree_view_column_new_with_attributes("Enabled", cellEnabled, "active", 3, NULL);

    g_object_set(G_OBJECT(colText), "expand", TRUE, NULL);
    g_object_set(G_OBJECT(colImage), "expand", FALSE, NULL);
    g_object_set(G_OBJECT(colEnabled), "expand", FALSE, NULL);

    tree = glade_xml_get_widget(priv->glade, "plugins-list");
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(priv->plugins));
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), colEnabled);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), colImage);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), colText);
    gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tree), 4);
}


static gchar *capa_preferences_display_plugin_tooltip(CapaPlugin *plugin)
{
    gchar *tip = g_strdup_printf("Name: %s\n"
                                 "Version: %s\n"
                                 "URI: %s\n"
                                 "Email: %s",
                                 capa_plugin_get_name(plugin),
                                 capa_plugin_get_version(plugin),
                                 capa_plugin_get_uri(plugin),
                                 capa_plugin_get_email(plugin));

    return tip;
}

static void capa_preferences_display_populate_plugins(CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    GList *plugins;
    GList *tmp;

    gtk_list_store_clear(priv->plugins);

    if (!priv->pluginManager)
        return;

    plugins = capa_plugin_manager_get_all(priv->pluginManager);

    tmp = plugins;
    while (tmp) {
        CapaPlugin *plugin = CAPA_PLUGIN(tmp->data);
        GtkTreeIter iter;
        gchar *tip = capa_preferences_display_plugin_tooltip(plugin);

        gtk_list_store_append(priv->plugins, &iter);
        gtk_list_store_set(priv->plugins, &iter,
                           0, plugin,
                           1, capa_plugin_get_description(plugin),
                           2, NULL,
                           3, TRUE,
                           4, tip,
                           -1);

        tmp = tmp->next;
    }

    g_list_free(plugins);
}

static void capa_preferences_display_notify(GObject *object, GParamSpec *spec, gpointer opaque)
{
    CapaPreferencesDisplay *preferences = CAPA_PREFERENCES_DISPLAY(opaque);
    CapaPreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *tmp;

    CAPA_DEBUG("Internal display Set %p %s", object, spec->name);
    if (strcmp(spec->name, "colour-managed-display") == 0) {
        gboolean newvalue;
        gboolean oldvalue;
        tmp = glade_xml_get_widget(priv->glade, "cms-enabled");

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "detect-system-profile") == 0) {
        gboolean newvalue;
        gboolean oldvalue;
        tmp = glade_xml_get_widget(priv->glade, "cms-system-profile");

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tmp));

        if (newvalue != oldvalue)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), newvalue);
    } else if (strcmp(spec->name, "rgb-profile") == 0) {
        CapaColourProfile *profile;
        const gchar *oldvalue;
        const gchar *newvalue;
        tmp = glade_xml_get_widget(priv->glade, "cms-rgb-profile");

        g_object_get(object, spec->name, &profile, NULL);

        newvalue = profile ? capa_colour_profile_filename(profile) : NULL;
        oldvalue = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(tmp));

        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), newvalue);

        if (profile)
            g_object_unref(G_OBJECT(profile));
    } else if (strcmp(spec->name, "monitor-profile") == 0) {
        CapaColourProfile *profile;
        const gchar *oldvalue;
        const gchar *newvalue;
        tmp = glade_xml_get_widget(priv->glade, "cms-monitor-profile");

        g_object_get(object, spec->name, &profile, NULL);

        newvalue = profile ? capa_colour_profile_filename(profile) : NULL;
        oldvalue = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(tmp));

        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), newvalue);

        if (profile)
            g_object_unref(G_OBJECT(profile));
    } else if (strcmp(spec->name, "picture-dir") == 0) {
        gchar *newvalue;
        const gchar *oldvalue;
        tmp = glade_xml_get_widget(priv->glade, "picture-folder");

        g_object_get(object, spec->name, &newvalue, NULL);

        oldvalue = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(tmp));
        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), newvalue);

        g_free(newvalue);
    } else if (strcmp(spec->name, "filename-pattern") == 0) {
        gchar *newvalue;
        const gchar *oldvalue;
        tmp = glade_xml_get_widget(priv->glade, "filename-pattern");

        g_object_get(object, spec->name, &newvalue, NULL);

        oldvalue = gtk_entry_get_text(GTK_ENTRY(tmp));
        if ((newvalue && !oldvalue) ||
            (!newvalue && oldvalue) ||
            strcmp(newvalue, oldvalue) != 0)
            gtk_entry_set_text(GTK_ENTRY(tmp), newvalue);

        g_free(newvalue);
    } else if (strcmp(spec->name, "profile-rendering-intent") == 0) {
        int newvalue;
        int oldvalue;
        tmp = glade_xml_get_widget(priv->glade, "cms-render-intent");

        g_object_get(object, spec->name, &newvalue, NULL);
        oldvalue = gtk_combo_box_get_active(GTK_COMBO_BOX(tmp));

        if (oldvalue != newvalue)
            gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), newvalue);
    }
}


static void capa_preferences_display_set_property(GObject *object,
                                                  guint prop_id,
                                                  const GValue *value,
                                                  GParamSpec *pspec)
{
    CapaPreferencesDisplay *display = CAPA_PREFERENCES_DISPLAY(object);
    CapaPreferencesDisplayPrivate *priv = display->priv;

    CAPA_DEBUG("Set prop on preferences display %d", prop_id);

    switch (prop_id)
        {
        case PROP_PREFERENCES: {
            if (priv->prefs) {
                g_signal_handler_disconnect(G_OBJECT(priv->prefs), priv->prefsID);
                g_object_unref(G_OBJECT(priv->prefs));
            }
            priv->prefs = g_value_get_object(value);
            g_object_ref(G_OBJECT(priv->prefs));
            capa_preferences_display_refresh(display);
            priv->prefsID = g_signal_connect(G_OBJECT(priv->prefs),
                                             "notify",
                                             G_CALLBACK(capa_preferences_display_notify),
                                             object);
        } break;

        case PROP_PLUGIN_MANAGER: {
            if (priv->pluginManager) {
                g_object_unref(G_OBJECT(priv->pluginManager));
            }
            priv->pluginManager = g_value_get_object(value);
            if (priv->pluginManager)
                g_object_ref(G_OBJECT(priv->pluginManager));

            capa_preferences_display_populate_plugins(display);
        } break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_preferences_display_refresh(CapaPreferencesDisplay *preferences)
{
    CapaPreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *tmp;
    CapaColourProfile *profile;
    const char *dir;

    tmp = glade_xml_get_widget(priv->glade, "cms-enabled");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), capa_preferences_enable_color_management(priv->prefs));

    tmp = glade_xml_get_widget(priv->glade, "cms-system-profile");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), capa_preferences_detect_monitor_profile(priv->prefs));

    tmp = glade_xml_get_widget(priv->glade, "cms-rgb-profile");
    profile = capa_preferences_rgb_profile(priv->prefs);
    if (profile)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), capa_colour_profile_filename(profile));

    tmp = glade_xml_get_widget(priv->glade, "cms-monitor-profile");
    profile = capa_preferences_monitor_profile(priv->prefs);
    if (profile)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), capa_colour_profile_filename(profile));

    tmp = glade_xml_get_widget(priv->glade, "cms-render-intent");
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), capa_preferences_profile_rendering_intent(priv->prefs));


    tmp = glade_xml_get_widget(priv->glade, "filename-pattern");
    gtk_entry_set_text(GTK_ENTRY(tmp), capa_preferences_filename_pattern(priv->prefs));

    tmp = glade_xml_get_widget(priv->glade, "picture-folder");
    dir = capa_preferences_picture_dir(priv->prefs);
    if (dir)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(tmp), dir);
}

static void capa_preferences_display_finalize (GObject *object)
{
    CapaPreferencesDisplay *preferences = CAPA_PREFERENCES_DISPLAY(object);
    CapaPreferencesDisplayPrivate *priv = preferences->priv;

    CAPA_DEBUG("Finalize preferences");

    g_signal_handler_disconnect(G_OBJECT(priv->prefs), priv->prefsID);
    g_object_unref(G_OBJECT(priv->prefs));
    g_object_unref(G_OBJECT(priv->pluginManager));

    g_object_unref(G_OBJECT(priv->glade));
}

static void capa_preferences_display_class_init(CapaPreferencesDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_preferences_display_finalize;
    object_class->get_property = capa_preferences_display_get_property;
    object_class->set_property = capa_preferences_display_set_property;

    g_object_class_install_property(object_class,
                                    PROP_PREFERENCES,
                                    g_param_spec_object("preferences",
                                                        "Preferences",
                                                        "Preferences to be displayed",
                                                        CAPA_TYPE_PREFERENCES,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_object_class_install_property(object_class,
                                    PROP_PLUGIN_MANAGER,
                                    g_param_spec_object("plugin-manager",
                                                        "Plugin manager",
                                                        "Plugin manager",
                                                        CAPA_TYPE_PLUGIN_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(CapaPreferencesDisplayPrivate));
}


CapaPreferencesDisplay *capa_preferences_display_new(CapaPreferences *preferences,
                                                     CapaPluginManager *pluginManager)
{
    return CAPA_PREFERENCES_DISPLAY(g_object_new(CAPA_TYPE_PREFERENCES_DISPLAY,
                                                 "preferences", preferences,
                                                 "plugin-manager", pluginManager,
                                                 NULL));
}


static void do_preferences_close(GtkButton *src G_GNUC_UNUSED, CapaPreferencesDisplay *preferences)
{
    CapaPreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "preferences");
    gtk_widget_hide(win);
}


static gboolean do_preferences_delete(GtkWidget *src,
                                      GdkEvent *ev G_GNUC_UNUSED,
                                      CapaPreferencesDisplay *preferences G_GNUC_UNUSED)
{
    CAPA_DEBUG("preferences delete");
    gtk_widget_hide(src);
    return TRUE;
}

static void do_page_changed(GtkTreeSelection *selection,
                            CapaPreferencesDisplay *preferences)
{
    CapaPreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *list;
    GtkTreeIter iter;
    gboolean selected;
    GValue val;
    int page;
    GtkWidget *notebook;

    CAPA_DEBUG("select page");

    list = glade_xml_get_widget(priv->glade, "preferences-switch");

    selected = gtk_tree_selection_get_selected(selection, NULL, &iter);
    if (!selected)
        return;

    memset(&val, 0, sizeof val);
    gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(list)),
                             &iter, 0, &val);

    notebook = glade_xml_get_widget(priv->glade, "preferences-notebook");
    page = g_value_get_int(&val);
    if (page >= 0)
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
}


static void do_cms_enabled_toggled(GtkToggleButton *src, CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *rgbProfile = glade_xml_get_widget(priv->glade, "cms-rgb-profile");
    GtkWidget *monitorProfile = glade_xml_get_widget(priv->glade, "cms-monitor-profile");
    GtkWidget *systemProfile = glade_xml_get_widget(priv->glade, "cms-system-profile");
    GtkWidget *renderIntent = glade_xml_get_widget(priv->glade, "cms-render-intent");

    g_object_set(G_OBJECT(priv->prefs), "colour-managed-display", enabled, NULL);
    gtk_widget_set_sensitive(rgbProfile, enabled);
    gtk_widget_set_sensitive(systemProfile, enabled);
    gtk_widget_set_sensitive(renderIntent, enabled);
    gtk_widget_set_sensitive(monitorProfile, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(systemProfile)));
}

static void do_cms_rgb_profile_file_set(GtkFileChooserButton *src, CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    CapaColourProfile *profile = capa_colour_profile_new_file(filename);
    g_object_set(G_OBJECT(priv->prefs), "rgb-profile", profile, NULL);
    g_free(filename);
    g_object_unref(profile);
}

static void do_cms_monitor_profile_file_set(GtkFileChooserButton *src, CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    CapaColourProfile *profile = capa_colour_profile_new_file(filename);
    g_object_set(G_OBJECT(priv->prefs), "monitor-profile", profile, NULL);
    g_free(filename);
    g_object_unref(profile);
}

static void do_cms_system_profile_toggled(GtkToggleButton *src, CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    gboolean enabled = gtk_toggle_button_get_active(src);
    GtkWidget *monitorProfile = glade_xml_get_widget(priv->glade, "cms-monitor-profile");
    g_object_set(G_OBJECT(priv->prefs), "detect-system-profile", enabled, NULL);
    gtk_widget_set_sensitive(monitorProfile, !enabled);
}

static void do_cms_render_intent_changed(GtkComboBox *src, CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    int option = gtk_combo_box_get_active(src);
    if (option < 0)
        option = CAPA_COLOUR_PROFILE_INTENT_PERCEPTUAL;
    g_object_set(G_OBJECT(priv->prefs), "profile-rendering-intent", option, NULL);
}

static void do_picture_folder_file_set(GtkFileChooserButton *src, CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    g_object_set(G_OBJECT(priv->prefs), "picture-dir", filename, NULL);
    g_free(filename);
}

static void do_filename_pattern_changed(GtkEntry *src, CapaPreferencesDisplay *display)
{
    CapaPreferencesDisplayPrivate *priv = display->priv;
    const char *text = gtk_entry_get_text(src);
    g_object_set(G_OBJECT(priv->prefs), "filename-pattern", text, NULL);
}

static void capa_preferences_display_init(CapaPreferencesDisplay *preferences)
{
    CapaPreferencesDisplayPrivate *priv;
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

    priv = preferences->priv = CAPA_PREFERENCES_DISPLAY_GET_PRIVATE(preferences);

    if (access("./capa.glade", R_OK) == 0) {
        priv->glade = glade_xml_new("capa.glade", "preferences", "capa");
        local = TRUE;
    } else {
        priv->glade = glade_xml_new(PKGDATADIR "/capa.glade", "preferences", "capa");
    }

    glade_xml_signal_connect_data(priv->glade, "preferences_close", G_CALLBACK(do_preferences_close), preferences);

    glade_xml_signal_connect_data(priv->glade, "cms_enabled_toggled", G_CALLBACK(do_cms_enabled_toggled), preferences);
    glade_xml_signal_connect_data(priv->glade, "cms_monitor_profile_file_set", G_CALLBACK(do_cms_monitor_profile_file_set), preferences);
    glade_xml_signal_connect_data(priv->glade, "cms_rgb_profile_file_set", G_CALLBACK(do_cms_rgb_profile_file_set), preferences);
    glade_xml_signal_connect_data(priv->glade, "cms_render_intent_changed", G_CALLBACK(do_cms_render_intent_changed), preferences);
    glade_xml_signal_connect_data(priv->glade, "cms_system_profile_toggled", G_CALLBACK(do_cms_system_profile_toggled), preferences);

    glade_xml_signal_connect_data(priv->glade, "filename_pattern_changed", G_CALLBACK(do_filename_pattern_changed), preferences);
    glade_xml_signal_connect_data(priv->glade, "picture_folder_file_set", G_CALLBACK(do_picture_folder_file_set), preferences);

    win = glade_xml_get_widget(priv->glade, "preferences");
    g_signal_connect(win, "delete-event", G_CALLBACK(do_preferences_delete), preferences);

    notebook = glade_xml_get_widget(priv->glade, "preferences-notebook");
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

    box = glade_xml_get_widget(priv->glade, "cms-box");
    gtk_widget_set_state(box, GTK_STATE_SELECTED);
    image = glade_xml_get_widget(priv->glade, "cms-image");
    if (local)
        gtk_image_set_from_file(GTK_IMAGE(image), "./color-management.png");
    else
        gtk_image_set_from_file(GTK_IMAGE(image), PKGDATADIR "/color-management.png");

    box = glade_xml_get_widget(priv->glade, "folders-box");
    gtk_widget_set_state(box, GTK_STATE_SELECTED);
    image = glade_xml_get_widget(priv->glade, "folders-image");
    if (local)
        gtk_image_set_from_file(GTK_IMAGE(image), "./folders.png");
    else
        gtk_image_set_from_file(GTK_IMAGE(image), PKGDATADIR "/folders.png");

    box = glade_xml_get_widget(priv->glade, "plugins-box");
    gtk_widget_set_state(box, GTK_STATE_SELECTED);
    image = glade_xml_get_widget(priv->glade, "plugins-image");
    if (local)
        gtk_image_set_from_file(GTK_IMAGE(image), "./plugins.png");
    else
        gtk_image_set_from_file(GTK_IMAGE(image), PKGDATADIR "/plugins.png");

    list = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF, -1);

    gtk_list_store_append(list, &iter);
    if (local)
        gtk_list_store_set(list, &iter,
                           0, 0,
                           1, "Folders",
                           2, gdk_pixbuf_new_from_file("./folders-22.png", NULL),
                           -1);
    else
        gtk_list_store_set(list, &iter,
                           0, 0,
                           1, "Folders",
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

    g_object_set(G_OBJECT(colText), "expand", TRUE, NULL);
    g_object_set(G_OBJECT(colImage), "expand", FALSE, NULL);

    tree = glade_xml_get_widget(priv->glade, "preferences-switch");
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

    chooser = GTK_FILE_CHOOSER(glade_xml_get_widget(priv->glade, "cms-rgb-profile"));
    g_object_ref(allFilter);
    gtk_file_chooser_add_filter(chooser, allFilter);
    g_object_ref(iccFilter);
    gtk_file_chooser_add_filter(chooser, iccFilter);
    gtk_file_chooser_set_filter(chooser, iccFilter);

    chooser = GTK_FILE_CHOOSER(glade_xml_get_widget(priv->glade, "cms-monitor-profile"));
    g_object_ref(allFilter);
    gtk_file_chooser_add_filter(chooser, allFilter);
    g_object_ref(iccFilter);
    gtk_file_chooser_add_filter(chooser, iccFilter);
    gtk_file_chooser_set_filter(chooser, iccFilter);

    g_object_unref(iccFilter);
    g_object_unref(allFilter);

    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(do_page_changed), preferences);

    capa_preferences_display_setup_plugins(preferences);
}


void capa_preferences_display_show(CapaPreferencesDisplay *preferences)
{
    CapaPreferencesDisplayPrivate *priv = preferences->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "preferences");

    gtk_widget_show(win);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
