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
#include <math.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gunixoutputstream.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <errno.h>

#include "entangle-debug.h"
#include "entangle-camera-automata.h"
#include "entangle-camera-manager.h"
#include "entangle-camera-list.h"
#include "entangle-camera-support.h"
#include "entangle-camera-picker.h"
#include "entangle-session.h"
#include "entangle-image-display.h"
#include "entangle-image-statusbar.h"
#include "entangle-image-loader.h"
#include "entangle-thumbnail-loader.h"
#include "entangle-image-popup.h"
#include "entangle-image-histogram.h"
#include "entangle-help-about.h"
#include "entangle-script-config.h"
#include "entangle-session-browser.h"
#include "entangle-control-panel.h"
#include "entangle-colour-profile.h"
#include "entangle-preferences-display.h"
#include "entangle-progress.h"
#include "entangle-dpms.h"
#include "entangle-window.h"
#include "entangle-auto-drawer.h"

#define ENTANGLE_CAMERA_MANAGER_GET_PRIVATE(obj)                        \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_MANAGER, EntangleCameraManagerPrivate))

struct _EntangleCameraManagerPrivate {
    EntangleCameraAutomata *automata;
    EntangleCamera *camera;
    EntangleCameraPreferences *cameraPrefs;
    gboolean cameraReady;
    gboolean cameraChanged;
    EntangleSession *session;
    EntangleScriptConfig *scriptConfig;

    EntangleCameraPicker *picker;
    EntangleHelpAbout *about;

    EntangleCameraSupport *supported;

    EntangleImageLoader *imageLoader;
    EntangleThumbnailLoader *thumbLoader;
    EntangleColourProfileTransform *colourTransform;
    GtkScrolledWindow *imageScroll;
    EntangleImageDisplay *imageDisplay;
    EntangleImageStatusbar *imageStatusbar;
    EntangleAutoDrawer *imageDrawer;
    gulong imageDrawerTimer;
    EntangleSessionBrowser *sessionBrowser;
    GtkMenu *sessionBrowserMenu;
    EntangleImage *sessionBrowserImage;
    EntangleControlPanel *controlPanel;
    EntanglePreferencesDisplay *prefsDisplay;
    EntangleImageHistogram *imageHistogram;
    GtkWidget *scriptConfigExpander;

    EntangleImage *currentImage;

    EntangleImagePopup *imagePresentation;
    gint presentationMonitor;
    GHashTable *popups;

    gdouble imageScrollVOffset;
    gdouble imageScrollHOffset;
    gboolean imageScrollRestored;

    int zoomLevel;

    gulong sigFilePreview;
    gulong sigChanged;
    gulong sigPrefsNotify;
    gulong sigImageAdd;

    GCancellable *monitorCancel;
    GCancellable *taskCancel;
    GCancellable *taskConfirm;
    gboolean taskCapture;
    gboolean taskPreview;
    gboolean taskActive;
    gboolean taskProcessEvents;
    char *deleteImageDup;

    GtkBuilder *builder;
};


static void entangle_camera_progress_interface_init(gpointer g_iface,
                                                    gpointer iface_data);
static void entangle_camera_manager_window_interface_init(gpointer g_iface,
                                                          gpointer iface_data);
static void do_select_image(EntangleCameraManager *manager,
                            EntangleImage *image);

G_DEFINE_TYPE_EXTENDED(EntangleCameraManager, entangle_camera_manager, GTK_TYPE_WINDOW, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_PROGRESS, entangle_camera_progress_interface_init)
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_WINDOW, entangle_camera_manager_window_interface_init));

enum {
    PROP_O,
    PROP_CAMERA,
};


void do_menu_help_supported(GtkMenuItem *src,
                            EntangleCameraManager *manager);
void do_menu_new_window(GtkImageMenuItem *src,
                        EntangleCameraManager *manager);
void do_menu_select_session(GtkImageMenuItem *src,
                            EntangleCameraManager *manager);
void do_menu_settings_toggled(GtkCheckMenuItem *src,
                              EntangleCameraManager *manager);
void do_menu_zoom_in(GtkImageMenuItem *src,
                     EntangleCameraManager *manager);
void do_menu_zoom_out(GtkImageMenuItem *src,
                      EntangleCameraManager *manager);
void do_menu_zoom_normal(GtkImageMenuItem *src,
                         EntangleCameraManager *manager);
void do_menu_zoom_best(GtkImageMenuItem *src,
                       EntangleCameraManager *manager);
void do_menu_fullscreen(GtkCheckMenuItem *src,
                        EntangleCameraManager *manager);
void do_menu_presentation(GtkCheckMenuItem *src,
                          EntangleCameraManager *manager);
void do_menu_preferences(GtkCheckMenuItem *src,
                         EntangleCameraManager *manager);
void do_toolbar_select_session(GtkFileChooserButton *src,
                               EntangleCameraManager *manager);
void do_toolbar_settings(GtkToggleToolButton *src,
                         EntangleCameraManager *manager);
void do_toolbar_cancel_clicked(GtkToolButton *src,
                               EntangleCameraManager *manager);
void do_toolbar_capture(GtkToolButton *src,
                        EntangleCameraManager *manager);
void do_toolbar_preview(GtkToggleToolButton *src,
                        EntangleCameraManager *manager);
void do_toolbar_zoom_in(GtkToolButton *src,
                        EntangleCameraManager *manager);
void do_toolbar_zoom_out(GtkToolButton *src,
                         EntangleCameraManager *manager);
void do_toolbar_zoom_normal(GtkToolButton *src,
                            EntangleCameraManager *manager);
void do_toolbar_zoom_best(GtkToolButton *src,
                          EntangleCameraManager *manager);
void do_toolbar_fullscreen(GtkToggleToolButton *src,
                           EntangleCameraManager *manager);
void do_menu_close(GtkMenuItem *src,
                   EntangleCameraManager *manager);
void do_menu_quit(GtkMenuItem *src,
                  EntangleCameraManager *manager);
void do_menu_help_about(GtkMenuItem *src,
                        EntangleCameraManager *manager);
void do_menu_help_manual(GtkMenuItem *src,
                         EntangleCameraManager *manager);
void do_menu_connect(GtkMenuItem *src,
                     EntangleCameraManager *manager);
void do_menu_disconnect(GtkMenuItem *src,
                        EntangleCameraManager *manager);
void do_menu_capture(GtkMenuItem *src,
                     EntangleCameraManager *manager);
void do_menu_preview(GtkMenuItem *src,
                     EntangleCameraManager *manager);
void do_menu_cancel(GtkMenuItem *src,
                    EntangleCameraManager *manager);
void do_menu_sync_clock(GtkMenuItem *src,
                        EntangleCameraManager *manager);
gboolean do_manager_key_release(GtkWidget *widget G_GNUC_UNUSED,
                                GdkEventKey *ev,
                                gpointer data);
void do_menu_session_open_activate(GtkMenuItem *item G_GNUC_UNUSED,
                                   EntangleCameraManager *manager);
void do_menu_session_delete_activate(GtkMenuItem *item G_GNUC_UNUSED,
                                     EntangleCameraManager *manager);

static void do_camera_sync_clock_finish(GObject *src,
                                        GAsyncResult *res,
                                        gpointer opaque);

static EntanglePreferences *entangle_camera_manager_get_preferences(EntangleCameraManager *manager)
{
    GtkApplication *gapp = gtk_window_get_application(GTK_WINDOW(manager));
    EntangleApplication *app = ENTANGLE_APPLICATION(gapp);
    return entangle_application_get_preferences(app);
}

static EntangleColourProfile *entangle_camera_manager_monitor_profile(GtkWindow *window)
{
    GdkScreen *screen;
    GByteArray *profileData;
    gchar *atom;
    int monitor = 0;
    GdkAtom type = GDK_NONE;
    gint format = 0;
    gint nitems = 0;
    guint8 *data = NULL;
    EntangleColourProfile *profile = NULL;

    gtk_widget_realize(GTK_WIDGET(window));

    screen = gtk_widget_get_screen(GTK_WIDGET(window));
    monitor = gdk_screen_get_monitor_at_window(screen,
                                               gtk_widget_get_window(GTK_WIDGET(window)));

    if (monitor == 0)
        atom = g_strdup("_ICC_PROFILE");
    else
        atom = g_strdup_printf("_ICC_PROFILE_%d", monitor);

    if (!gdk_property_get(gdk_screen_get_root_window(screen),
                          gdk_atom_intern(atom, FALSE),
                          GDK_NONE,
                          0, 64 * 1024 * 1024, FALSE,
                          &type, &format, &nitems, &data) || nitems <= 0)
        goto cleanup;

    profileData = g_byte_array_new();
    g_byte_array_append(profileData, data, nitems);

    profile = entangle_colour_profile_new_data(profileData);
    g_byte_array_unref(profileData);

 cleanup:
    g_free(data);
    g_free(atom);

    return profile;
}


static EntangleColourProfileTransform *entangle_camera_manager_colour_transform(EntangleCameraManager *manager)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager), NULL);

    EntangleColourProfileTransform *transform = NULL;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);

    if (entangle_preferences_cms_get_enabled(prefs)) {
        EntangleColourProfile *rgbProfile;
        EntangleColourProfile *monitorProfile;
        EntangleColourProfileIntent intent;

        rgbProfile = entangle_preferences_cms_get_rgb_profile(prefs);
        intent = entangle_preferences_cms_get_rendering_intent(prefs);
        if (entangle_preferences_cms_get_detect_system_profile(prefs)) {
            monitorProfile = entangle_camera_manager_monitor_profile(GTK_WINDOW(manager));
        } else {
            monitorProfile = entangle_preferences_cms_get_monitor_profile(prefs);
        }

        if (monitorProfile) {
            transform = entangle_colour_profile_transform_new(rgbProfile, monitorProfile, intent);
            g_object_unref(monitorProfile);
        }
        g_object_unref(rgbProfile);
    }

    return transform;
}


static void entangle_camera_manager_update_colour_transform(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->colourTransform)
        g_object_unref(priv->colourTransform);

    priv->colourTransform = entangle_camera_manager_colour_transform(manager);
    if (priv->imageLoader)
        entangle_pixbuf_loader_set_colour_transform(ENTANGLE_PIXBUF_LOADER(priv->imageLoader),
                                                    priv->colourTransform);
    if (priv->thumbLoader)
        entangle_pixbuf_loader_set_colour_transform(ENTANGLE_PIXBUF_LOADER(priv->thumbLoader),
                                                    priv->colourTransform);
}


static void entangle_camera_manager_update_aspect_ratio(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    const gchar *aspect = entangle_preferences_img_get_aspect_ratio(prefs);

    if (!aspect) {
        entangle_image_display_set_aspect_ratio(priv->imageDisplay, 1.33);
    } else {
        gdouble d;
        gchar *end;
        d = g_ascii_strtod(aspect, &end);
        if (end == aspect || (errno == ERANGE))
            entangle_image_display_set_aspect_ratio(priv->imageDisplay, 1.33);
        else
            entangle_image_display_set_aspect_ratio(priv->imageDisplay, d);
    }
}


static void entangle_camera_manager_update_mask_opacity(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    gint opacity = entangle_preferences_img_get_mask_opacity(prefs);

    entangle_image_display_set_mask_opacity(priv->imageDisplay, opacity / 100.0);
}


static void entangle_camera_manager_update_mask_enabled(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    gboolean enabled = entangle_preferences_img_get_mask_enabled(prefs);

    entangle_image_display_set_mask_enabled(priv->imageDisplay, enabled);
}


static void entangle_camera_manager_update_histogram_linear(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    gboolean histogram_linear = entangle_preferences_interface_get_histogram_linear(prefs);

    entangle_image_histogram_set_histogram_linear(priv->imageHistogram, histogram_linear);
}


static void entangle_camera_manager_update_background_highlight(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    GHashTableIter iter;
    void *key;
    void *value;
    gchar *bg = entangle_preferences_img_get_background(prefs);
    gchar *hl = entangle_preferences_img_get_highlight(prefs);

    memset(&iter, 0, sizeof(iter));

    entangle_image_display_set_background(priv->imageDisplay, bg);
    entangle_session_browser_set_background(priv->sessionBrowser, bg);
    entangle_session_browser_set_highlight(priv->sessionBrowser, hl);
    if (priv->imagePresentation)
        entangle_image_popup_set_background(priv->imagePresentation, bg);

    g_hash_table_iter_init(&iter, priv->popups);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        EntangleImagePopup *pop = value;
        entangle_image_popup_set_background(pop, bg);
    }
    g_free(bg);
    g_free(hl);
}


static void do_presentation_monitor_toggled(GtkCheckMenuItem *menu, gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;
    gpointer mon = g_object_get_data(G_OBJECT(menu), "monitor");

    priv->presentationMonitor = GPOINTER_TO_INT(mon);

    ENTANGLE_DEBUG("Set monitor %d", priv->presentationMonitor);

    if (priv->imagePresentation)
        entangle_image_popup_move_to_monitor(priv->imagePresentation,
                                             priv->presentationMonitor);
}


static GtkWidget *entangle_camera_manager_monitor_menu(EntangleCameraManager *manager)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager), NULL);

    EntangleCameraManagerPrivate *priv = manager->priv;
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(manager));
    GtkWidget *menu = gtk_menu_new();
    GSList *group = NULL;
#ifdef gdk_screen_get_primary_monitor
    int active = gdk_screen_get_primary_monitor(screen);
#else
    int active = 0;
#endif

    for (int i = 0; i < gdk_screen_get_n_monitors(screen); i++) {
        const gchar *name = gdk_screen_get_monitor_plug_name(screen, i);
        GtkWidget *submenu = gtk_radio_menu_item_new_with_label(group, name);
        g_object_set_data(G_OBJECT(submenu), "monitor", GINT_TO_POINTER(i));
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(submenu));

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

        if (i == active)
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(submenu), TRUE);

        g_signal_connect(submenu, "toggled",
                         G_CALLBACK(do_presentation_monitor_toggled), manager);
    }

    priv->presentationMonitor = active;
    gtk_widget_show_all(menu);

    return menu;
}


static void entangle_camera_manager_update_viewfinder(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->taskPreview) {
        EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
        gint gridLines = entangle_preferences_img_get_grid_lines(prefs);
        gboolean focusPoint = entangle_preferences_img_get_focus_point(prefs);

        entangle_image_display_set_focus_point(priv->imageDisplay, focusPoint);
        entangle_image_display_set_grid_display(priv->imageDisplay, gridLines);
    } else {
        entangle_image_display_set_focus_point(priv->imageDisplay, FALSE);
        entangle_image_display_set_grid_display(priv->imageDisplay,
                                                ENTANGLE_IMAGE_DISPLAY_GRID_NONE);
    }
}


static void entangle_camera_manager_update_image_loader(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    gboolean embeddedPreview = entangle_preferences_img_get_embedded_preview(prefs);
    entangle_image_loader_set_embedded_preview(priv->imageLoader, embeddedPreview);
}


static void entangle_camera_manager_update_automata(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    gboolean deleteFile = entangle_preferences_capture_get_delete_file(prefs);

    entangle_camera_automata_set_delete_file(priv->automata, deleteFile);
}


static void entangle_camera_manager_prefs_changed(GObject *object G_GNUC_UNUSED,
                                                  GParamSpec *spec,
                                                  gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);

    if (g_str_equal(spec->name, "interface-histogram-linear")) {
        entangle_camera_manager_update_histogram_linear(manager);
    } else if (g_str_equal(spec->name, "cms-enabled") ||
        g_str_equal(spec->name, "cms-rgb-profile") ||
        g_str_equal(spec->name, "cms-monitor-profile") ||
        g_str_equal(spec->name, "cms-detect-system-profile") ||
        g_str_equal(spec->name, "cms-rendering-intent")) {
        entangle_camera_manager_update_colour_transform(manager);
    } else if (g_str_equal(spec->name, "img-aspect-ratio")) {
        entangle_camera_manager_update_aspect_ratio(manager);
    } else if (g_str_equal(spec->name, "img-mask-opacity")) {
        entangle_camera_manager_update_mask_opacity(manager);
    } else if (g_str_equal(spec->name, "img-mask-enabled")) {
        entangle_camera_manager_update_mask_enabled(manager);
    } else if (g_str_equal(spec->name, "img-focus-point") ||
               g_str_equal(spec->name, "img-grid-lines")) {
        entangle_camera_manager_update_viewfinder(manager);
    } else if (g_str_equal(spec->name, "img-embedded-preview")) {
        entangle_camera_manager_update_image_loader(manager);
    } else if (g_str_equal(spec->name, "capture-delete-file")) {
        entangle_camera_manager_update_automata(manager);
    } else if (g_str_equal(spec->name, "img-onion-skin") ||
               g_str_equal(spec->name, "img-onion-layers")) {
        EntangleCameraManagerPrivate *priv = manager->priv;
        do_select_image(manager, priv->currentImage);
    } else if (g_str_equal(spec->name, "img-background") ||
               g_str_equal(spec->name, "img-highlight")) {
        EntangleCameraManagerPrivate *priv = manager->priv;
        do_select_image(manager, priv->currentImage);

        entangle_camera_manager_update_background_highlight(manager);
    }
}


static void do_capture_widget_sensitivity(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolCapture;
    GtkWidget *toolPreview;
    GtkWidget *toolCancel;
    GtkWidget *menuCapture;
    GtkWidget *menuPreview;
    GtkWidget *menuCancel;

    GtkWidget *toolSession;
    GtkWidget *menuSession;
    GtkWidget *menuConnect;
    GtkWidget *menuDisconnect;


    toolCapture = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-capture"));
    toolPreview = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-preview"));
    toolCancel = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-cancel"));
    menuCapture = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-capture"));
    menuPreview = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-preview"));
    menuCancel = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-cancel"));

    toolSession = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-session"));
    menuSession = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-session"));
    menuConnect = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-connect"));
    menuDisconnect = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-disconnect"));


    gtk_widget_set_sensitive(toolCapture,
                             priv->cameraReady && !priv->taskCapture && priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) ? TRUE : FALSE);
    gtk_widget_set_sensitive(toolPreview,
                             priv->cameraReady && !priv->taskCapture && priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) &&
                             entangle_camera_get_has_preview(priv->camera) &&
                             !priv->taskCapture ? TRUE : FALSE);
    gtk_widget_set_sensitive(menuCapture,
                             priv->cameraReady && !priv->taskCapture && priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) ? TRUE : FALSE);
    gtk_widget_set_sensitive(menuPreview,
                             priv->cameraReady && !priv->taskCapture && priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) &&
                             entangle_camera_get_has_preview(priv->camera) &&
                             !priv->taskCapture ? TRUE : FALSE);

    gtk_widget_set_sensitive(toolSession,
                             !priv->taskActive);
    gtk_widget_set_sensitive(menuSession,
                             !priv->taskActive);
    gtk_widget_set_sensitive(menuConnect,
                             priv->camera ? FALSE : TRUE);
    gtk_widget_set_sensitive(menuDisconnect,
                             priv->camera && priv->cameraReady ? TRUE : FALSE);

    gtk_widget_set_tooltip_text(toolCapture, _("Capture an image"));
    gtk_widget_set_tooltip_text(toolPreview, _("Continuous capture preview"));

    if (priv->camera) {
        if (!entangle_camera_get_has_capture(priv->camera))
            gtk_widget_set_tooltip_text(toolCapture, _("This camera does not support image capture"));
        if (!entangle_camera_get_has_capture(priv->camera) ||
            !entangle_camera_get_has_preview(priv->camera))
            gtk_widget_set_tooltip_text(toolPreview, _("This camera does not support image preview"));
    }

#if 0
    if (priv->camera &&
        hasControls) {
        gtk_widget_show(settingsScroll);
        gtk_widget_set_sensitive(settingsScroll, priv->cameraReady);
    } else {
        gtk_widget_hide(settingsScroll);
    }
#endif

    if (priv->taskCapture) {
        gtk_widget_set_sensitive(toolCancel, True);
        gtk_widget_set_sensitive(menuCancel, True);
    } else {
        gtk_widget_set_sensitive(toolCancel, False);
        gtk_widget_set_sensitive(menuCancel, False);
    }

    entangle_camera_manager_update_viewfinder(manager);
}


static void do_restore_scroll(GtkWidget *widget,
                              GdkRectangle *allocation G_GNUC_UNUSED,
                              EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));
    GtkAdjustment *hadjust;
    GtkAdjustment *vadjust;
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!entangle_image_display_get_loaded(ENTANGLE_IMAGE_DISPLAY(widget)))
        return;

    hadjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(priv->imageScroll));
    vadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(priv->imageScroll));

    gtk_adjustment_set_value(hadjust,
                             priv->imageScrollHOffset);
    gtk_adjustment_set_value(vadjust,
                             priv->imageScrollVOffset);
    priv->imageScrollRestored = TRUE;
}


static void do_select_image(EntangleCameraManager *manager,
                            EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));
    g_return_if_fail(!image || ENTANGLE_IS_IMAGE(image));
    GList *newimages = NULL;
    GList *oldimages;
    GList *tmp;
    GtkAdjustment *hadjust;
    GtkAdjustment *vadjust;

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);

    ENTANGLE_DEBUG("Selected image %p %s", image,
                   image ? entangle_image_get_filename(image) : "<none>");

    if (image) {
        if (entangle_preferences_img_get_onion_skin(prefs))
            newimages = entangle_session_browser_earlier_images
                (priv->sessionBrowser,
                 entangle_image_get_filename(image) == NULL,
                 entangle_preferences_img_get_onion_layers(prefs));

        newimages = g_list_prepend(newimages, g_object_ref(image));
    }

    /* Load all new images first */
    tmp = newimages;
    while (tmp) {
        EntangleImage *thisimage = tmp->data;
        ENTANGLE_DEBUG("New image %p %s", thisimage,
                       entangle_image_get_filename(thisimage));

        if (entangle_image_get_filename(thisimage))
            entangle_pixbuf_loader_load(ENTANGLE_PIXBUF_LOADER(priv->imageLoader),
                                        thisimage);

        tmp = tmp->next;
    }

    /* Now unload old images */
    tmp = oldimages = entangle_image_display_get_image_list(priv->imageDisplay);
    while (tmp) {
        EntangleImage *thisimage = tmp->data;

        ENTANGLE_DEBUG("Old %p %s", thisimage,
                       entangle_image_get_filename(thisimage));

        if (entangle_image_get_filename(thisimage))
            entangle_pixbuf_loader_unload(ENTANGLE_PIXBUF_LOADER(priv->imageLoader),
                                          thisimage);

        tmp = tmp->next;
    }

    hadjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(priv->imageScroll));
    vadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(priv->imageScroll));

    if (priv->imageScrollRestored) {
        priv->imageScrollHOffset = gtk_adjustment_get_value(hadjust);
        priv->imageScrollVOffset = gtk_adjustment_get_value(vadjust);
        priv->imageScrollRestored = FALSE;
    }

    entangle_image_display_set_image_list(priv->imageDisplay, newimages);

    if (image)
        g_object_ref(image);
    if (priv->currentImage)
        g_object_unref(priv->currentImage);
    priv->currentImage = image;

    entangle_image_statusbar_set_image(priv->imageStatusbar, priv->currentImage);
    entangle_image_histogram_set_image(priv->imageHistogram, priv->currentImage);
    if (priv->imagePresentation)
        entangle_image_popup_set_image(priv->imagePresentation, priv->currentImage);

    g_list_foreach(oldimages, (GFunc)g_object_unref, NULL);
    g_list_free(oldimages);
    g_list_foreach(newimages, (GFunc)g_object_unref, NULL);
    g_list_free(newimages);
}


static void do_camera_task_error(EntangleCameraManager *manager,
                                 const char *label, GError *error)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                            0,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Operation: %s"), label);
    gtk_window_set_title(GTK_WINDOW(msg),
                         _("Entangle: Operation failed"));
    if (error)
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg),
                                                 "%s",
                                                 error->message);
    g_signal_connect_swapped(msg,
                             "response",
                             G_CALLBACK(gtk_widget_destroy),
                             msg);
    gtk_widget_show_all(msg);
}


static void do_camera_process_events(EntangleCameraManager *manager);


static void do_camera_load_controls_refresh_finish(GObject *source,
                                                   GAsyncResult *result,
                                                   gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (!entangle_camera_load_controls_finish(camera, result, &error)) {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                0,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Camera load controls failed"));
        gtk_window_set_title(GTK_WINDOW(msg),
                             _("Entangle: Camera load controls failed"));
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

    g_cancellable_reset(priv->monitorCancel);

    do_camera_process_events(manager);
}


static void do_camera_process_events_finish(GObject *src,
                                            GAsyncResult *result,
                                            gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    priv->taskProcessEvents = FALSE;

    if (!entangle_camera_process_events_finish(camera,
                                               result,
                                               &error)) {
        if (g_cancellable_is_cancelled(priv->monitorCancel)) {
            g_cancellable_reset(priv->monitorCancel);
        } else {
            do_camera_task_error(manager, _("Monitor"), error);
        }
        g_error_free(error);
        return;
    }

    if (!priv->cameraReady) {
        priv->cameraReady = TRUE;
        entangle_camera_automata_set_camera(priv->automata, priv->camera);
        do_capture_widget_sensitivity(manager);
    }

    if (!priv->camera)
        return;

    if (priv->cameraChanged) {
        priv->cameraChanged = FALSE;
        entangle_camera_load_controls_async(priv->camera,
                                            NULL,
                                            do_camera_load_controls_refresh_finish,
                                            manager);
    } else {
        do_camera_process_events(manager);
    }
}


static void do_camera_process_events(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->camera)
        return;

    if (priv->taskProcessEvents)
        return;

    entangle_camera_process_events_async(priv->camera, 500, priv->monitorCancel,
                                         do_camera_process_events_finish, manager);

    priv->taskProcessEvents = TRUE;
}


static gboolean do_camera_task_begin(EntangleCameraManager *manager)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager), FALSE);

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->taskActive)
        return FALSE;

    g_cancellable_cancel(priv->monitorCancel);
    g_cancellable_reset(priv->taskConfirm);
    g_cancellable_reset(priv->taskCancel);
    priv->taskActive = TRUE;

    return TRUE;
}


static void do_camera_task_complete(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->taskActive = priv->taskPreview = priv->taskCapture = FALSE;

    do_capture_widget_sensitivity(manager);

    g_cancellable_reset(priv->taskConfirm);
    g_cancellable_reset(priv->taskCancel);
    g_cancellable_reset(priv->monitorCancel);

    do_camera_process_events(manager);
}


static void do_camera_file_preview(EntangleCamera *cam G_GNUC_UNUSED, EntangleCameraFile *file, void *data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));
    g_return_if_fail(ENTANGLE_IS_CAMERA_FILE(file));

    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;
    GdkPixbuf *pixbuf;
    GByteArray *bytes;
    GInputStream *is;
    EntangleImage *image = NULL;

    if (priv->taskPreview &&
        priv->taskCancel  &&
        !g_cancellable_is_cancelled(priv->taskCancel)) {
        ENTANGLE_DEBUG("File preview %p %p %p", cam, file, data);

        bytes = entangle_camera_file_get_data(file);
        is = g_memory_input_stream_new_from_data(bytes->data, bytes->len, NULL);

        pixbuf = gdk_pixbuf_new_from_stream(is, NULL, NULL);

        if (priv->taskCapture) {
            char *localpath = entangle_session_next_filename(priv->session, file);

            if (!entangle_camera_file_save_path(file, localpath, NULL)) {
                ENTANGLE_DEBUG("Failed save path");
            } else {
                ENTANGLE_DEBUG("Saved to %s", localpath);
                image = entangle_image_new_file(localpath);

                entangle_session_add(priv->session, image);
            }
            g_free(localpath);
            priv->taskCapture = FALSE;
        }
        if (!image)
            image = entangle_image_new_pixbuf(pixbuf);

        do_select_image(manager, image);

        g_object_unref(pixbuf);
        g_object_unref(is);
        g_object_unref(image);
    }
}


static void do_entangle_camera_progress_start(EntangleProgress *iface,
                                              float target G_GNUC_UNUSED,
                                              const char *msg)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(iface));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;

    if (priv->taskPreview && !g_cancellable_is_cancelled(priv->taskConfirm))
        return;

    mtr = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-spinner"));
    gtk_widget_set_tooltip_text(mtr, msg);
    gtk_spinner_start(GTK_SPINNER(mtr));
}


static void do_entangle_camera_progress_update(EntangleProgress *iface G_GNUC_UNUSED,
                                               float current G_GNUC_UNUSED)
{
}


static void do_entangle_camera_progress_stop(EntangleProgress *iface)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(iface));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;

    if (priv->taskPreview && !g_cancellable_is_cancelled(priv->taskConfirm))
        return;

    mtr = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-spinner"));
    gtk_widget_set_tooltip_text(mtr, "");
    gtk_spinner_stop(GTK_SPINNER(mtr));
}


static void entangle_camera_progress_interface_init(gpointer g_iface,
                                                    gpointer iface_data G_GNUC_UNUSED)
{
    EntangleProgressInterface *iface = g_iface;
    iface->start = do_entangle_camera_progress_start;
    iface->update = do_entangle_camera_progress_update;
    iface->stop = do_entangle_camera_progress_stop;
}


static void do_entangle_camera_manager_set_builder(EntangleWindow *window,
                                                   GtkBuilder *builder);
static GtkBuilder *do_entangle_camera_manager_get_builder(EntangleWindow *window);

static void entangle_camera_manager_window_interface_init(gpointer g_iface,
                                                          gpointer iface_data G_GNUC_UNUSED)
{
    EntangleWindowInterface *iface = g_iface;
    iface->set_builder = do_entangle_camera_manager_set_builder;
    iface->get_builder = do_entangle_camera_manager_get_builder;
}

static void do_remove_camera(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    g_cancellable_cancel(priv->monitorCancel);
    g_cancellable_cancel(priv->taskCancel);

    gtk_window_set_title(GTK_WINDOW(manager), _("Camera Manager - Entangle"));

    entangle_camera_preferences_set_camera(priv->cameraPrefs, NULL);
    entangle_camera_set_progress(priv->camera, NULL);

    g_signal_handler_disconnect(priv->camera, priv->sigFilePreview);

    entangle_camera_automata_set_camera(priv->automata, NULL);

    if (priv->imagePresentation) {
        gtk_widget_hide(GTK_WIDGET(priv->imagePresentation));
        g_object_unref(priv->imagePresentation);
        priv->imagePresentation = NULL;
    }
}


static void do_camera_load_controls_finish(GObject *source,
                                           GAsyncResult *result,
                                           gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;
    gint64 epochsecs = g_get_real_time() / (1000 * 1000);
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    EntangleCamera *cam = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (entangle_camera_load_controls_finish(cam, result, &error)) {
        do_capture_widget_sensitivity(manager);
        entangle_camera_preferences_set_camera(priv->cameraPrefs, priv->camera);
    } else {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                0,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Camera load controls failed"));
        gtk_window_set_title(GTK_WINDOW(msg),
                             _("Entangle: Camera load controls failed"));
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

    g_cancellable_reset(priv->monitorCancel);

    if (priv->camera && 0 &&
        entangle_preferences_capture_get_sync_clock(prefs))
        entangle_camera_set_clock_async(priv->camera,
                                        epochsecs,
                                        NULL,
                                        do_camera_sync_clock_finish,
                                        manager);

    do_camera_process_events(manager);
}


static void do_camera_connect_finish(GObject *source,
                                     GAsyncResult *result,
                                     gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCamera *cam = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (entangle_camera_connect_finish(cam, result, &error)) {
        entangle_camera_load_controls_async(priv->camera,
                                            NULL,
                                            do_camera_load_controls_finish,
                                            manager);
    } else {
        int response;
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_NONE,
                                                _("Unable to connect to camera: %s"),
                                                error->message);

        gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(msg),
                                                   "%s",
                                                   _("Check that the camera is not\n\n"
                                                     " - opened by another photo <b>application</b>\n"
                                                     " - mounted as a <b>filesystem</b> on the desktop\n"
                                                     " - in <b>sleep mode</b> to save battery power\n"));

        gtk_dialog_add_button(GTK_DIALOG(msg), _("Cancel"), GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button(GTK_DIALOG(msg), _("Retry"), GTK_RESPONSE_ACCEPT);
        gtk_dialog_set_default_response(GTK_DIALOG(msg), GTK_RESPONSE_ACCEPT);
        response = gtk_dialog_run(GTK_DIALOG(msg));

        gtk_widget_destroy(msg);

        if (response == GTK_RESPONSE_CANCEL) {
            entangle_camera_manager_set_camera(manager, NULL);
        } else {
            entangle_camera_connect_async(cam,
                                          NULL,
                                          do_camera_connect_finish,
                                          manager);
        }
        g_error_free(error);
    }
}


static gboolean need_camera_unmount(EntangleCameraManager *manager,
                                    EntangleCamera *cam)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA(cam), FALSE);

    if (entangle_camera_is_mounted(cam)) {
        int response;
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_NONE,
                                                "%s",
                                                _("Camera is in use"));

        gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(msg),
                                                   "%s",
                                                   _("The camera cannot be opened because it is "
                                                     "currently mounted as a filesystem. Do you "
                                                     "wish to umount it now ?"));

        gtk_dialog_add_button(GTK_DIALOG(msg), _("No"), GTK_RESPONSE_NO);
        gtk_dialog_add_button(GTK_DIALOG(msg), _("Yes"), GTK_RESPONSE_YES);
        gtk_dialog_set_default_response(GTK_DIALOG(msg), GTK_RESPONSE_YES);

        response = gtk_dialog_run(GTK_DIALOG(msg));

        gtk_widget_destroy(msg);

        if (response == GTK_RESPONSE_YES)
            return TRUE;
    }
    return FALSE;
}


static void do_camera_unmount_finish(GObject *source,
                                     GAsyncResult *result,
                                     gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCamera *cam = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (entangle_camera_unmount_finish(cam, result, &error)) {
        entangle_camera_connect_async(priv->camera,
                                      NULL,
                                      do_camera_connect_finish,
                                      manager);
    } else {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                0,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Camera connect failed"));
        gtk_window_set_title(GTK_WINDOW(msg),
                             _("Entangle: Camera connect failed"));
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg),
                                                 "%s",
                                                 error->message);
        g_signal_connect_swapped(msg,
                                 "response",
                                 G_CALLBACK(gtk_widget_destroy),
                                 msg);
        gtk_widget_show_all(msg);
        entangle_camera_manager_set_camera(manager, NULL);

        g_error_free(error);
    }
}


static void do_camera_control_changed(EntangleCamera *cam G_GNUC_UNUSED,
                                      gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->cameraChanged = TRUE;
}


static void do_add_camera(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    char *title;

    title = g_strdup_printf(_("%s Camera Manager - Entangle"),
                            entangle_camera_get_model(priv->camera));

    gtk_window_set_title(GTK_WINDOW(manager), title);
    g_free(title);

    priv->sigFilePreview = g_signal_connect(priv->camera, "camera-file-previewed",
                                            G_CALLBACK(do_camera_file_preview), manager);
    priv->sigChanged = g_signal_connect(priv->camera, "camera-controls-changed",
                                        G_CALLBACK(do_camera_control_changed), manager);

    entangle_camera_set_progress(priv->camera, ENTANGLE_PROGRESS(manager));

    if (need_camera_unmount(manager, priv->camera)) {
        entangle_camera_unmount_async(priv->camera,
                                      NULL,
                                      do_camera_unmount_finish,
                                      manager);
    } else {
        entangle_camera_connect_async(priv->camera,
                                      NULL,
                                      do_camera_connect_finish,
                                      manager);
    }
}


static void entangle_camera_manager_get_property(GObject *object,
                                                 guint prop_id,
                                                 GValue *value,
                                                 GParamSpec *pspec)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(object);
    EntangleCameraManagerPrivate *priv = manager->priv;

    switch (prop_id)
        {
        case PROP_CAMERA:
            g_value_set_object(value, priv->camera);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void do_camera_removed(EntangleCameraList *cameras G_GNUC_UNUSED,
                              EntangleCamera *camera,
                              gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->camera == camera)
        entangle_camera_manager_set_camera(manager, NULL);
}


static void do_session_image_added(EntangleSession *session G_GNUC_UNUSED,
                                   EntangleImage *img,
                                   gpointer data)
{
    EntangleCameraManager *manager = data;

    do_select_image(manager, img);
}

static void do_camera_manager_set_session(EntangleCameraManager *manager,
                                          EntangleSession *session)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->session) {
        entangle_camera_automata_set_session(priv->automata, priv->session);
        g_signal_handler_disconnect(priv->camera, priv->sigImageAdd);
        priv->sigImageAdd = 0;
        g_object_unref(priv->session);
        priv->session = NULL;
    }
    if (session) {
        EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
        priv->session = g_object_ref(session);
        entangle_session_load(priv->session);
        priv->sigImageAdd = g_signal_connect(priv->session,
                                             "session-image-added",
                                             G_CALLBACK(do_session_image_added), manager);

        entangle_preferences_capture_set_last_session(prefs,
                                                      entangle_session_directory(priv->session));
        entangle_camera_automata_set_session(priv->automata, priv->session);
    }
    entangle_session_browser_set_session(priv->sessionBrowser, priv->session);
}


static void do_entangle_camera_manager_set_app(GObject *object,
                                               GParamSpec *spec G_GNUC_UNUSED)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(object));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(object);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs;
    EntangleCameraList *cameras;
    gchar *directory;
    gchar *pattern;
    GtkWidget *chooser;
    EntangleApplication *app;
    EntangleSession *session;

    app = ENTANGLE_APPLICATION(gtk_window_get_application(GTK_WINDOW(manager)));

    prefs = entangle_application_get_preferences(app);
    priv->sigPrefsNotify = g_signal_connect(prefs,
                                            "notify",
                                            G_CALLBACK(entangle_camera_manager_prefs_changed),
                                            manager);
    cameras = entangle_application_get_active_cameras(app);
    g_signal_connect(cameras, "camera-removed", G_CALLBACK(do_camera_removed), manager);
    directory = entangle_preferences_capture_get_last_session(prefs);
    pattern = entangle_preferences_capture_get_filename_pattern(prefs);

    entangle_camera_manager_update_histogram_linear(manager);
    entangle_camera_manager_update_colour_transform(manager);
    entangle_camera_manager_update_aspect_ratio(manager);
    entangle_camera_manager_update_mask_opacity(manager);
    entangle_camera_manager_update_mask_enabled(manager);
    entangle_camera_manager_update_image_loader(manager);
    entangle_camera_manager_update_background_highlight(manager);
    entangle_camera_manager_update_automata(manager);

    session = entangle_session_new(directory, pattern);
    do_camera_manager_set_session(manager, session);
    g_object_unref(session);

    chooser = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-session-button"));
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), directory);

    g_free(directory);
    g_free(pattern);
}

static void entangle_camera_manager_set_property(GObject *object,
                                                 guint prop_id,
                                                 const GValue *value,
                                                 GParamSpec *pspec)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(object);

    ENTANGLE_DEBUG("Set prop %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            entangle_camera_manager_set_camera(manager, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}



static void entangle_camera_manager_finalize(GObject *object)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(object);
    EntangleCameraManagerPrivate *priv = manager->priv;

    ENTANGLE_DEBUG("Finalize manager");

    g_free(priv->deleteImageDup);

    g_object_unref(priv->scriptConfig);

    g_object_unref(priv->monitorCancel);
    g_object_unref(priv->taskCancel);
    g_object_unref(priv->taskConfirm);

    if (priv->imageLoader)
        g_object_unref(priv->imageLoader);
    if (priv->thumbLoader)
        g_object_unref(priv->thumbLoader);
    if (priv->colourTransform)
        g_object_unref(priv->colourTransform);
    if (priv->camera)
        g_object_unref(priv->camera);
    if (priv->prefsDisplay)
        g_object_unref(priv->prefsDisplay);
    if (priv->picker)
        g_object_unref(priv->picker);

    if (priv->imagePresentation)
        g_object_unref(priv->imagePresentation);

    if (priv->builder)
        g_object_unref(priv->builder);

    g_hash_table_destroy(priv->popups);

    g_object_unref(priv->cameraPrefs);
    g_object_unref(priv->automata);

    G_OBJECT_CLASS(entangle_camera_manager_parent_class)->finalize(object);
}


static void entangle_camera_manager_class_init(EntangleCameraManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_camera_manager_finalize;
    object_class->get_property = entangle_camera_manager_get_property;
    object_class->set_property = entangle_camera_manager_set_property;

    g_signal_new("closed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraManagerClass, manager_connect),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to be managed",
                                                        ENTANGLE_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleCameraManagerPrivate));
}


void do_menu_help_supported(GtkMenuItem *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->supported) {
        EntangleApplication *app = ENTANGLE_APPLICATION(gtk_window_get_application(GTK_WINDOW(manager)));
        priv->supported = entangle_camera_support_new();
        entangle_camera_support_set_camera_list(priv->supported,
                                                entangle_application_get_supported_cameras(app));
        gtk_window_set_transient_for(GTK_WINDOW(priv->supported),
                                     GTK_WINDOW(manager));
    }

    gtk_widget_show(GTK_WIDGET(priv->supported));
}


void do_menu_new_window(GtkImageMenuItem *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    GtkApplication *app = gtk_window_get_application(GTK_WINDOW(manager));
    EntangleCameraManager *newmanager = entangle_camera_manager_new();
    gtk_window_set_application(GTK_WINDOW(newmanager), app);

    gtk_widget_show(GTK_WIDGET(newmanager));
    gtk_window_present(GTK_WINDOW(newmanager));
}


void do_toolbar_select_session(GtkFileChooserButton *src,
                               EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    EntangleSession *session;
    gchar *pattern;
    gchar *dir;

    do_select_image(manager, NULL);
    dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(src));
    pattern = entangle_preferences_capture_get_filename_pattern(prefs);
    session = entangle_session_new(dir, pattern);
    g_free(dir);
    g_free(pattern);
    g_hash_table_remove_all(priv->popups);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(src), dir);
    do_camera_manager_set_session(manager, session);
    g_object_unref(session);
}


void do_menu_select_session(GtkImageMenuItem *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *chooser;
    gchar *dir;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);

    chooser = gtk_file_chooser_dialog_new(_("Select a folder"),
                                          GTK_WINDOW(manager),
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                          _("_Cancel"),
                                          GTK_RESPONSE_REJECT,
                                          _("_Open"),
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser), TRUE);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), FALSE);

    dir = entangle_preferences_capture_get_last_session(prefs);
    g_mkdir_with_parents(dir, 0777);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
    g_free(dir);

    gtk_widget_hide(chooser);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        GtkWidget *toolchooser = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-session-button"));
        EntangleSession *session;
        gchar *pattern;
        do_select_image(manager, NULL);
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        pattern = entangle_preferences_capture_get_filename_pattern(prefs);
        session = entangle_session_new(dir, pattern);
        g_free(dir);
        g_free(pattern);
        g_hash_table_remove_all(priv->popups);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(toolchooser), dir);
        do_camera_manager_set_session(manager, session);
        g_object_unref(session);
    }

    gtk_widget_destroy(chooser);
}


void do_menu_settings_toggled(GtkCheckMenuItem *src,
                              EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *settings;
    GtkWidget *toolbar;
    gboolean active;

    settings = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-scroll"));
    toolbar = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-settings"));

    active = gtk_check_menu_item_get_active(src);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolbar), active);

    if (active)
        gtk_widget_show(settings);
    else
        gtk_widget_hide(settings);
}


void do_toolbar_settings(GtkToggleToolButton *src,
                         EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *settings;
    GtkWidget *menu;
    gboolean active;

    settings = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-scroll"));
    menu = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-settings"));

    active = gtk_toggle_tool_button_get_active(src);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), active);

    if (active)
        gtk_widget_show(settings);
    else
        gtk_widget_hide(settings);
}


void do_toolbar_cancel_clicked(GtkToolButton *src G_GNUC_UNUSED,
                               EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->taskCancel)
        g_cancellable_cancel(priv->taskCancel);
}


static void do_camera_manager_capture(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(priv->builder, "menu-sync-windows"));

    if (gtk_check_menu_item_get_active(item)) {
        GtkApplication *app = gtk_window_get_application(GTK_WINDOW(manager));
        GList *windows = gtk_application_get_windows(app);

        while (windows) {
            GtkWindow *window = windows->data;

            if (ENTANGLE_IS_CAMERA_MANAGER(window))
                entangle_camera_manager_capture(ENTANGLE_CAMERA_MANAGER(window));

            windows = windows->next;
        }
    } else {
        entangle_camera_manager_capture(manager);
    }
}


static void do_camera_manager_preview_begin(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(priv->builder, "menu-sync-windows"));

    if (gtk_check_menu_item_get_active(item)) {
        GtkApplication *app = gtk_window_get_application(GTK_WINDOW(manager));
        GList *windows = gtk_application_get_windows(app);

        while (windows) {
            GtkWindow *window = windows->data;
            if (ENTANGLE_IS_CAMERA_MANAGER(window))
                entangle_camera_manager_preview_begin(ENTANGLE_CAMERA_MANAGER(window));

            windows = windows->next;
        }
    } else {
        entangle_camera_manager_preview_begin(manager);
    }
}


static void do_camera_manager_preview_cancel(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(priv->builder, "menu-sync-windows"));

    if (gtk_check_menu_item_get_active(item)) {
        GtkApplication *app = gtk_window_get_application(GTK_WINDOW(manager));
        GList *windows = gtk_application_get_windows(app);

        while (windows) {
            GtkWindow *window = windows->data;
            if (ENTANGLE_IS_CAMERA_MANAGER(window))
                entangle_camera_manager_preview_cancel(ENTANGLE_CAMERA_MANAGER(window));

            windows = windows->next;
        }
    } else {
        entangle_camera_manager_preview_cancel(manager);
    }
}


static void do_entangle_camera_manager_capture_finish(GObject *src,
                                                      GAsyncResult *res,
                                                      gpointer opaque)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(src));
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(opaque));

    EntangleCameraAutomata *automata = ENTANGLE_CAMERA_AUTOMATA(src);
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    GError *error = NULL;

    if (!entangle_camera_automata_capture_finish(automata,
                                                 res,
                                                 &error)) {
        do_camera_task_error(manager, _("Capture"), error);
        g_error_free(error);
    }

    do_camera_task_complete(manager);
}


static void do_entangle_camera_manager_script_finish(GObject *src,
                                                     GAsyncResult *res,
                                                     gpointer opaque)
{
    g_return_if_fail(ENTANGLE_IS_SCRIPT(src));
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(opaque));
    EntangleScript *script = ENTANGLE_SCRIPT(src);
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    GError *error = NULL;

    if (!entangle_script_execute_finish(script,
                                        res,
                                        &error)) {
        do_camera_task_error(manager, _("Script"), error);
        if (error)
            g_error_free(error);
    }

    do_camera_task_complete(manager);
    g_object_unref(script);
}


void entangle_camera_manager_capture(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    ENTANGLE_DEBUG("starting capture operation");

    if (!priv->camera)
        return;

    if (priv->taskPreview) {
        if (entangle_preferences_capture_get_continuous_preview(prefs) &&
            entangle_preferences_capture_get_electronic_shutter(prefs))
            priv->taskCapture = TRUE;
        else
            g_cancellable_cancel(priv->taskConfirm);
    } else {
        EntangleScript *script;
        if (!do_camera_task_begin(manager))
            return;

        priv->taskCapture = TRUE;
        do_capture_widget_sensitivity(manager);

        script = entangle_script_config_get_selected(priv->scriptConfig);
    if (script)
            entangle_script_execute_async(script,
                                          priv->automata,
                                          priv->taskCancel,
                                          do_entangle_camera_manager_script_finish,
                                          manager);
        else
            entangle_camera_automata_capture_async(priv->automata,
                                                   priv->taskCancel,
                                                   do_entangle_camera_manager_capture_finish,
                                                   manager);
    }
}


static void do_entangle_camera_manager_preview_finish(GObject *src,
                                                      GAsyncResult *res,
                                                      gpointer opaque)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(src));
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(opaque));

    EntangleCameraAutomata *automata = ENTANGLE_CAMERA_AUTOMATA(src);
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
    GtkWidget *preview;
    GError *error = NULL;

    if (!entangle_camera_automata_preview_finish(automata,
                                                 res,
                                                 &error)) {
        do_camera_task_error(manager, _("Preview"), error);
    }

    if (!error &&
        priv->camera &&
        !g_cancellable_is_cancelled(priv->taskCancel) &&
        entangle_preferences_capture_get_continuous_preview(prefs)) {
        entangle_camera_automata_preview_async(priv->automata,
                                               priv->taskCancel,
                                               priv->taskConfirm,
                                               do_entangle_camera_manager_preview_finish,
                                               manager);
    } else {
        preview = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-preview"));
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(preview), FALSE);

        do_camera_task_complete(manager);
        if (error)
            g_error_free(error);
    }
}


void entangle_camera_manager_preview_begin(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->camera)
        return;

    ENTANGLE_DEBUG("starting preview operation");
    if (!do_camera_task_begin(manager))
        return;

    priv->taskPreview = TRUE;
    do_capture_widget_sensitivity(manager);

    entangle_camera_automata_preview_async(priv->automata,
                                           priv->taskCancel,
                                           priv->taskConfirm,
                                           do_entangle_camera_manager_preview_finish,
                                           manager);
}


void entangle_camera_manager_preview_cancel(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleImage *img;

    if (!priv->camera)
        return;

    if (priv->taskPreview) {
        ENTANGLE_DEBUG("Cancelling capture operation");
        g_cancellable_cancel(priv->taskCancel);

        img = entangle_session_browser_selected_image(priv->sessionBrowser);
        if (img) {
            do_select_image(manager, img);
            g_object_unref(img);
        }
    }
}


void do_toolbar_capture(GtkToolButton *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    do_camera_manager_capture(manager);
}


void do_toolbar_preview(GtkToggleToolButton *src,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(priv->builder, "menu-preview"));

    if (gtk_toggle_tool_button_get_active(src)) {
        if (!gtk_check_menu_item_get_active(item)) {
            gtk_check_menu_item_set_active(item, True);
            do_camera_manager_preview_begin(manager);
        }
    } else {
        if (gtk_check_menu_item_get_active(item)) {
            gtk_check_menu_item_set_active(item, False);
            do_camera_manager_preview_cancel(manager);
        }
    }
}


void do_menu_capture(GtkMenuItem *src G_GNUC_UNUSED,
                     EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    do_camera_manager_capture(manager);
}


void do_menu_preview(GtkMenuItem *src,
                     EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkToggleToolButton *button = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(priv->builder, "toolbar-preview"));

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(src))) {
        if (!gtk_toggle_tool_button_get_active(button)) {
            gtk_toggle_tool_button_set_active(button, True);
            do_camera_manager_preview_begin(manager);
        }
    } else {
        if (gtk_toggle_tool_button_get_active(button)) {
            gtk_toggle_tool_button_set_active(button, False);
            do_camera_manager_preview_cancel(manager);
        }
    }
}


void do_menu_cancel(GtkMenuItem *src G_GNUC_UNUSED,
                    EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->taskCancel)
        g_cancellable_cancel(priv->taskCancel);
}


static void do_camera_sync_clock_finish(GObject *src,
                                        GAsyncResult *res,
                                        gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    if (!entangle_camera_set_clock_finish(camera,
                                          res,
                                          &error)) {
        do_camera_task_error(manager, _("Set clock"), error);
        g_error_free(error);
        return;
    }
}


void do_menu_sync_clock(GtkMenuItem *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));
    gint64 epochsecs = g_get_real_time() / (1000 * 1000);

    EntangleCameraManagerPrivate *priv = manager->priv;

    ENTANGLE_DEBUG("starting sync clock operation");

    if (!priv->camera)
        return;

    entangle_camera_set_clock_async(priv->camera,
                                    epochsecs,
                                    NULL,
                                    do_camera_sync_clock_finish,
                                    manager);
}


static void do_camera_autofocus_finish(GObject *source,
                                       GAsyncResult *result,
                                       gpointer opaque)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(opaque));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCamera *camera = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (!entangle_camera_autofocus_finish(camera, result, &error)) {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                0,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Autofocus failed"));
        gtk_window_set_title(GTK_WINDOW(msg),
                             _("Entangle: Camera autofocus failed"));
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


static void do_camera_manualfocus_finish(GObject *source,
                                         GAsyncResult *result,
                                         gpointer opaque)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(opaque));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCamera *camera = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (!entangle_camera_manualfocus_finish(camera, result, &error)) {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                0,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Manual focus failed"));
        gtk_window_set_title(GTK_WINDOW(msg),
                             _("Entangle: Camera manual focus failed"));
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


gboolean do_manager_key_release(GtkWidget *widget G_GNUC_UNUSED,
                                GdkEventKey *ev,
                                gpointer data)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data), FALSE);

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;

    switch (ev->keyval) {
    case GDK_KEY_m: {
        EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
        gboolean enabled = entangle_preferences_img_get_mask_enabled(prefs);
        entangle_preferences_img_set_mask_enabled(prefs, !enabled);
    }   break;

    case GDK_KEY_h: {
        EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
        gboolean linear = entangle_preferences_interface_get_histogram_linear(prefs);
        entangle_preferences_interface_set_histogram_linear(prefs, !linear);
    }   break;

    case GDK_KEY_a: {
        if (priv->taskPreview) {
            entangle_camera_autofocus_async(priv->camera,
                                            NULL,
                                            do_camera_autofocus_finish,
                                            manager);
        }
    }   break;

    case GDK_KEY_comma: {
        if (priv->taskPreview) {
            entangle_camera_manualfocus_async(priv->camera,
                                              ENTANGLE_CAMERA_MANUAL_FOCUS_STEP_OUT_MEDIUM,
                                              NULL,
                                              do_camera_manualfocus_finish,
                                              manager);
        }
    }   break;

    case GDK_KEY_period: {
        if (priv->taskPreview) {
            entangle_camera_manualfocus_async(priv->camera,
                                              ENTANGLE_CAMERA_MANUAL_FOCUS_STEP_IN_MEDIUM,
                                              NULL,
                                              do_camera_manualfocus_finish,
                                              manager);
        }
    }   break;

    case GDK_KEY_less: {
        if (priv->taskPreview) {
            entangle_camera_manualfocus_async(priv->camera,
                                              ENTANGLE_CAMERA_MANUAL_FOCUS_STEP_OUT_COARSE,
                                              NULL,
                                              do_camera_manualfocus_finish,
                                              manager);
        }
    }   break;

    case GDK_KEY_greater: {
        if (priv->taskPreview) {
            entangle_camera_manualfocus_async(priv->camera,
                                              ENTANGLE_CAMERA_MANUAL_FOCUS_STEP_IN_COARSE,
                                              NULL,
                                              do_camera_manualfocus_finish,
                                              manager);
        }
    }   break;

    default:
        break;
    }

    return FALSE;
}


static void do_zoom_widget_sensitivity(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolzoomnormal = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-zoom-normal"));
    GtkWidget *toolzoombest = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-zoom-best"));
    GtkWidget *toolzoomin = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-zoom-in"));
    GtkWidget *toolzoomout = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-zoom-out"));

    GtkWidget *menuzoomnormal = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-zoom-normal"));
    GtkWidget *menuzoombest = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-zoom-best"));
    GtkWidget *menuzoomin = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-zoom-in"));
    GtkWidget *menuzoomout = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-zoom-out"));
    gboolean autoscale;

    autoscale = entangle_image_display_get_autoscale(priv->imageDisplay);

    if (autoscale) {
        gtk_widget_set_sensitive(toolzoombest, FALSE);
        gtk_widget_set_sensitive(toolzoomnormal, TRUE);
        gtk_widget_set_sensitive(toolzoomin, FALSE);
        gtk_widget_set_sensitive(toolzoomout, FALSE);

        gtk_widget_set_sensitive(menuzoombest, FALSE);
        gtk_widget_set_sensitive(menuzoomnormal, TRUE);
        gtk_widget_set_sensitive(menuzoomin, FALSE);
        gtk_widget_set_sensitive(menuzoomout, FALSE);
    } else {
        gtk_widget_set_sensitive(toolzoombest, TRUE);
        gtk_widget_set_sensitive(toolzoomnormal, priv->zoomLevel == 0 ? FALSE : TRUE);
        gtk_widget_set_sensitive(toolzoomin, priv->zoomLevel == 10 ? FALSE : TRUE);
        gtk_widget_set_sensitive(toolzoomout, priv->zoomLevel == -10 ? FALSE : TRUE);

        gtk_widget_set_sensitive(menuzoombest, TRUE);
        gtk_widget_set_sensitive(menuzoomnormal, priv->zoomLevel == 0 ? FALSE : TRUE);
        gtk_widget_set_sensitive(menuzoomin, priv->zoomLevel == 10 ? FALSE : TRUE);
        gtk_widget_set_sensitive(menuzoomout, priv->zoomLevel == -10 ? FALSE : TRUE);
    }
}


static void entangle_camera_manager_update_zoom(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel > 0)
        entangle_image_display_set_scale(priv->imageDisplay, 1.0+priv->zoomLevel);
    else if (priv->zoomLevel < 0)
        entangle_image_display_set_scale(priv->imageDisplay, 1.0/pow(1.5, -priv->zoomLevel));
    else
        entangle_image_display_set_scale(priv->imageDisplay, 0.0);
    do_zoom_widget_sensitivity(manager);
}


static void entangle_camera_manager_zoom_in(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel < 10)
        priv->zoomLevel += 1;

    entangle_camera_manager_update_zoom(manager);
}


static void entangle_camera_manager_zoom_out(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel > -10)
        priv->zoomLevel -= 1;

    entangle_camera_manager_update_zoom(manager);
}

static void entangle_camera_manager_zoom_normal(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    entangle_image_display_set_scale(priv->imageDisplay, 0.0);
    entangle_image_display_set_autoscale(priv->imageDisplay, FALSE);
    do_zoom_widget_sensitivity(manager);
}


static void entangle_camera_manager_zoom_best(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    entangle_image_display_set_autoscale(priv->imageDisplay, TRUE);
    do_zoom_widget_sensitivity(manager);
}


void do_toolbar_zoom_in(GtkToolButton *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_in(manager);
}


void do_toolbar_zoom_out(GtkToolButton *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_out(manager);
}


void do_toolbar_zoom_normal(GtkToolButton *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_normal(manager);
}


void do_toolbar_zoom_best(GtkToolButton *src G_GNUC_UNUSED,
                          EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_best(manager);
}


void do_menu_zoom_in(GtkImageMenuItem *src G_GNUC_UNUSED,
                     EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_in(manager);
}


void do_menu_zoom_out(GtkImageMenuItem *src G_GNUC_UNUSED,
                      EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_out(manager);
}

void do_menu_zoom_normal(GtkImageMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_normal(manager);
}

void do_menu_zoom_best(GtkImageMenuItem *src G_GNUC_UNUSED,
                       EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_zoom_best(manager);
}


void do_toolbar_fullscreen(GtkToggleToolButton *src,
                           EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *menu = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-fullscreen"));
    GtkWidget *menubar = GTK_WIDGET(gtk_builder_get_object(priv->builder, "win-menubar"));

    if (gtk_toggle_tool_button_get_active(src)) {
        if (0) gtk_widget_hide(menubar);
        gtk_window_fullscreen(GTK_WINDOW(manager));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(manager));
        if (0) gtk_widget_show(menubar);
    }

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) !=
        gtk_toggle_tool_button_get_active(src))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu),
                                       gtk_toggle_tool_button_get_active(src));
}


void do_menu_fullscreen(GtkCheckMenuItem *src,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *tool = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-fullscreen"));
    GtkWidget *menubar = GTK_WIDGET(gtk_builder_get_object(priv->builder, "win-menubar"));

    if (gtk_check_menu_item_get_active(src)) {
        gtk_widget_hide(menubar);
        gtk_window_fullscreen(GTK_WINDOW(manager));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(manager));
        gtk_widget_show(menubar);
    }

    if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(tool)) !=
        gtk_check_menu_item_get_active(src))
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool),
                                          gtk_check_menu_item_get_active(src));
}


static void do_presentation_end(EntangleImagePopup *popup G_GNUC_UNUSED,
                                EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *menu = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-presentation"));

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), FALSE);
}

void do_menu_presentation(GtkCheckMenuItem *src,
                          EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (gtk_check_menu_item_get_active(src)) {
        if (!priv->imagePresentation) {
            EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
            gchar *bg = entangle_preferences_img_get_background(prefs);
            priv->imagePresentation = entangle_image_popup_new();
            entangle_image_popup_set_background(priv->imagePresentation, bg);
            g_signal_connect(priv->imagePresentation, "popup-close", G_CALLBACK(do_presentation_end), manager);
            g_free(bg);
        }
        entangle_image_popup_set_image(priv->imagePresentation, priv->currentImage);
        entangle_image_popup_show_on_monitor(priv->imagePresentation,
                                             priv->presentationMonitor);
    } else if (priv->imagePresentation) {
        gtk_widget_hide(GTK_WIDGET(priv->imagePresentation));
        g_object_unref(priv->imagePresentation);
        priv->imagePresentation = NULL;
    }
}


void do_menu_preferences(GtkCheckMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    if (priv->prefsDisplay == NULL) {
        GtkApplication *app = gtk_window_get_application(GTK_WINDOW(manager));
        priv->prefsDisplay = entangle_preferences_display_new();
        gtk_application_add_window(app, GTK_WINDOW(priv->prefsDisplay));
        gtk_window_set_transient_for(GTK_WINDOW(priv->prefsDisplay),
                                     GTK_WINDOW(manager));
    }

    gtk_widget_show(GTK_WIDGET(priv->prefsDisplay));
}


void do_menu_close(GtkMenuItem *src G_GNUC_UNUSED,
                   EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->prefsDisplay) {
        gtk_widget_destroy(GTK_WIDGET(priv->prefsDisplay));
        priv->prefsDisplay = NULL;
    }
    gtk_widget_destroy(GTK_WIDGET(manager));
}


static gboolean do_manager_delete(GtkWidget *src G_GNUC_UNUSED,
                                  GdkEvent *ev G_GNUC_UNUSED,
                                  EntangleCameraManager *manager)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager), TRUE);

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->prefsDisplay) {
        gtk_widget_destroy(GTK_WIDGET(priv->prefsDisplay));
        priv->prefsDisplay = NULL;
    }
    gtk_widget_destroy(GTK_WIDGET(manager));
    return TRUE;
}


void do_menu_quit(GtkMenuItem *src G_GNUC_UNUSED,
                  EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    GtkApplication *app = gtk_window_get_application(GTK_WINDOW(manager));
    GList *windows = gtk_application_get_windows(app);
    GList *tmp = g_list_copy(windows);

    while (tmp) {
        GtkWidget *window = GTK_WIDGET(tmp->data);
        gtk_widget_destroy(window);
        tmp = tmp->next;
    }
    g_list_free(tmp);
}


void do_menu_help_about(GtkMenuItem *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->about) {
        priv->about = entangle_help_about_new();
        gtk_window_set_transient_for(GTK_WINDOW(priv->about),
                                     GTK_WINDOW(manager));
    }

    gtk_widget_show(GTK_WIDGET(priv->about));
}


void do_menu_help_manual(GtkMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    gtk_show_uri(gtk_widget_get_screen(GTK_WIDGET(src)),
                 "help:entangle",
                 GDK_CURRENT_TIME,
                 NULL);
}


static void do_picker_refresh(EntangleCameraPicker *picker G_GNUC_UNUSED, EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleApplication *app = ENTANGLE_APPLICATION(gtk_window_get_application(GTK_WINDOW(manager)));
    EntangleCameraList *list = entangle_application_get_active_cameras(app);
    entangle_camera_list_refresh(list, NULL);
}


static void do_picker_connect(EntangleCameraPicker *picker G_GNUC_UNUSED,
                              EntangleCamera *cam,
                              gpointer *data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));
    g_return_if_fail(ENTANGLE_IS_CAMERA(cam));

    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(data);
    EntangleCameraManagerPrivate *priv = manager->priv;
#if 0
    GError *error = NULL;
    ENTANGLE_DEBUG("emit connect %p %s", cam, entangle_camera_get_model(cam));
    if (need_camera_unmount(manager, cam))
        do_camera_unmount(cam);

    while (!entangle_camera_connect(cam, &error)) {
        int response;
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(manager),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_NONE,
                                                _("Unable to connect to camera: %s"),
                                                error->message);
        g_error_free(error);

        gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(msg),
                                                   "%s",
                                                   _("Check that the camera is not\n\n"
                                                     " - opened by another photo <b>application</b>\n"
                                                     " - mounted as a <b>filesystem</b> on the desktop\n"
                                                     " - in <b>sleep mode</b> to save battery power\n"));

        gtk_dialog_add_button(GTK_DIALOG(msg), _("Cancel"), GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button(GTK_DIALOG(msg), _("Retry"), GTK_RESPONSE_ACCEPT);
        gtk_dialog_set_default_response(GTK_DIALOG(msg), GTK_RESPONSE_ACCEPT);

        response = gtk_dialog_run(GTK_DIALOG(msg));

        gtk_widget_destroy(msg);

        if (response == GTK_RESPONSE_CANCEL)
            return;
    }
#endif
    entangle_camera_manager_set_camera(manager, cam);
    gtk_widget_hide(GTK_WIDGET(priv->picker));
}


static void do_camera_connect(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleApplication *app = ENTANGLE_APPLICATION(gtk_window_get_application(GTK_WINDOW(manager)));
    EntangleCameraList *cameras = entangle_application_get_active_cameras(app);

    if (!priv->picker) {
        priv->picker = entangle_camera_picker_new();
        entangle_camera_picker_set_camera_list(priv->picker, cameras);
        gtk_window_set_transient_for(GTK_WINDOW(priv->picker),
                                     GTK_WINDOW(manager));
        g_signal_connect(priv->picker, "picker-refresh", G_CALLBACK(do_picker_refresh), manager);
        g_signal_connect(priv->picker, "picker-connect", G_CALLBACK(do_picker_connect), manager);
    }

    gtk_widget_show(GTK_WIDGET(priv->picker));
}


void do_menu_connect(GtkMenuItem *src G_GNUC_UNUSED,
                     EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    do_camera_connect(manager);
}


void do_menu_disconnect(GtkMenuItem *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    entangle_camera_manager_set_camera(manager, NULL);
}


static void do_session_image_selected(GtkIconView *view G_GNUC_UNUSED,
                                      gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleImage *img = entangle_session_browser_selected_image(priv->sessionBrowser);

    ENTANGLE_DEBUG("Image selection changed");
    if (img) {
        ENTANGLE_DEBUG("Try load");
        do_select_image(manager, img);
        g_object_unref(img);
    }
}


void do_menu_session_open_activate(GtkMenuItem *item G_GNUC_UNUSED,
                                   EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    gchar *ctype = NULL;
    GList *files = NULL;
    GAppInfo *info = NULL;
    const gchar *filename;

    if (!priv->sessionBrowserImage)
        return;

    filename = entangle_image_get_filename(priv->sessionBrowserImage);
    ctype = g_content_type_guess(filename,
                                 NULL, 0, NULL);
    if (!ctype)
        return;

    info = g_app_info_get_default_for_type(ctype, FALSE);
    g_free(ctype);
    if (!info)
        return;

    files = g_list_append(files, g_file_new_for_path(filename));

    g_app_info_launch(info, files, NULL, NULL);

    g_list_foreach(files, (GFunc)g_object_unref, NULL);
    g_list_free(files);
}


void do_menu_session_delete_activate(GtkMenuItem *item G_GNUC_UNUSED,
                                     EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GError *error = NULL;

    if (!priv->sessionBrowserImage)
        return;

    if (!entangle_image_delete(priv->sessionBrowserImage, &error)) {
        do_camera_task_error(manager, _("Delete"), error);
        return;
    }
    entangle_session_remove(priv->session, priv->sessionBrowserImage);
}


static void do_session_browser_open_with_app(GtkMenuItem *src,
                                             EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GAppInfo *info = g_object_get_data(G_OBJECT(src), "appinfo");
    GList *files = NULL;
    const gchar *filename;

    if (!info)
        return;

    if (!priv->sessionBrowserImage)
        return;

    filename = entangle_image_get_filename(priv->sessionBrowserImage);
    files = g_list_append(files, g_file_new_for_path(filename));

    g_app_info_launch(info, files, NULL, NULL);

    g_list_foreach(files, (GFunc)g_object_unref, NULL);
    g_list_free(files);
}


static void do_session_browser_open_with_select(GtkMenuItem *src G_GNUC_UNUSED,
                                                EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkDialog *dialog;
    GFile *file;
    GAppInfo *info;
    GList *files = NULL;

    if (!priv->sessionBrowserImage)
        return;

    file = g_file_new_for_path(entangle_image_get_filename(priv->sessionBrowserImage));
    dialog = GTK_DIALOG(gtk_app_chooser_dialog_new(GTK_WINDOW(manager),
                                                   GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   file));

    gtk_dialog_run(dialog);

    info = gtk_app_chooser_get_app_info(GTK_APP_CHOOSER(dialog));

    files = g_list_append(files, file);

    if (info)
        g_app_info_launch(info, files, NULL, NULL);

    g_list_foreach(files, (GFunc)g_object_unref, NULL);
    g_list_free(files);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


static gboolean do_session_browser_popup(EntangleSessionBrowser *browser,
                                         GdkEventButton  *event,
                                         EntangleCameraManager *manager)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager), FALSE);

    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *open;
    GtkWidget *openWith;
    GtkMenu *appList;
    GtkMenuItem *child;
    gchar *ctype = NULL;
    GList *appInfoList = NULL;
    const gchar *filename;

    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;
    if (event->button != 3)
        return FALSE;

    priv->sessionBrowserImage = entangle_session_browser_get_image_at_coords(browser,
                                                                             event->x,
                                                                             event->y);

    if (!priv->sessionBrowserImage)
        return FALSE;

    filename = entangle_image_get_filename(priv->sessionBrowserImage);

    open = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-session-open"));
    openWith = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-session-open-with"));

    ctype = g_content_type_guess(filename,
                                 NULL, 0, NULL);
    if (!ctype)
        goto cleanup;

    appInfoList = g_app_info_get_all_for_type(ctype);

    appList = GTK_MENU(gtk_menu_new());

    if (appInfoList) {
        GList *tmp = appInfoList;
        while (tmp) {
            GAppInfo *appInfo = tmp->data;

            child = GTK_MENU_ITEM(gtk_menu_item_new_with_label
                                  (g_app_info_get_display_name(appInfo)));
            g_signal_connect(child, "activate",
                             G_CALLBACK(do_session_browser_open_with_app), manager);
            g_object_set_data_full(G_OBJECT(child), "appinfo",
                                   appInfo, (GDestroyNotify)g_object_unref);
            gtk_container_add(GTK_CONTAINER(appList), GTK_WIDGET(child));

            tmp = tmp->next;
        }
        g_list_free(appInfoList);

        child = GTK_MENU_ITEM(gtk_separator_menu_item_new());
        gtk_container_add(GTK_CONTAINER(appList), GTK_WIDGET(child));
    }

    child = GTK_MENU_ITEM(gtk_menu_item_new_with_label(_("Select application...")));
    g_signal_connect(child, "activate",
                     G_CALLBACK(do_session_browser_open_with_select), manager);
    gtk_container_add(GTK_CONTAINER(appList), GTK_WIDGET(child));

    gtk_widget_show_all(GTK_WIDGET(appList));

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(openWith), GTK_WIDGET(appList));

    gtk_menu_popup(priv->sessionBrowserMenu,
                   NULL, NULL, NULL, NULL,
                   event->button, event->time);

 cleanup:
    gtk_widget_set_sensitive(GTK_WIDGET(open), ctype && appInfoList ? TRUE : FALSE);
    return TRUE;
}


static void do_popup_close(EntangleImagePopup *popup,
                           EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE_POPUP(popup));
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleImage *img = entangle_image_popup_get_image(popup);
    const char *filename = entangle_image_get_filename(img);

    g_hash_table_remove(priv->popups, filename);

    entangle_pixbuf_loader_unload(ENTANGLE_PIXBUF_LOADER(priv->imageLoader),
                                  img);
}


static void do_session_browser_drag_failed(GtkWidget *widget G_GNUC_UNUSED,
                                           GdkDragContext *ctx G_GNUC_UNUSED,
                                           GtkDragResult res,
                                           gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (res == GTK_DRAG_RESULT_NO_TARGET) {
        EntangleImage *img = entangle_session_browser_selected_image(priv->sessionBrowser);
        if (img) {
            GdkScreen *screen;
            GdkDevice *dev;
            GdkDeviceManager *devmgr;
            int x, y;

            devmgr = gdk_display_get_device_manager(gtk_widget_get_display(widget));
            dev = gdk_device_manager_get_client_pointer(devmgr);
            gdk_device_get_position(dev,
                                    &screen,
                                    &x, &y);

            const gchar *filename = entangle_image_get_filename(img);
            EntangleImagePopup *pol;
            EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);
            if (!(pol = g_hash_table_lookup(priv->popups, filename))) {
                gchar *bg = entangle_preferences_img_get_background(prefs);
                pol = entangle_image_popup_new();
                entangle_image_popup_set_image(pol, img);
                entangle_image_popup_set_background(pol, bg);
                g_signal_connect(pol, "hide", G_CALLBACK(do_popup_close), manager);
                g_hash_table_insert(priv->popups, g_strdup(filename), pol);
                entangle_pixbuf_loader_load(ENTANGLE_PIXBUF_LOADER(priv->imageLoader),
                                            img);
                g_free(bg);
            }
            ENTANGLE_DEBUG("Popup %p for %s", pol, filename);
            entangle_image_popup_show(pol, GTK_WINDOW(manager), x, y);
            g_object_unref(img);
        }
    }
}


static void do_pixbuf_loaded(EntanglePixbufLoader *loader,
                             EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE(image));

    GdkPixbuf *pixbuf = entangle_pixbuf_loader_get_pixbuf(loader, image);

    entangle_image_set_pixbuf(image, pixbuf);
}


static void do_metadata_loaded(EntanglePixbufLoader *loader,
                               EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE(image));

    GExiv2Metadata *metadata = entangle_pixbuf_loader_get_metadata(loader, image);

    entangle_image_set_metadata(image, metadata);
}


static void do_pixbuf_unloaded(EntanglePixbufLoader *loader G_GNUC_UNUSED,
                               EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE(image));

    entangle_image_set_pixbuf(image, NULL);
}


static void do_metadata_unloaded(EntanglePixbufLoader *loader G_GNUC_UNUSED,
                                 EntangleImage *image)
{
    g_return_if_fail(ENTANGLE_IS_IMAGE(image));

    entangle_image_set_metadata(image, NULL);
}


static gboolean do_image_status_hide(gpointer data)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data), FALSE);

    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;

    entangle_auto_drawer_set_pinned(ENTANGLE_AUTO_DRAWER(priv->imageDrawer), FALSE);
    priv->imageDrawerTimer = 0;
    g_object_unref(manager);
    return FALSE;
}


static void do_image_status_show(GtkWidget *widget G_GNUC_UNUSED,
                                 GdkEvent *event G_GNUC_UNUSED,
                                 gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(data));

    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!entangle_auto_drawer_get_pinned(ENTANGLE_AUTO_DRAWER(priv->imageDrawer)))
        entangle_auto_drawer_set_pinned(ENTANGLE_AUTO_DRAWER(priv->imageDrawer), TRUE);
    if (priv->imageDrawerTimer)
        g_source_remove(priv->imageDrawerTimer);
    priv->imageDrawerTimer = g_timeout_add_seconds(3,
                                                   do_image_status_hide,
                                                   g_object_ref(manager));
}


static void do_entangle_camera_manager_hide(EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    entangle_camera_manager_set_camera(manager, NULL);

    ENTANGLE_DEBUG("Removing all popups");
    g_hash_table_remove_all(priv->popups);
}


static void do_entangle_camera_manager_set_builder(EntangleWindow *window,
                                                   GtkBuilder *builder)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(window);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *display;
    GtkWidget *iconScroll;
    GtkWidget *settingsBox;
    GtkWidget *settingsViewport;
    GtkWidget *menu;
    GtkWidget *monitorMenu;
    GtkWidget *imageViewport;
    GtkWidget *imageHistogramExpander;

    priv->builder = g_object_ref(builder);

    menu = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-monitor"));
    monitorMenu = entangle_camera_manager_monitor_menu(manager);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), monitorMenu);

    imageViewport = gtk_viewport_new(NULL, NULL);
    priv->imageScroll = GTK_SCROLLED_WINDOW
        (gtk_scrolled_window_new(gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(imageViewport)),
                                 gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(imageViewport))));

    gtk_container_add(GTK_CONTAINER(priv->imageScroll), imageViewport);

    priv->imageLoader = entangle_image_loader_new();
    priv->thumbLoader = entangle_thumbnail_loader_new(140, 140);

    g_signal_connect(priv->imageLoader, "pixbuf-loaded", G_CALLBACK(do_pixbuf_loaded), NULL);
    g_signal_connect(priv->imageLoader, "metadata-loaded", G_CALLBACK(do_metadata_loaded), NULL);
    g_signal_connect(priv->imageLoader, "pixbuf-unloaded", G_CALLBACK(do_pixbuf_unloaded), NULL);
    g_signal_connect(priv->imageLoader, "metadata-unloaded", G_CALLBACK(do_metadata_unloaded), NULL);

    priv->imageDisplay = entangle_image_display_new();
    priv->imageStatusbar = entangle_image_statusbar_new();
    priv->imageDrawer = entangle_auto_drawer_new();
    priv->sessionBrowser = entangle_session_browser_new();
    priv->sessionBrowserMenu = GTK_MENU(gtk_builder_get_object(priv->builder, "menu-session-browser"));
    priv->controlPanel = entangle_control_panel_new(priv->cameraPrefs);
    priv->imageHistogram = entangle_image_histogram_new();
    gtk_widget_show(GTK_WIDGET(priv->imageHistogram));
    priv->scriptConfig = entangle_script_config_new();
    gtk_widget_show(GTK_WIDGET(priv->scriptConfig));

    g_object_set(priv->sessionBrowser, "thumbnail-loader", priv->thumbLoader, NULL);

    g_signal_connect(priv->imageDisplay, "size-allocate",
                     G_CALLBACK(do_restore_scroll), manager);

    g_signal_connect(priv->sessionBrowser, "selection-changed",
                     G_CALLBACK(do_session_image_selected), manager);
    g_signal_connect(priv->sessionBrowser, "button-press-event",
                     G_CALLBACK(do_session_browser_popup), manager);
    g_signal_connect(priv->sessionBrowser, "drag-failed",
                     G_CALLBACK(do_session_browser_drag_failed), manager);

    settingsViewport = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-viewport"));
    settingsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    imageHistogramExpander = gtk_expander_new(_("Image histogram"));
    gtk_expander_set_expanded(GTK_EXPANDER(imageHistogramExpander), TRUE);
    priv->scriptConfigExpander = gtk_expander_new(_("Automation"));
    gtk_expander_set_expanded(GTK_EXPANDER(priv->scriptConfigExpander), TRUE);
    display = GTK_WIDGET(gtk_builder_get_object(priv->builder, "display-panel"));

    iconScroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(iconScroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_NEVER);

    gtk_widget_set_size_request(settingsBox, 300, 100);
    gtk_widget_set_size_request(iconScroll, 140, 170);

    priv->popups = g_hash_table_new_full(g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_object_unref);

    gtk_container_add(GTK_CONTAINER(imageViewport), GTK_WIDGET(priv->imageDisplay));

    entangle_overlay_box_set_over(ENTANGLE_OVERLAY_BOX(priv->imageDrawer), GTK_WIDGET(priv->imageStatusbar));
    entangle_overlay_box_set_under(ENTANGLE_OVERLAY_BOX(priv->imageDrawer), GTK_WIDGET(priv->imageScroll));
    entangle_auto_drawer_set_offset(priv->imageDrawer, -1);
    entangle_auto_drawer_set_fill(priv->imageDrawer, TRUE);
    entangle_auto_drawer_set_overlap_pixels(priv->imageDrawer, 1);
    entangle_auto_drawer_set_no_overlap_pixels(priv->imageDrawer, 0);
    entangle_drawer_set_speed(ENTANGLE_DRAWER(priv->imageDrawer), 20, 0.05);
    gtk_widget_show(GTK_WIDGET(priv->imageDrawer));
    gtk_widget_show(GTK_WIDGET(priv->imageStatusbar));
    entangle_auto_drawer_set_active(priv->imageDrawer, TRUE);

    gtk_widget_realize(GTK_WIDGET(manager));
    GdkWindow *imgWin = gtk_widget_get_window(GTK_WIDGET(manager));
    gdk_window_set_events(imgWin,
                          gdk_window_get_events(imgWin) | GDK_POINTER_MOTION_MASK);
    g_signal_connect(manager, "motion-notify-event",
                     G_CALLBACK(do_image_status_show), manager);

    ENTANGLE_DEBUG("Adding %p to %p", priv->imageDisplay, imageViewport);
    gtk_paned_pack1(GTK_PANED(display), GTK_WIDGET(priv->imageDrawer), TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(display), GTK_WIDGET(iconScroll), FALSE, TRUE);
    gtk_widget_show_all(display);

    gtk_container_add(GTK_CONTAINER(iconScroll), GTK_WIDGET(priv->sessionBrowser));
    gtk_container_add(GTK_CONTAINER(settingsViewport), settingsBox);
    gtk_box_pack_start(GTK_BOX(settingsBox), GTK_WIDGET(priv->controlPanel), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(settingsBox), GTK_WIDGET(priv->scriptConfigExpander), FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(priv->scriptConfigExpander), GTK_WIDGET(priv->scriptConfig));
    gtk_box_pack_start(GTK_BOX(settingsBox), GTK_WIDGET(imageHistogramExpander), FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(imageHistogramExpander), GTK_WIDGET(priv->imageHistogram));
    gtk_widget_show(settingsViewport);
    gtk_widget_show(settingsBox);
    gtk_widget_show(imageHistogramExpander);

#if GTK_CHECK_VERSION(3, 12, 0)
    gtk_widget_set_margin_end(priv->scriptConfigExpander, 6);
    gtk_widget_set_margin_end(imageHistogramExpander, 6);
#else
    gtk_widget_set_margin_right(priv->scriptConfigExpander, 6);
    gtk_widget_set_margin_right(imageHistogramExpander, 6);
#endif

    priv->monitorCancel = g_cancellable_new();
    priv->taskCancel = g_cancellable_new();
    priv->taskConfirm = g_cancellable_new();

    do_zoom_widget_sensitivity(manager);
    do_capture_widget_sensitivity(manager);

    gtk_widget_show(GTK_WIDGET(manager));
    gtk_widget_show(GTK_WIDGET(priv->controlPanel));
    gtk_widget_show(GTK_WIDGET(priv->imageDisplay));
    gtk_widget_show(GTK_WIDGET(priv->sessionBrowser));

    g_signal_connect(manager, "hide",
                     G_CALLBACK(do_entangle_camera_manager_hide), NULL);
    g_signal_connect(manager, "delete-event",
                     G_CALLBACK(do_manager_delete), manager);
}


static GtkBuilder *do_entangle_camera_manager_get_builder(EntangleWindow *window)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(window);
    EntangleCameraManagerPrivate *priv = manager->priv;

    return priv->builder;
}


static void do_camera_capture_begin(EntangleCameraAutomata *automata,
                                    EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata));
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);

    if (entangle_preferences_interface_get_screen_blank(prefs))
        entangle_dpms_set_blanking(TRUE, NULL);
}


static void do_camera_capture_end(EntangleCameraAutomata *automata,
                                    EntangleCameraManager *manager)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_AUTOMATA(automata));
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntanglePreferences *prefs = entangle_camera_manager_get_preferences(manager);

    if (entangle_preferences_interface_get_screen_blank(prefs))
        entangle_dpms_set_blanking(FALSE, NULL);
}


static void entangle_camera_manager_init(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv;
    priv = manager->priv = ENTANGLE_CAMERA_MANAGER_GET_PRIVATE(manager);

    priv->imageScrollHOffset = 0;
    priv->imageScrollVOffset = 0;

    priv->automata = entangle_camera_automata_new();
    priv->cameraPrefs = entangle_camera_preferences_new();

    g_signal_connect(priv->automata, "camera-capture-begin",
                     G_CALLBACK(do_camera_capture_begin), manager);
    g_signal_connect(priv->automata, "camera-capture-end",
                     G_CALLBACK(do_camera_capture_end), manager);

    g_signal_connect(manager,
                     "notify::application",
                     G_CALLBACK(do_entangle_camera_manager_set_app),
                     NULL);
}


EntangleCameraManager *entangle_camera_manager_new(void)
{
    return ENTANGLE_CAMERA_MANAGER(entangle_window_new(ENTANGLE_TYPE_CAMERA_MANAGER,
                                                       GTK_TYPE_WINDOW,
                                                       "camera-manager"));
}


void entangle_camera_manager_add_script(EntangleCameraManager *manager,
                                        EntangleScript *script)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));
    g_return_if_fail(ENTANGLE_IS_SCRIPT(script));

    EntangleCameraManagerPrivate *priv = manager->priv;

    entangle_script_config_add_script(priv->scriptConfig,
                                      script);
    gtk_widget_show(priv->scriptConfigExpander);
}


void entangle_camera_manager_remove_script(EntangleCameraManager *manager,
                                           EntangleScript *script)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));
    g_return_if_fail(ENTANGLE_IS_SCRIPT(script));

    EntangleCameraManagerPrivate *priv = manager->priv;

    entangle_script_config_remove_script(priv->scriptConfig,
                                         script);

    if (!entangle_script_config_has_scripts(priv->scriptConfig))
        gtk_widget_hide(priv->scriptConfigExpander);
}


static void do_camera_disconnect_finish(GObject *source,
                                        GAsyncResult *result,
                                        gpointer opaque G_GNUC_UNUSED)
{
    EntangleCamera *cam = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (entangle_camera_disconnect_finish(cam, result, &error)) {
        ENTANGLE_DEBUG("Unable to disconnect from camera");
    }
}


void entangle_camera_manager_set_camera(EntangleCameraManager *manager,
                                        EntangleCamera *cam)
{
    g_return_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager));

    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->camera) {
        do_remove_camera(manager);
        entangle_camera_disconnect_async(priv->camera,
                                         NULL,
                                         do_camera_disconnect_finish,
                                         manager);
        g_object_unref(priv->camera);
    }
    priv->camera = cam;
    priv->cameraReady = FALSE;
    if (priv->camera) {
        g_object_ref(priv->camera);
        do_add_camera(manager);
    }

    do_capture_widget_sensitivity(manager);
}


/**
 * entangle_camera_manager_get_camera:
 * @manager: the camera manager window
 *
 * Get the camera currently being used
 *
 * Returns: (transfer none): the camera or NULL
 */
EntangleCamera *entangle_camera_manager_get_camera(EntangleCameraManager *manager)
{
    g_return_val_if_fail(ENTANGLE_IS_CAMERA_MANAGER(manager), NULL);

    EntangleCameraManagerPrivate *priv = manager->priv;

    return priv->camera;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
