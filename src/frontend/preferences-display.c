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

#include "internal.h"
#include "preferences-display.h"
#include "camera-picker.h"
#include "camera-manager.h"

#define CAPA_PREFERENCES_DISPLAY_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_PREFERENCES_DISPLAY, CapaPreferencesDisplayPrivate))

static void capa_preferences_display_refresh(CapaPreferencesDisplay *preferences);

struct _CapaPreferencesDisplayPrivate {
    GladeXML *glade;

    CapaPreferences *prefs;
};

G_DEFINE_TYPE(CapaPreferencesDisplay, capa_preferences_display, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_PREFERENCES,
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

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
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
            if (priv->prefs)
                g_object_unref(G_OBJECT(priv->prefs));
            priv->prefs = g_value_get_object(value);
            g_object_ref(G_OBJECT(priv->prefs));
            capa_preferences_display_refresh(display);
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


    g_type_class_add_private(klass, sizeof(CapaPreferencesDisplayPrivate));
}


CapaPreferencesDisplay *capa_preferences_display_new(CapaPreferences *preferences)
{
    return CAPA_PREFERENCES_DISPLAY(g_object_new(CAPA_TYPE_PREFERENCES_DISPLAY,
                                                 "preferences", preferences,
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

    priv = preferences->priv = CAPA_PREFERENCES_DISPLAY_GET_PRIVATE(preferences);

    if (access("./capa.glade", R_OK) == 0) {
        priv->glade = glade_xml_new("capa.glade", "preferences", "capa");
        local = TRUE;
    } else {
        priv->glade = glade_xml_new(PKGDATADIR "/capa.glade", "preferences", "capa");
    }

    glade_xml_signal_connect_data(priv->glade, "preferences_close", G_CALLBACK(do_preferences_close), preferences);

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

    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(do_page_changed), preferences);
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
