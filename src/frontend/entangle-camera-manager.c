/*
 *  Entangle: Entangle Assists Photograph Aquisition
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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
#include <unistd.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

#include "entangle-debug.h"
#include "entangle-camera-manager.h"
#include "entangle-camera-list.h"
#include "entangle-camera-info.h"
#include "entangle-camera-picker.h"
#include "entangle-session.h"
#include "entangle-image-display.h"
#include "entangle-image-statusbar.h"
#include "entangle-image-loader.h"
#include "entangle-thumbnail-loader.h"
#include "entangle-image-popup.h"
#include "entangle-help-about.h"
#include "entangle-session-browser.h"
#include "entangle-control-panel.h"
#include "entangle-colour-profile.h"
#include "entangle-preferences-display.h"
#include "entangle-progress.h"
#include "view/autoDrawer.h"

#define ENTANGLE_CAMERA_MANAGER_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_MANAGER, EntangleCameraManagerPrivate))

struct _EntangleCameraManagerPrivate {
    EntangleCamera *camera;
    gboolean cameraReady;
    gboolean cameraChanged;
    EntangleSession *session;

    EntangleContext *context;

    EntangleCameraPicker *picker;
    EntangleHelpAbout *about;

    EntangleCameraInfo *summary;
    EntangleCameraInfo *manual;
    EntangleCameraInfo *driver;
    EntangleCameraInfo *supported;

    EntangleImageLoader *imageLoader;
    EntangleThumbnailLoader *thumbLoader;
    EntangleColourProfileTransform *colourTransform;
    EntangleImageDisplay *imageDisplay;
    EntangleImageStatusbar *imageStatusbar;
    ViewAutoDrawer *imageDrawer;
    gulong imageDrawerTimer;
    EntangleSessionBrowser *sessionBrowser;
    EntangleControlPanel *controlPanel;
    EntanglePreferencesDisplay *prefsDisplay;

    EntangleImage *currentImage;

    EntangleImagePopup *imagePresentation;
    gint presentationMonitor;
    GHashTable *popups;

    int zoomLevel;

    gulong sigFileDownload;
    gulong sigFilePreview;
    gulong sigFileAdd;
    gulong sigChanged;
    gulong sigPrefsNotify;

    GCancellable *monitorCancel;
    GCancellable *taskCancel;
    GCancellable *taskConfirm;
    gboolean taskCapture;
    gboolean taskPreview;
    gboolean taskActive;
    gboolean taskProcessEvents;
    float taskTarget;

    GtkBuilder *builder;
};


typedef struct _EntangleCameraFileTaskData EntangleCameraFileTaskData;
struct _EntangleCameraFileTaskData {
    EntangleCameraManager *manager;
    EntangleCameraFile *file;
};


static void entangle_camera_progress_interface_init (gpointer g_iface,
                                                 gpointer iface_data);

//G_DEFINE_TYPE(EntangleCameraManager, entangle_camera_manager, G_TYPE_OBJECT);
G_DEFINE_TYPE_EXTENDED(EntangleCameraManager, entangle_camera_manager, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_PROGRESS, entangle_camera_progress_interface_init));

enum {
    PROP_O,
    PROP_CAMERA,
    PROP_CONTEXT,
};


void do_menu_help_summary(GtkMenuItem *src,
                          EntangleCameraManager *manager);
void do_menu_help_manual(GtkMenuItem *src,
                         EntangleCameraManager *manager);
void do_menu_help_driver(GtkMenuItem *src,
                         EntangleCameraManager *manager);
void do_menu_help_supported(GtkMenuItem *src,
                            EntangleCameraManager *manager);
void do_menu_new_session(GtkImageMenuItem *src,
                         EntangleCameraManager *manager);
void do_menu_open_session(GtkImageMenuItem *src,
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
void do_toolbar_new_session(GtkToolButton *src,
                            EntangleCameraManager *manager);
void do_toolbar_open_session(GtkToolButton *src,
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
void do_menu_quit(GtkMenuItem *src,
                  EntangleCameraManager *manager);
void do_menu_help_about(GtkMenuItem *src,
                        EntangleCameraManager *manager);
void do_menu_connect(GtkMenuItem *src,
                     EntangleCameraManager *manager);
void do_menu_disconnect(GtkMenuItem *src,
                        EntangleCameraManager *manager);


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
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleColourProfileTransform *transform = NULL;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);

    if (entangle_preferences_cms_get_enabled(prefs)) {
        EntangleColourProfile *rgbProfile;
        EntangleColourProfile *monitorProfile;
        EntangleColourProfileIntent intent;

        rgbProfile = entangle_preferences_cms_get_rgb_profile(prefs);
        intent = entangle_preferences_cms_get_rendering_intent(prefs);
        if (entangle_preferences_cms_get_detect_system_profile(prefs)) {
            GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
            monitorProfile = entangle_camera_manager_monitor_profile(GTK_WINDOW(window));
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


static void do_presentation_monitor_toggled(GtkCheckMenuItem *menu, gpointer opaque)
{
    EntangleCameraManager *manager = opaque;
    EntangleCameraManagerPrivate *priv = manager->priv;
    gpointer data = g_object_get_data(G_OBJECT(menu), "monitor");

    priv->presentationMonitor = GPOINTER_TO_INT(data);

    ENTANGLE_DEBUG("Set monitor %d", priv->presentationMonitor);

    if (priv->imagePresentation)
        entangle_image_popup_move_to_monitor(priv->imagePresentation,
                                             priv->presentationMonitor);
}


static GtkWidget *entangle_camera_manager_monitor_menu(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(win));
    GtkWidget *menu = gtk_menu_new();
    GSList *group = NULL;
#ifdef gdk_screen_get_primary_monitor
    int active = gdk_screen_get_primary_monitor(screen);
#else
    int active = 0;
#endif

    for (int i = 0 ; i < gdk_screen_get_n_monitors(screen) ; i++) {
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


static void entangle_camera_manager_prefs_changed(GObject *object G_GNUC_UNUSED,
                                                  GParamSpec *spec,
                                                  gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);

    if (strcmp(spec->name, "cms-enabled") == 0 ||
        strcmp(spec->name, "cms-rgb-profile") == 0 ||
        strcmp(spec->name, "cms-monitor-profile") == 0 ||
        strcmp(spec->name, "cms-detect-system-profile") == 0 ||
        strcmp(spec->name, "cms-rendering-intent") == 0) {
        entangle_camera_manager_update_colour_transform(manager);
    }
}


static void do_capture_widget_sensitivity(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolCapture;
    GtkWidget *toolPreview;
    GtkWidget *settingsBox;

    GtkWidget *toolNew;
    GtkWidget *toolOpen;
    GtkWidget *toolSettings;
    GtkWidget *menuNew;
    GtkWidget *menuOpen;
    GtkWidget *menuConnect;
    GtkWidget *menuDisconnect;
    GtkWidget *menuHelp;
    GtkWidget *menuSettings;

    GtkWidget *cancel;
    gboolean hasControls;

    hasControls = entangle_control_panel_get_has_controls(priv->controlPanel);

    toolCapture = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-capture"));
    toolPreview = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-preview"));
    settingsBox = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-box"));

    toolNew = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-new"));
    toolOpen = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-open"));
    toolSettings = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-settings"));
    menuNew = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-new"));
    menuOpen = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-open"));
    menuConnect = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-connect"));
    menuDisconnect = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-disconnect"));
    menuHelp = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-help-camera"));
    menuSettings = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-settings"));

    cancel = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-cancel"));


    gtk_widget_set_sensitive(toolCapture,
                             priv->cameraReady && !priv->taskCapture && priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) ? TRUE : FALSE);
    gtk_widget_set_sensitive(toolPreview,
                             priv->cameraReady && !priv->taskCapture && priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) &&
                             entangle_camera_get_has_preview(priv->camera) &&
                             !priv->taskCapture ? TRUE : FALSE);

    gtk_widget_set_sensitive(toolNew,
                             !priv->taskActive);
    gtk_widget_set_sensitive(toolOpen,
                             !priv->taskActive);
    gtk_widget_set_sensitive(menuNew,
                             !priv->taskActive);
    gtk_widget_set_sensitive(menuOpen,
                             !priv->taskActive);
    gtk_widget_set_sensitive(menuConnect,
                             priv->camera ? FALSE : TRUE);
    gtk_widget_set_sensitive(menuDisconnect,
                             priv->camera && priv->cameraReady ? TRUE : FALSE);
    gtk_widget_set_sensitive(menuHelp,
                             priv->camera ? TRUE : FALSE);

    if (priv->camera) {
        if (!entangle_camera_get_has_capture(priv->camera))
            gtk_widget_set_tooltip_text(toolCapture, _("This camera does not support image capture"));
        else
            gtk_widget_set_tooltip_text(toolCapture, "");
        if (!entangle_camera_get_has_capture(priv->camera) ||
            !entangle_camera_get_has_preview(priv->camera))
            gtk_widget_set_tooltip_text(toolPreview, _("This camera does not support image preview"));
        else
            gtk_widget_set_tooltip_text(toolPreview, "");
    }

    if (priv->camera &&
        hasControls) {
        gtk_widget_show(settingsBox);
        gtk_widget_set_sensitive(settingsBox, priv->cameraReady);
        gtk_widget_set_sensitive(toolSettings, TRUE);
        gtk_widget_set_sensitive(menuSettings, TRUE);
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolSettings), TRUE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuSettings), TRUE);
    } else {
        gtk_widget_hide(settingsBox);
        gtk_widget_set_sensitive(toolSettings, FALSE);
        gtk_widget_set_sensitive(menuSettings, FALSE);
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolSettings), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuSettings), FALSE);
    }

    if (priv->taskCapture) {
        gtk_widget_show(cancel);
    } else {
        gtk_widget_hide(cancel);
    }
}


static void do_select_image(EntangleCameraManager *manager,
                            EntangleImage *image)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    ENTANGLE_DEBUG("Select image %p", image);
    if (priv->currentImage && entangle_image_get_filename(priv->currentImage)) {
        entangle_pixbuf_loader_unload(ENTANGLE_PIXBUF_LOADER(priv->imageLoader),
                                      priv->currentImage);
        g_object_unref(priv->currentImage);
    }

    priv->currentImage = image;

    if (priv->currentImage && entangle_image_get_filename(priv->currentImage)) {
        g_object_ref(priv->currentImage);
        entangle_pixbuf_loader_load(ENTANGLE_PIXBUF_LOADER(priv->imageLoader),
                                    priv->currentImage);
    }

    entangle_image_display_set_image(priv->imageDisplay, priv->currentImage);
    entangle_image_statusbar_set_image(priv->imageStatusbar, priv->currentImage);
    if (priv->imagePresentation)
        entangle_image_popup_set_image(priv->imagePresentation, priv->currentImage);
}


static void do_camera_task_error(EntangleCameraManager *manager G_GNUC_UNUSED,
                                 const char *label, GError *error)
{
    gdk_threads_enter();
    GtkWidget *msg = gtk_message_dialog_new(NULL,
                                            0,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Operation: %s"), label);
    gtk_window_set_title(GTK_WINDOW(msg),
                         _("Entangle: Operation failed"));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg),
                                             "%s",
                                             error->message);
    g_signal_connect_swapped(msg,
                             "response",
                             G_CALLBACK (gtk_widget_destroy),
                             msg);
    gtk_widget_show_all(msg);
    gdk_threads_leave();
}


static void do_camera_process_events(EntangleCameraManager *manager);


static void do_camera_load_controls_refresh_finish(GObject *source,
                                                   GAsyncResult *result,
                                                   gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (!entangle_camera_load_controls_finish(camera, result, &error)) {
        gdk_threads_enter();
        GtkWidget *msg = gtk_message_dialog_new(NULL,
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
                                 G_CALLBACK (gtk_widget_destroy),
                                 msg);
        gtk_widget_show_all(msg);
        gdk_threads_leave();

        g_error_free(error);
    }

    g_cancellable_reset(priv->monitorCancel);

    do_camera_process_events(manager);
}


static void do_camera_process_events_finish(GObject *src,
                                            GAsyncResult *result,
                                            gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
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

    gdk_threads_enter();
    if (!priv->cameraReady) {
        priv->cameraReady = TRUE;
        do_capture_widget_sensitivity(manager);
    }
    gdk_threads_leave();

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
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->camera)
        return;

    if (priv->taskProcessEvents)
        return;

    entangle_camera_process_events_async(priv->camera, 500, priv->monitorCancel,
                                         do_camera_process_events_finish, manager);

    priv->taskProcessEvents = TRUE;
}

static void do_camera_preview_image_finish(GObject *src,
                                           GAsyncResult *res,
                                           gpointer opaque);


static EntangleCameraFileTaskData *do_camera_task_begin(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCameraFileTaskData *data;

    if (priv->taskActive)
        return NULL;

    data = g_new0(EntangleCameraFileTaskData, 1);
    data->manager = g_object_ref(manager);

    g_cancellable_cancel(priv->monitorCancel);
    g_cancellable_reset(priv->taskConfirm);
    g_cancellable_reset(priv->taskCancel);
    priv->taskActive = TRUE;

    return data;
}


static void do_camera_task_complete(EntangleCameraFileTaskData *data)
{
    EntangleCameraManagerPrivate *priv = data->manager->priv;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    GtkWidget *preview;

    if (data->file) {
        g_object_unref(data->file);
        data->file = NULL;
    }

    if (priv->taskPreview &&
        priv->camera &&
        !g_cancellable_is_cancelled(priv->taskCancel) &&
        entangle_preferences_capture_get_continuous_preview(prefs)) {
        entangle_camera_preview_image_async(priv->camera,
                                            priv->taskCancel,
                                            do_camera_preview_image_finish,
                                            data);
    } else {
        gdk_threads_enter();

        preview = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-preview"));
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(preview), FALSE);

        priv->taskActive = priv->taskPreview = priv->taskCapture = FALSE;

        do_capture_widget_sensitivity(data->manager);
        gdk_threads_leave();

        g_cancellable_reset(priv->taskConfirm);
        g_cancellable_reset(priv->taskCancel);
        g_cancellable_reset(priv->monitorCancel);

        do_camera_process_events(data->manager);
        g_object_unref(data->manager);
        g_free(data);
    }
}


static void do_camera_file_download(EntangleCamera *cam G_GNUC_UNUSED, EntangleCameraFile *file, void *data)
{
    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleImage *image;
    gchar *localpath;

    ENTANGLE_DEBUG("File download %p %p %p", cam, file, data);

    localpath = entangle_session_next_filename(priv->session, file);

    if (!entangle_camera_file_save_path(file, localpath, NULL)) {
        ENTANGLE_DEBUG("Failed save path");
        goto cleanup;
    }
    ENTANGLE_DEBUG("Saved to %s", localpath);
    image = entangle_image_new_file(localpath);

    gdk_threads_enter();
    entangle_session_add(priv->session, image);
    do_select_image(manager, image);
    gdk_threads_leave();

    g_object_unref(image);

 cleanup:
    g_free(localpath);
}

static void do_camera_delete_file_finish(GObject *src,
                                         GAsyncResult *res,
                                         gpointer opaque)
{
    EntangleCameraFileTaskData *data = opaque;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    if (!entangle_camera_delete_file_finish(camera, res, &error)) {
        do_camera_task_error(data->manager, _("Delete"), error);
        g_error_free(error);
        /* Fallthrough to unref */
    }

    do_camera_task_complete(data);
}


static void do_camera_download_file_finish(GObject *src,
                                           GAsyncResult *res,
                                           gpointer opaque)
{
    EntangleCameraFileTaskData *data = opaque;
    EntangleCameraManagerPrivate *priv = data->manager->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    GError *error = NULL;

    if (!entangle_camera_download_file_finish(camera, res, &error)) {
        do_camera_task_error(data->manager, _("Download"), error);
        g_error_free(error);
        /* Fallthrough to delete anyway */
    }

    if (entangle_preferences_capture_get_delete_file(prefs)) {
        entangle_camera_delete_file_async(camera,
                                          data->file,
                                          NULL,
                                          do_camera_delete_file_finish,
                                          data);
    } else {
        do_camera_task_complete(data);
    }
}


static void do_camera_capture_image_finish(GObject *src,
                                           GAsyncResult *res,
                                           gpointer opaque)
{
    EntangleCameraFileTaskData *data = opaque;
    EntangleCameraManagerPrivate *priv = data->manager->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);
    GError *error = NULL;

    if (!(data->file = entangle_camera_capture_image_finish(camera, res, &error))) {
        do_camera_task_error(data->manager, _("Capture"), error);
        do_camera_task_complete(data);
        g_error_free(error);
        return;
    }

    if (g_cancellable_is_cancelled(priv->taskCancel)) {
        if (entangle_preferences_capture_get_delete_file(prefs)) {
            entangle_camera_delete_file_async(camera,
                                              data->file,
                                              NULL,
                                              do_camera_delete_file_finish,
                                              data);
        } else {
            do_camera_task_complete(data);
        }
    } else {
        entangle_camera_download_file_async(camera,
                                            data->file,
                                            NULL,
                                            do_camera_download_file_finish,
                                            data);
    }
}


static void do_camera_capture_image_discard_finish(GObject *src,
                                                   GAsyncResult *res,
                                                   gpointer opaque)
{
    EntangleCameraFileTaskData *data = opaque;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    GError *error = NULL;

    if (!(data->file = entangle_camera_capture_image_finish(camera, res, &error))) {
        do_camera_task_error(data->manager, _("Capture"), error);
        do_camera_task_complete(data);
        g_error_free(error);
        return;
    }

    entangle_camera_delete_file_async(camera,
                                      data->file,
                                      NULL,
                                      do_camera_delete_file_finish,
                                      data);
}


static void do_camera_preview_image_finish(GObject *src,
                                           GAsyncResult *res,
                                           gpointer opaque)
{
    EntangleCameraFileTaskData *data = opaque;
    EntangleCameraManagerPrivate *priv = data->manager->priv;
    EntangleCamera *camera = ENTANGLE_CAMERA(src);
    EntangleCameraFile *file;
    GError *error = NULL;

    if (!(file = entangle_camera_preview_image_finish(camera, res, &error))) {
        if (g_cancellable_is_cancelled(priv->taskCancel) && priv->camera) {
            entangle_camera_capture_image_async(priv->camera,
                                                NULL,
                                                do_camera_capture_image_discard_finish,
                                                data);
        } else {
            priv->taskPreview = FALSE;
            do_camera_task_error(data->manager, _("Preview"), error);
            do_camera_task_complete(data);
        }
        g_error_free(error);
        return;
    }

    g_object_unref(file);

    if (g_cancellable_is_cancelled(priv->taskCancel)) {
        entangle_camera_capture_image_async(priv->camera,
                                            NULL,
                                            do_camera_capture_image_discard_finish,
                                            data);
    } else if (g_cancellable_is_cancelled(priv->taskConfirm)) {
        g_cancellable_reset(priv->taskConfirm);
        entangle_camera_capture_image_async(priv->camera,
                                            priv->taskCancel,
                                            do_camera_capture_image_finish,
                                            data);
    } else {
        entangle_camera_preview_image_async(priv->camera,
                                            priv->taskCancel,
                                            do_camera_preview_image_finish,
                                            data);
    }
}


static void do_camera_file_add(EntangleCamera *camera, EntangleCameraFile *file, void *opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraFileTaskData *data = g_new0(EntangleCameraFileTaskData, 1);

    ENTANGLE_DEBUG("File add %p %p %p", camera, file, data);

    data->manager = g_object_ref(manager);
    data->file = g_object_ref(file);

    entangle_camera_download_file_async(camera,
                                        data->file,
                                        NULL,
                                        do_camera_download_file_finish,
                                        data);
}


static void do_camera_file_preview(EntangleCamera *cam G_GNUC_UNUSED, EntangleCameraFile *file, void *data)
{
    EntangleCameraManager *manager = data;
    GdkPixbuf *pixbuf;
    GByteArray *bytes;
    GInputStream *is;
    EntangleImage *image;

    ENTANGLE_DEBUG("File preview %p %p %p", cam, file, data);

    bytes = entangle_camera_file_get_data(file);
    is = g_memory_input_stream_new_from_data(bytes->data, bytes->len, NULL);

    pixbuf = gdk_pixbuf_new_from_stream(is, NULL, NULL);

    image = entangle_image_new_pixbuf(pixbuf);

    gdk_threads_enter();
    do_select_image(manager, image);
    gdk_threads_leave();

    g_object_unref(pixbuf);
    g_object_unref(is);
}


static void do_entangle_camera_progress_start(EntangleProgress *iface, float target, const char *format, va_list args)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;
    char *txt;
    GtkWidget *operation;

    gdk_threads_enter();

    priv->taskTarget = target;
    mtr = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-progress"));
    operation = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-operation"));

    txt = g_strdup_vprintf(format, args);

    gtk_widget_set_tooltip_text(mtr, txt);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mtr), txt);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

    gtk_widget_show(operation);

    g_free(txt);

    gdk_threads_leave();
}

static void do_entangle_camera_progress_update(EntangleProgress *iface, float current)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;

    gdk_threads_enter();

    mtr = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-progress"));

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), current / priv->taskTarget);

    gdk_threads_leave();
}

static void do_entangle_camera_progress_stop(EntangleProgress *iface)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;
    GtkWidget *operation;

    gdk_threads_enter();

    mtr = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-progress"));
    operation = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-operation"));

    gtk_widget_set_tooltip_text(mtr, "");
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mtr), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

    gtk_widget_hide(operation);

    gdk_threads_leave();
}

static void entangle_camera_progress_interface_init(gpointer g_iface,
                                                    gpointer iface_data G_GNUC_UNUSED)
{
    EntangleProgressInterface *iface = g_iface;
    iface->start = do_entangle_camera_progress_start;
    iface->update = do_entangle_camera_progress_update;
    iface->stop = do_entangle_camera_progress_stop;
}


static void do_remove_camera(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win;
    GtkWidget *mtr;
    GtkWidget *operation;

    g_cancellable_cancel(priv->monitorCancel);
    g_cancellable_cancel(priv->taskCancel);

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
    gtk_window_set_title(GTK_WINDOW(win), _("Camera Manager - Entangle"));

    entangle_control_panel_set_camera(priv->controlPanel, NULL);
    entangle_camera_set_progress(priv->camera, NULL);

    g_signal_handler_disconnect(priv->camera, priv->sigFilePreview);
    g_signal_handler_disconnect(priv->camera, priv->sigFileDownload);
    g_signal_handler_disconnect(priv->camera, priv->sigFileAdd);

    if (priv->imagePresentation) {
        entangle_image_popup_hide(priv->imagePresentation);
        g_object_unref(priv->imagePresentation);
        priv->imagePresentation = NULL;
    }

    mtr = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-progress"));
    operation = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-operation"));
    gtk_widget_set_tooltip_text(mtr, "");
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mtr), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

    gtk_widget_hide(operation);
}


static void do_camera_load_controls_finish(GObject *source,
                                           GAsyncResult *result,
                                           gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraManagerPrivate *priv = manager->priv;

    EntangleCamera *cam = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (entangle_camera_load_controls_finish(cam, result, &error)) {
        gdk_threads_enter();
        do_capture_widget_sensitivity(manager);
        entangle_control_panel_set_camera(priv->controlPanel, priv->camera);
        gdk_threads_leave();
    } else {
        gdk_threads_enter();
        GtkWidget *msg = gtk_message_dialog_new(NULL,
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
                                 G_CALLBACK (gtk_widget_destroy),
                                 msg);
        gtk_widget_show_all(msg);
        gdk_threads_leave();

        g_error_free(error);
    }

    g_cancellable_reset(priv->monitorCancel);
    do_camera_process_events(manager);
}


static void do_camera_connect_finish(GObject *source,
                                     GAsyncResult *result,
                                     gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
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
        gdk_threads_enter();
        GtkWidget *msg = gtk_message_dialog_new(NULL,
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
        gdk_threads_leave();
        g_error_free(error);
    }
}


static gboolean need_camera_unmount(EntangleCamera *cam)
{
    if (entangle_camera_is_mounted(cam)) {
        int response;
        GtkWidget *msg = gtk_message_dialog_new(NULL,
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
                                     gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCamera *cam = ENTANGLE_CAMERA(source);
    GError *error = NULL;

    if (entangle_camera_unmount_finish(cam, result, &error)) {
        entangle_camera_connect_async(priv->camera,
                                      NULL,
                                      do_camera_connect_finish,
                                      manager);
    } else {
        gdk_threads_enter();
        GtkWidget *msg = gtk_message_dialog_new(NULL,
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
                                 G_CALLBACK (gtk_widget_destroy),
                                 msg);
        gtk_widget_show_all(msg);
        entangle_camera_manager_set_camera(manager, NULL);
        gdk_threads_leave();

        g_error_free(error);
    }
}


static void do_camera_control_changed(EntangleCamera *cam G_GNUC_UNUSED,
                                      gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraManagerPrivate *priv = manager->priv;

    gdk_threads_enter();
    priv->cameraChanged = TRUE;
    gdk_threads_leave();
}

static void do_add_camera(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    char *title;
    GtkWidget *win;

    title = g_strdup_printf(_("%s Camera Manager - Entangle"),
                            entangle_camera_get_model(priv->camera));

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);

    priv->sigFilePreview = g_signal_connect(priv->camera, "camera-file-previewed",
                                            G_CALLBACK(do_camera_file_preview), manager);
    priv->sigFileDownload = g_signal_connect(priv->camera, "camera-file-downloaded",
                                             G_CALLBACK(do_camera_file_download), manager);
    priv->sigFileAdd = g_signal_connect(priv->camera, "camera-file-added",
                                        G_CALLBACK(do_camera_file_add), manager);
    priv->sigChanged = g_signal_connect(priv->camera, "camera-controls-changed",
                                        G_CALLBACK(do_camera_control_changed), manager);

    entangle_camera_set_progress(priv->camera, ENTANGLE_PROGRESS(manager));

    if (need_camera_unmount(priv->camera)) {
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

        case PROP_CONTEXT:
            g_value_set_object(value, priv->context);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void do_camera_removed(EntangleCameraList *cameras G_GNUC_UNUSED,
                              EntangleCamera *camera,
                              gpointer opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraManagerPrivate *priv = manager->priv;
    gdk_threads_enter();

    if (priv->camera == camera)
        entangle_camera_manager_set_camera(manager, NULL);

    gdk_threads_leave();
}

static void entangle_camera_manager_set_property(GObject *object,
                                                 guint prop_id,
                                                 const GValue *value,
                                                 GParamSpec *pspec)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(object);
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntanglePreferences *prefs;
    EntangleCameraList *cameras;
    gchar *directory;
    gchar *pattern;

    ENTANGLE_DEBUG("Set prop %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            entangle_camera_manager_set_camera(manager, g_value_get_object(value));
            break;

        case PROP_CONTEXT:
            priv->context = g_value_get_object(value);
            g_object_ref(priv->context);
            prefs = entangle_context_get_preferences(priv->context);
            priv->sigPrefsNotify = g_signal_connect(prefs,
                                                    "notify",
                                                    G_CALLBACK(entangle_camera_manager_prefs_changed),
                                                    manager);
            cameras = entangle_context_get_cameras(priv->context);
            g_signal_connect(cameras, "camera-removed", G_CALLBACK(do_camera_removed), manager);
            directory = entangle_preferences_capture_get_last_session(prefs);
            pattern = entangle_preferences_capture_get_filename_pattern(prefs);
            priv->session = entangle_session_new(directory, pattern);

            entangle_session_load(priv->session);
            entangle_session_browser_set_session(priv->sessionBrowser, priv->session);

            g_free(directory);
            g_free(pattern);

            entangle_camera_manager_update_colour_transform(manager);
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
    if (priv->context)
        g_object_unref(priv->context);
    if (priv->prefsDisplay)
        g_object_unref(priv->prefsDisplay);
    if (priv->picker)
        g_object_unref(priv->picker);

    if (priv->imagePresentation)
        g_object_unref(priv->imagePresentation);

    g_hash_table_destroy(priv->popups);

    g_object_unref(priv->builder);

    G_OBJECT_CLASS (entangle_camera_manager_parent_class)->finalize (object);
}


static void entangle_camera_manager_class_init(EntangleCameraManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

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

    g_type_class_add_private(klass, sizeof(EntangleCameraManagerPrivate));
}

EntangleCameraManager *entangle_camera_manager_new(EntangleContext *context)
{
    return ENTANGLE_CAMERA_MANAGER(g_object_new(ENTANGLE_TYPE_CAMERA_MANAGER,
                                                "context", context,
                                                NULL));
}


GtkWindow *entangle_camera_manager_get_window(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    return GTK_WINDOW(gtk_builder_get_object(priv->builder, "camera-manager"));
}


void do_menu_help_summary(GtkMenuItem *src G_GNUC_UNUSED,
                          EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->summary) {
        priv->summary = entangle_camera_info_new(priv->camera,
                                                 ENTANGLE_CAMERA_INFO_DATA_SUMMARY);
        gtk_window_set_transient_for(entangle_camera_info_get_window(priv->summary),
                                     entangle_camera_manager_get_window(manager));
    }

    entangle_camera_info_show(priv->summary);
}

void do_menu_help_manual(GtkMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->manual) {
        priv->manual = entangle_camera_info_new(priv->camera,
                                                ENTANGLE_CAMERA_INFO_DATA_MANUAL);
        gtk_window_set_transient_for(entangle_camera_info_get_window(priv->manual),
                                     entangle_camera_manager_get_window(manager));
    }

    entangle_camera_info_show(priv->manual);
}

void do_menu_help_driver(GtkMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->driver) {
        priv->driver = entangle_camera_info_new(priv->camera,
                                                ENTANGLE_CAMERA_INFO_DATA_DRIVER);
        gtk_window_set_transient_for(entangle_camera_info_get_window(priv->driver),
                                     entangle_camera_manager_get_window(manager));
    }

    entangle_camera_info_show(priv->driver);
}

void do_menu_help_supported(GtkMenuItem *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->supported) {
        priv->supported = entangle_camera_info_new(priv->camera,
                                                   ENTANGLE_CAMERA_INFO_DATA_SUPPORTED);
        gtk_window_set_transient_for(entangle_camera_info_get_window(priv->supported),
                                     entangle_camera_manager_get_window(manager));
    }

    entangle_camera_info_show(priv->supported);
}


static void entangle_camera_manager_new_session(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *chooser;
    GtkWidget *win;
    gchar *dir;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));

    chooser = gtk_file_chooser_dialog_new(_("Start new session"),
                                          GTK_WINDOW(win),
                                          GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_REJECT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser), TRUE);

    dir = entangle_preferences_capture_get_last_session(prefs);
    g_mkdir_with_parents(dir, 0777);
    ENTANGLE_DEBUG("Set curent folder '%s'", dir);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
    g_free(dir);

    gtk_widget_hide(chooser);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *pattern;
        EntangleSession *session;
        do_select_image(manager, NULL);
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        pattern = entangle_preferences_capture_get_filename_pattern(prefs);
        session = entangle_session_new(dir, pattern);
        entangle_preferences_capture_set_last_session(prefs, dir);
        g_free(pattern);
        g_free(dir);
        entangle_session_load(session);
        if (priv->session)
            g_object_unref(priv->session);
        priv->session = session;
        entangle_session_browser_set_session(priv->sessionBrowser, session);
    }

    gtk_widget_destroy(chooser);
}

static void entangle_camera_manager_open_session(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *chooser;
    GtkWidget *win;
    gchar *dir;
    EntanglePreferences *prefs = entangle_context_get_preferences(priv->context);

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));

    chooser = gtk_file_chooser_dialog_new(_("Open existing session"),
                                          GTK_WINDOW(win),
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_REJECT,
                                          GTK_STOCK_OK,
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
        EntangleSession *session;
        gchar *pattern;
        do_select_image(manager, NULL);
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        pattern = entangle_preferences_capture_get_last_session(prefs);
        session = entangle_session_new(dir, pattern);
        entangle_preferences_capture_set_last_session(prefs, dir);
        g_free(dir);
        g_free(pattern);
        entangle_session_load(session);
        if (priv->session)
            g_object_unref(priv->session);
        priv->session = session;
        entangle_session_browser_set_session(priv->sessionBrowser, session);
    }

    gtk_widget_destroy(chooser);
}


void do_toolbar_new_session(GtkToolButton *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    entangle_camera_manager_new_session(manager);
}

void do_toolbar_open_session(GtkToolButton *src G_GNUC_UNUSED,
                             EntangleCameraManager *manager)
{
    entangle_camera_manager_open_session(manager);
}

void do_menu_new_session(GtkImageMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    entangle_camera_manager_new_session(manager);
}

void do_menu_open_session(GtkImageMenuItem *src G_GNUC_UNUSED,
                          EntangleCameraManager *manager)
{
    entangle_camera_manager_open_session(manager);
}

void do_menu_settings_toggled(GtkCheckMenuItem *src,
                              EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *settings;
    GtkWidget *toolbar;
    gboolean active;

    settings = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-box"));
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
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *settings;
    GtkWidget *menu;
    gboolean active;

    settings = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-box"));
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
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->taskCancel)
        g_cancellable_cancel(priv->taskCancel);
}


void do_toolbar_capture(GtkToolButton *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    ENTANGLE_DEBUG("starting capture operation");

    if (priv->taskPreview) {
        g_cancellable_cancel(priv->taskConfirm);
    } else {
        EntangleCameraFileTaskData *data;
        if (!(data = do_camera_task_begin(manager)))
            return;

        priv->taskCapture = TRUE;
        do_capture_widget_sensitivity(manager);
        entangle_camera_capture_image_async(priv->camera,
                                            priv->taskCancel,
                                            do_camera_capture_image_finish,
                                            data);
    }
}


void do_toolbar_preview(GtkToggleToolButton *src,
                        EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (gtk_toggle_tool_button_get_active(src)) {
        EntangleCameraFileTaskData *data;
        ENTANGLE_DEBUG("starting preview operation");
        if (!(data = do_camera_task_begin(manager)))
            return;

        priv->taskPreview = TRUE;
        do_capture_widget_sensitivity(manager);
        entangle_camera_preview_image_async(priv->camera,
                                            priv->taskCancel,
                                            do_camera_preview_image_finish,
                                            data);
    } else if (priv->taskPreview) {
        ENTANGLE_DEBUG("Cancelling capture operation");
        g_cancellable_cancel(priv->taskCancel);
    }
}

static void do_zoom_widget_sensitivity(EntangleCameraManager *manager)
{
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
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel < 10)
        priv->zoomLevel += 1;

    entangle_camera_manager_update_zoom(manager);
}

static void entangle_camera_manager_zoom_out(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel > -10)
        priv->zoomLevel -= 1;

    entangle_camera_manager_update_zoom(manager);
}

static void entangle_camera_manager_zoom_normal(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    entangle_image_display_set_scale(priv->imageDisplay, 0.0);
    entangle_image_display_set_autoscale(priv->imageDisplay, FALSE);
    do_zoom_widget_sensitivity(manager);
}

static void entangle_camera_manager_zoom_best(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    entangle_image_display_set_autoscale(priv->imageDisplay, TRUE);
    do_zoom_widget_sensitivity(manager);
}

void do_toolbar_zoom_in(GtkToolButton *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_in(manager);
}

void do_toolbar_zoom_out(GtkToolButton *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_out(manager);
}

void do_toolbar_zoom_normal(GtkToolButton *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_normal(manager);
}

void do_toolbar_zoom_best(GtkToolButton *src G_GNUC_UNUSED,
                          EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_best(manager);
}


void do_menu_zoom_in(GtkImageMenuItem *src G_GNUC_UNUSED,
                     EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_in(manager);
}

void do_menu_zoom_out(GtkImageMenuItem *src G_GNUC_UNUSED,
                      EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_out(manager);
}

void do_menu_zoom_normal(GtkImageMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_normal(manager);
}

void do_menu_zoom_best(GtkImageMenuItem *src G_GNUC_UNUSED,
                       EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_best(manager);
}


void do_toolbar_fullscreen(GtkToggleToolButton *src,
                           EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
    GtkWidget *menu = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-fullscreen"));
    GtkWidget *menubar = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menubar"));

    if (gtk_toggle_tool_button_get_active(src)) {
        gtk_widget_hide(menubar);
        gtk_window_fullscreen(GTK_WINDOW(win));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(win));
        gtk_widget_show(menubar);
    }

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) !=
        gtk_toggle_tool_button_get_active(src))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu),
                                       gtk_toggle_tool_button_get_active(src));
}

void do_menu_fullscreen(GtkCheckMenuItem *src,
                        EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
    GtkWidget *tool = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-fullscreen"));
    GtkWidget *menubar = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menubar"));

    if (gtk_check_menu_item_get_active(src)) {
        gtk_widget_hide(menubar);
        gtk_window_fullscreen(GTK_WINDOW(win));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(win));
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
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *menu = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-presentation"));

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), FALSE);
}

void do_menu_presentation(GtkCheckMenuItem *src,
                          EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (gtk_check_menu_item_get_active(src)) {
        if (!priv->imagePresentation) {
            priv->imagePresentation = entangle_image_popup_new();
            g_signal_connect(priv->imagePresentation, "popup-close", G_CALLBACK(do_presentation_end), manager);
        }
        entangle_image_popup_set_image(priv->imagePresentation, priv->currentImage);
        entangle_image_popup_show_on_monitor(priv->imagePresentation,
                                             priv->presentationMonitor);
    } else if (priv->imagePresentation) {
        entangle_image_popup_hide(priv->imagePresentation);
        g_object_unref(priv->imagePresentation);
        priv->imagePresentation = NULL;
    }
}


void do_menu_preferences(GtkCheckMenuItem *src G_GNUC_UNUSED,
                         EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    if (priv->prefsDisplay == NULL) {
        priv->prefsDisplay = entangle_preferences_display_new(priv->context);
        gtk_window_set_transient_for(entangle_preferences_display_get_window(priv->prefsDisplay),
                                     entangle_camera_manager_get_window(manager));
    }

    entangle_preferences_display_show(priv->prefsDisplay);
}


void do_menu_quit(GtkMenuItem *src G_GNUC_UNUSED,
                  EntangleCameraManager *manager G_GNUC_UNUSED)
{
#if GLIB_CHECK_VERSION(2, 32, 0)
    g_application_quit(g_application_get_default());
#else
    gtk_main_quit();
#endif
}


void do_menu_help_about(GtkMenuItem *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->about) {
        priv->about = entangle_help_about_new();
        gtk_window_set_transient_for(entangle_help_about_get_window(priv->about),
                                     entangle_camera_manager_get_window(manager));
    }

    entangle_help_about_show(priv->about);
}


static void do_picker_close(EntangleCameraPicker *picker, EntangleCameraManager *manager G_GNUC_UNUSED)
{
    entangle_camera_picker_hide(picker);
}


static void do_picker_refresh(EntangleCameraPicker *picker G_GNUC_UNUSED, EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    entangle_context_refresh_cameras(priv->context);
}


static void do_picker_connect(EntangleCameraPicker *picker G_GNUC_UNUSED,
                              EntangleCamera *cam,
                              gpointer *opaque)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(opaque);
    EntangleCameraManagerPrivate *priv = manager->priv;
#if 0
    GError *error = NULL;
    ENTANGLE_DEBUG("emit connect %p %s", cam, entangle_camera_get_model(cam));
    if (need_camera_unmount(cam))
        do_camera_unmount(cam);

    while (!entangle_camera_connect(cam, &error)) {
        int response;
        GtkWidget *msg = gtk_message_dialog_new(NULL,
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
    entangle_camera_picker_hide(priv->picker);

}


static void do_camera_connect(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCameraList *cameras;

    cameras = entangle_context_get_cameras(priv->context);

    if (entangle_camera_list_count(cameras) == 1) {
        EntangleCamera *cam = entangle_camera_list_get(cameras, 0);

        entangle_camera_manager_set_camera(manager, cam);
    } else {
        if (!priv->picker) {
            priv->picker = entangle_camera_picker_new(cameras);
            gtk_window_set_transient_for(entangle_camera_picker_get_window(priv->picker),
                                         entangle_camera_manager_get_window(manager));
            g_signal_connect(priv->picker, "picker-close", G_CALLBACK(do_picker_close), manager);
            g_signal_connect(priv->picker, "picker-refresh", G_CALLBACK(do_picker_refresh), manager);
            g_signal_connect(priv->picker, "picker-connect", G_CALLBACK(do_picker_connect), manager);
        }

        entangle_camera_picker_show(priv->picker);
    }
}

void do_menu_connect(GtkMenuItem *src G_GNUC_UNUSED,
                     EntangleCameraManager *manager)
{
    do_camera_connect(manager);
}


void do_menu_disconnect(GtkMenuItem *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager)
{
    entangle_camera_manager_set_camera(manager, NULL);
}


static gboolean do_manager_delete(GtkWidget *widget G_GNUC_UNUSED,
                                  GdkEvent *event G_GNUC_UNUSED,
                                  EntangleCameraManager *manager G_GNUC_UNUSED)
{
    ENTANGLE_DEBUG("Got delete");
#if GLIB_CHECK_VERSION(2, 32, 0)
    g_application_quit(g_application_get_default());
#else
    gtk_main_quit();
#endif
    return TRUE;
}


static void do_session_image_selected(GtkIconView *view G_GNUC_UNUSED,
                                      gpointer data)
{
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


static void do_popup_remove(gpointer data)
{
    EntangleImagePopup *pol = data;

    entangle_image_popup_hide(pol);
    g_object_unref(pol);
}


static void do_drag_failed(GtkWidget *widget G_GNUC_UNUSED,
                           GdkDragContext *ctx G_GNUC_UNUSED,
                           GtkDragResult res,
                           gpointer data)
{
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

            GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
            const gchar *filename = entangle_image_get_filename(img);
            EntangleImagePopup *pol;
            if (!(pol = g_hash_table_lookup(priv->popups, filename))) {
                pol = entangle_image_popup_new();
                entangle_image_popup_set_image(pol, img);
                g_object_unref(img);
                g_hash_table_insert(priv->popups, g_strdup(filename), pol);
            }
            ENTANGLE_DEBUG("Popup %p for %s", pol, filename);
            entangle_image_popup_show(pol, GTK_WINDOW(win), x, y);
        }
    }
}

static void do_pixbuf_loaded(EntanglePixbufLoader *loader,
                             EntangleImage *image)
{
    GdkPixbuf *pixbuf = entangle_pixbuf_loader_get_pixbuf(loader, image);

    gdk_threads_enter();
    entangle_image_set_pixbuf(image, pixbuf);
    gdk_threads_leave();
}

static void do_metadata_loaded(EntanglePixbufLoader *loader,
                               EntangleImage *image)
{
    GExiv2Metadata *metadata = entangle_pixbuf_loader_get_metadata(loader, image);

    gdk_threads_enter();
    entangle_image_set_metadata(image, metadata);
    gdk_threads_leave();
}


static gboolean do_image_status_hide(gpointer opaque)
{
    EntangleCameraManager *manager = opaque;
    EntangleCameraManagerPrivate *priv = manager->priv;

    gdk_threads_enter();
    ViewAutoDrawer_SetPinned(VIEW_AUTODRAWER(priv->imageDrawer), FALSE);
    gdk_threads_leave();
    priv->imageDrawerTimer = 0;
    return FALSE;
}

static void do_image_status_show(GtkWidget *widget G_GNUC_UNUSED,
                                 GdkEvent *event G_GNUC_UNUSED,
                                 gpointer opaque)
{
    EntangleCameraManager *manager = opaque;
    EntangleCameraManagerPrivate *priv = manager->priv;

    ViewAutoDrawer_SetPinned(VIEW_AUTODRAWER(priv->imageDrawer), TRUE);
    if (priv->imageDrawerTimer)
        g_source_remove(priv->imageDrawerTimer);
    priv->imageDrawerTimer = g_timeout_add_seconds(3,
                                                   do_image_status_hide,
                                                   manager);
}


static void entangle_camera_manager_init(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv;
    GtkWidget *viewport;
    GtkWidget *display;
    GtkWidget *imageScroll;
    GtkWidget *iconScroll;
    GtkWidget *settingsBox;
    GtkWidget *settingsViewport;
    GtkWidget *win;
    GtkWidget *menu;
    GtkWidget *monitorMenu;
    GtkWidget *operation;
    GError *error = NULL;
    GtkTargetEntry targets[] = {
        { g_strdup("demo"), GTK_TARGET_SAME_APP, 0,}
    };

    priv = manager->priv = ENTANGLE_CAMERA_MANAGER_GET_PRIVATE(manager);

    priv->builder = gtk_builder_new();

    if (access("./entangle", R_OK) == 0)
        gtk_builder_add_from_file(priv->builder, "frontend/entangle-camera-manager.xml", &error);
    else
        gtk_builder_add_from_file(priv->builder, PKGDATADIR "/entangle-camera-manager.xml", &error);

    if (error)
        g_error(_("Could not load user interface definition file: %s"), error->message);

    gtk_builder_connect_signals(priv->builder, manager);

    win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));
    g_signal_connect(win, "delete-event", G_CALLBACK(do_manager_delete), manager);

    menu = GTK_WIDGET(gtk_builder_get_object(priv->builder, "menu-monitor"));
    monitorMenu = entangle_camera_manager_monitor_menu(manager);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), monitorMenu);

    viewport = GTK_WIDGET(gtk_builder_get_object(priv->builder, "image-viewport"));

    priv->imageLoader = entangle_image_loader_new();
    priv->thumbLoader = entangle_thumbnail_loader_new(96, 96);

    g_signal_connect(priv->imageLoader, "pixbuf-loaded", G_CALLBACK(do_pixbuf_loaded), NULL);
    g_signal_connect(priv->imageLoader, "metadata-loaded", G_CALLBACK(do_metadata_loaded), NULL);

    priv->imageDisplay = entangle_image_display_new();
    priv->imageStatusbar = entangle_image_statusbar_new();
    priv->imageDrawer = VIEW_AUTODRAWER(ViewAutoDrawer_New());
    priv->sessionBrowser = entangle_session_browser_new();
    priv->controlPanel = entangle_control_panel_new();

    g_object_set(priv->sessionBrowser, "thumbnail-loader", priv->thumbLoader, NULL);

    g_signal_connect(priv->sessionBrowser, "selection-changed",
                     G_CALLBACK(do_session_image_selected), manager);

    imageScroll = GTK_WIDGET(gtk_builder_get_object(priv->builder, "image-scroll"));
    iconScroll = GTK_WIDGET(gtk_builder_get_object(priv->builder, "icon-scroll"));
    settingsBox = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-box"));
    settingsViewport = GTK_WIDGET(gtk_builder_get_object(priv->builder, "settings-viewport"));
    display = GTK_WIDGET(gtk_builder_get_object(priv->builder, "display-panel"));

    gtk_icon_view_enable_model_drag_source(GTK_ICON_VIEW(priv->sessionBrowser),
                                           GDK_BUTTON1_MASK,
                                           targets,
                                           1,
                                           GDK_ACTION_PRIVATE);

    g_signal_connect(priv->sessionBrowser, "drag-failed", G_CALLBACK(do_drag_failed), manager);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(iconScroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_NEVER);

    gtk_container_child_set(GTK_CONTAINER(display), imageScroll, "resize", TRUE, NULL);
    gtk_container_child_set(GTK_CONTAINER(display), iconScroll, "resize", FALSE, NULL);

    /* XXX match icon size + padding + scrollbar needs */
    gtk_widget_set_size_request(settingsBox, 300, 100);
    gtk_widget_set_size_request(iconScroll, 140, 170);

    priv->popups = g_hash_table_new_full(g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            do_popup_remove);


    ViewOvBox_SetOver(VIEW_OV_BOX(priv->imageDrawer), GTK_WIDGET(priv->imageStatusbar));
    ViewOvBox_SetUnder(VIEW_OV_BOX(priv->imageDrawer), GTK_WIDGET(priv->imageDisplay));
    ViewAutoDrawer_SetOffset(VIEW_AUTODRAWER(priv->imageDrawer), -1);
    ViewAutoDrawer_SetFill(VIEW_AUTODRAWER(priv->imageDrawer), TRUE);
    ViewAutoDrawer_SetOverlapPixels(VIEW_AUTODRAWER(priv->imageDrawer), 1);
    ViewAutoDrawer_SetNoOverlapPixels(VIEW_AUTODRAWER(priv->imageDrawer), 0);
    ViewDrawer_SetSpeed(VIEW_DRAWER(priv->imageDrawer), 20, 0.05);
    gtk_widget_show(GTK_WIDGET(priv->imageDrawer));
    gtk_widget_show(GTK_WIDGET(priv->imageStatusbar));
    ViewAutoDrawer_SetActive(priv->imageDrawer, TRUE);

    gtk_widget_realize(win);
    GdkWindow *imgWin = gtk_widget_get_window(win);
    gdk_window_set_events(imgWin,
                          gdk_window_get_events(imgWin) | GDK_POINTER_MOTION_MASK);
    g_signal_connect(GTK_WIDGET(win), "motion-notify-event",
                     G_CALLBACK(do_image_status_show), manager);

    ENTANGLE_DEBUG("Adding %p to %p", priv->imageDisplay, viewport);
    gtk_container_add(GTK_CONTAINER(viewport), GTK_WIDGET(priv->imageDrawer));
    gtk_container_add(GTK_CONTAINER(iconScroll), GTK_WIDGET(priv->sessionBrowser));
    gtk_container_add(GTK_CONTAINER(settingsViewport), GTK_WIDGET(priv->controlPanel));

    priv->monitorCancel = g_cancellable_new();
    priv->taskCancel = g_cancellable_new();
    priv->taskConfirm = g_cancellable_new();

    operation = GTK_WIDGET(gtk_builder_get_object(priv->builder, "toolbar-operation"));
    gtk_widget_hide(operation);

    do_zoom_widget_sensitivity(manager);
    do_capture_widget_sensitivity(manager);
}

void entangle_camera_manager_show(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));

    gtk_widget_show(win);
    gtk_widget_show(GTK_WIDGET(priv->controlPanel));
    gtk_widget_show(GTK_WIDGET(priv->imageDisplay));
    gtk_widget_show(GTK_WIDGET(priv->sessionBrowser));
    gtk_window_present(GTK_WINDOW(win));

    do_camera_connect(manager);
}

void entangle_camera_manager_hide(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));

    ENTANGLE_DEBUG("Removing all popups");
    g_hash_table_remove_all(priv->popups);

    gtk_widget_hide(win);
}

gboolean entangle_camera_manager_visible(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = GTK_WIDGET(gtk_builder_get_object(priv->builder, "camera-manager"));

#if GTK_CHECK_VERSION(2,20,0)
    return gtk_widget_get_visible(win);
#else
    return GTK_WIDGET_FLAGS(win) & GTK_VISIBLE;
#endif
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


EntangleCamera *entangle_camera_manager_get_camera(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    return priv->camera;
}


EntangleContext *entangle_camera_manager_get_context(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

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
