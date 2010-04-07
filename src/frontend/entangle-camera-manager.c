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
#include <math.h>
#include <glade/glade.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

#include "entangle-debug.h"
#include "entangle-camera-manager.h"
#include "entangle-camera-scheduler.h"
#include "entangle-camera-list.h"
#include "entangle-camera-info.h"
#include "entangle-session.h"
#include "entangle-image-display.h"
#include "entangle-image-loader.h"
#include "entangle-thumbnail-loader.h"
#include "entangle-image-popup.h"
#include "entangle-help-about.h"
#include "entangle-session-browser.h"
#include "entangle-control-panel.h"
#include "entangle-colour-profile.h"
#include "entangle-preferences-display.h"
#include "entangle-progress.h"
#include "entangle-camera-task-capture.h"
#include "entangle-camera-task-preview.h"
#include "entangle-camera-task-monitor.h"


#define ENTANGLE_CAMERA_MANAGER_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_MANAGER, EntangleCameraManagerPrivate))

struct _EntangleCameraManagerPrivate {
    EntangleCamera *camera;
    EntangleCameraScheduler *scheduler;
    EntangleSession *session;

    EntanglePreferences *prefs;
    EntanglePluginManager *pluginManager;

    EntangleHelpAbout *about;

    EntangleCameraInfo *summary;
    EntangleCameraInfo *manual;
    EntangleCameraInfo *driver;
    EntangleCameraInfo *supported;

    EntangleImageLoader *imageLoader;
    EntangleThumbnailLoader *thumbLoader;
    EntangleColourProfileTransform *colourTransform;
    EntangleImageDisplay *imageDisplay;
    EntangleSessionBrowser *sessionBrowser;
    EntangleControlPanel *controlPanel;
    EntanglePreferencesDisplay *prefsDisplay;

    EntangleImagePopup *imagePresentation;
    gint presentationMonitor;
    GHashTable *popups;

    GtkWidget *menuCapture;
    GtkWidget *menuItemCapture;
    GtkWidget *menuItemPreview;
    GtkWidget *menuItemMonitor;

    int zoomLevel;

    gulong sigFileDownload;
    gulong sigFilePreview;

    gulong sigTaskBegin;
    gulong sigTaskEnd;

    gulong sigPrefsNotify;

    gboolean inOperation;
    gboolean operationCancel;
    float operationTarget;

    GladeXML *glade;
};

static void entangle_camera_progress_interface_init (gpointer g_iface,
                                                 gpointer iface_data);

//G_DEFINE_TYPE(EntangleCameraManager, entangle_camera_manager, G_TYPE_OBJECT);
G_DEFINE_TYPE_EXTENDED(EntangleCameraManager, entangle_camera_manager, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_PROGRESS, entangle_camera_progress_interface_init));

enum {
    PROP_O,
    PROP_CAMERA,
    PROP_PREFERENCES,
    PROP_PLUGIN_MANAGER,
};


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

    if (entangle_preferences_enable_color_management(priv->prefs)) {
        EntangleColourProfile *rgbProfile;
        EntangleColourProfile *monitorProfile;
        EntangleColourProfileIntent intent;

        rgbProfile = entangle_preferences_rgb_profile(priv->prefs);
        intent = entangle_preferences_profile_rendering_intent(priv->prefs);
        if (entangle_preferences_detect_monitor_profile(priv->prefs)) {
            GtkWidget *window = glade_xml_get_widget(priv->glade, "camera-manager");
            monitorProfile = entangle_camera_manager_monitor_profile(GTK_WINDOW(window));
        } else {
            monitorProfile = entangle_preferences_monitor_profile(priv->prefs);
        }

        if (monitorProfile)
            transform = entangle_colour_profile_transform_new(rgbProfile, monitorProfile, intent);
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
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
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

    if (strcmp(spec->name, "colour-managed-display") == 0 ||
        strcmp(spec->name, "rgb-profile") == 0 ||
        strcmp(spec->name, "monitor-profile") == 0 ||
        strcmp(spec->name, "detect-system-profile") == 0 ||
        strcmp(spec->name, "profile-rendering-intent") == 0) {
        entangle_camera_manager_update_colour_transform(manager);
    }
}


static void do_capture_widget_sensitivity(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolCapture;
    GtkWidget *settingsScroll;
    GtkWidget *iconScroll;

    GtkWidget *toolNew;
    GtkWidget *toolOpen;
    GtkWidget *menuNew;
    GtkWidget *menuOpen;
    GtkWidget *menuConnect;
    GtkWidget *menuDisconnect;
    GtkWidget *menuHelp;

    GtkWidget *cancel;
    GtkWidget *operation;
    GtkWidget *confirm;

    toolCapture = glade_xml_get_widget(priv->glade, "toolbar-capture");
    settingsScroll = glade_xml_get_widget(priv->glade, "settings-scroll");
    iconScroll = glade_xml_get_widget(priv->glade, "icon-scroll");

    toolNew = glade_xml_get_widget(priv->glade, "toolbar-new");
    toolOpen = glade_xml_get_widget(priv->glade, "toolbar-open");
    menuNew = glade_xml_get_widget(priv->glade, "menu-new");
    menuOpen = glade_xml_get_widget(priv->glade, "menu-open");
    menuConnect = glade_xml_get_widget(priv->glade, "menu-connect");
    menuDisconnect = glade_xml_get_widget(priv->glade, "menu-disconnect");
    menuHelp = glade_xml_get_widget(priv->glade, "menu-help-camera");

    cancel = glade_xml_get_widget(priv->glade, "toolbar-cancel");
    operation = glade_xml_get_widget(priv->glade, "toolbar-operation");
    confirm = glade_xml_get_widget(priv->glade, "toolbar-confirm");


    gtk_widget_set_sensitive(toolCapture,
                             priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(priv->menuItemCapture,
                             priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(priv->menuItemPreview,
                             priv->camera &&
                             entangle_camera_get_has_preview(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(priv->menuItemMonitor,
                             priv->camera &&
                             entangle_camera_get_has_capture(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);

    gtk_widget_set_sensitive(toolNew,
                             priv->camera && !priv->inOperation ?
                             TRUE : FALSE);
    gtk_widget_set_sensitive(toolOpen,
                             priv->camera && !priv->inOperation ?
                             TRUE : FALSE);
    gtk_widget_set_sensitive(menuNew,
                             priv->camera && !priv->inOperation ?
                             TRUE : FALSE);
    gtk_widget_set_sensitive(menuOpen,
                             priv->camera && !priv->inOperation ?
                             TRUE : FALSE);
    gtk_widget_set_sensitive(menuConnect,
                             priv->camera ? FALSE : TRUE);
    gtk_widget_set_sensitive(menuDisconnect,
                             priv->camera && !priv->inOperation ?
                             TRUE : FALSE);
    gtk_widget_set_sensitive(menuHelp,
                             priv->camera && !priv->inOperation ?
                             TRUE : FALSE);

    if (priv->camera && !entangle_camera_get_has_capture(priv->camera)) {
        gtk_widget_set_tooltip_text(toolCapture, "This camera does not support image capture");
        gtk_widget_set_tooltip_text(priv->menuItemCapture, "This camera does not support image capture");
        /* XXX is this check correct ? unclear if some cameras can support wait-for-downloads
         * mode, but not be able to trigger the shutter for immediate capture */
        gtk_widget_set_tooltip_text(priv->menuItemMonitor, "This camera does not support image capture");
    }
    if (priv->camera && !entangle_camera_get_has_preview(priv->camera))
        gtk_widget_set_tooltip_text(priv->menuItemPreview, "This camera does not support image preview");

    if (priv->camera && entangle_camera_get_has_settings(priv->camera))
        gtk_widget_show(settingsScroll);
    else
        gtk_widget_hide(settingsScroll);

    gtk_widget_set_sensitive(settingsScroll, !priv->inOperation);
    /*gtk_widget_set_sensitive(iconScroll, !priv->inOperation);*/

    if (priv->inOperation) {
        gtk_widget_show(cancel);
        gtk_widget_show(operation);
    } else {
        gtk_widget_hide(cancel);
        gtk_widget_hide(operation);
        gtk_widget_hide(confirm);
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
    image = entangle_image_new(localpath);

    gdk_threads_enter();
    entangle_session_add(priv->session, image);

    entangle_image_display_set_filename(priv->imageDisplay,
                                        entangle_image_get_filename(image));
    if (priv->imagePresentation)
        g_object_set(priv->imagePresentation, "image", image, NULL);
    gdk_threads_leave();

    g_object_unref(image);

 cleanup:
    g_free(localpath);
}


static void do_camera_file_preview(EntangleCamera *cam G_GNUC_UNUSED, EntangleCameraFile *file, void *data)
{
    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;
    GdkPixbuf *pixbuf;
    GByteArray *bytes;
    GInputStream *is;

    ENTANGLE_DEBUG("File preview %p %p %p", cam, file, data);

    bytes = entangle_camera_file_get_data(file);
    is = g_memory_input_stream_new_from_data(bytes->data, bytes->len, NULL);

    pixbuf = gdk_pixbuf_new_from_stream(is, NULL, NULL);

    gdk_threads_enter();
    entangle_image_display_set_filename(priv->imageDisplay, NULL);
    entangle_image_display_set_pixbuf(priv->imageDisplay, pixbuf);
    gdk_threads_leave();

    g_object_unref(pixbuf);
    g_object_unref(is);
}


static void do_camera_task_begin(EntangleCamera *cam G_GNUC_UNUSED, EntangleCameraTask *task G_GNUC_UNUSED, void *data)
{
    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->operationCancel = FALSE;
}

static void do_camera_task_end(EntangleCamera *cam G_GNUC_UNUSED, EntangleCameraTask *task G_GNUC_UNUSED, void *data)
{
    EntangleCameraManager *manager = data;
    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->inOperation = FALSE;
    priv->operationCancel = FALSE;

    gdk_threads_enter();
    do_capture_widget_sensitivity(manager);
    gdk_threads_leave();
}

static void do_entangle_camera_progress_start(EntangleProgress *iface, float target, const char *format, va_list args)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;
    char *txt;

    gdk_threads_enter();

    priv->operationTarget = target;
    mtr = glade_xml_get_widget(priv->glade, "toolbar-progress");

    txt = g_strdup_vprintf(format, args);

    gtk_widget_set_tooltip_text(mtr, txt);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mtr), txt);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

    g_free(txt);

    gdk_threads_leave();
}

static void do_entangle_camera_progress_update(EntangleProgress *iface, float current)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;

    gdk_threads_enter();

    mtr = glade_xml_get_widget(priv->glade, "toolbar-progress");

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), current / priv->operationTarget);

    gdk_threads_leave();
}

static void do_entangle_camera_progress_stop(EntangleProgress *iface)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;

    gdk_threads_enter();

    mtr = glade_xml_get_widget(priv->glade, "toolbar-progress");

    gtk_widget_set_tooltip_text(mtr, "");
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mtr), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

    gdk_threads_leave();
}

static gboolean do_entangle_camera_progress_cancelled(EntangleProgress *iface)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(iface);
    EntangleCameraManagerPrivate *priv = manager->priv;

    ENTANGLE_DEBUG("Cancel queried %d", (int)priv->operationCancel);

    return priv->operationCancel;
}

static void entangle_camera_progress_interface_init (gpointer g_iface,
                                                 gpointer iface_data G_GNUC_UNUSED)
{
    EntangleProgressInterface *iface = g_iface;
    iface->start = do_entangle_camera_progress_start;
    iface->update = do_entangle_camera_progress_update;
    iface->stop = do_entangle_camera_progress_stop;
    iface->cancelled = do_entangle_camera_progress_cancelled;
}

static void do_remove_camera(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    char *title;
    GtkWidget *win;

    title = g_strdup_printf("Camera Manager - Entangle");
    win = glade_xml_get_widget(priv->glade, "camera-manager");
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);

    entangle_session_browser_set_session(priv->sessionBrowser, NULL);
    entangle_control_panel_set_camera(priv->controlPanel, NULL);
    entangle_control_panel_set_camera_scheduler(priv->controlPanel, NULL);
    entangle_camera_set_progress(priv->camera, NULL);

    entangle_image_display_set_filename(priv->imageDisplay, NULL);
    entangle_image_display_set_pixbuf(priv->imageDisplay, NULL);

    g_signal_handler_disconnect(priv->camera, priv->sigFilePreview);
    g_signal_handler_disconnect(priv->camera, priv->sigFileDownload);
    g_signal_handler_disconnect(priv->scheduler, priv->sigTaskBegin);
    g_signal_handler_disconnect(priv->scheduler, priv->sigTaskEnd);

    entangle_camera_scheduler_end(priv->scheduler);
    g_object_unref(priv->scheduler);
    g_object_unref(priv->session);
    priv->scheduler = NULL;
    priv->session = NULL;

    if (priv->imagePresentation) {
        entangle_image_popup_hide(priv->imagePresentation);
        g_object_unref(priv->imagePresentation);
        priv->imagePresentation = NULL;
    }
}

static void do_add_camera(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    char *title;
    GtkWidget *win;
    char *directory;

    priv->scheduler = entangle_camera_scheduler_new(priv->camera);

    title = g_strdup_printf("%s Camera Manager - Entangle",
                            entangle_camera_get_model(priv->camera));

    win = glade_xml_get_widget(priv->glade, "camera-manager");
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);

    priv->sigFilePreview = g_signal_connect(priv->camera, "camera-file-previewed",
                                            G_CALLBACK(do_camera_file_preview), manager);
    priv->sigFileDownload = g_signal_connect(priv->camera, "camera-file-downloaded",
                                             G_CALLBACK(do_camera_file_download), manager);

    priv->sigTaskBegin = g_signal_connect(priv->scheduler, "camera-scheduler-task-begin",
                                          G_CALLBACK(do_camera_task_begin), manager);
    priv->sigTaskEnd = g_signal_connect(priv->scheduler, "camera-scheduler-task-end",
                                        G_CALLBACK(do_camera_task_end), manager);

    directory = g_strdup_printf("%s/%s/Default Session",
                                entangle_preferences_picture_dir(priv->prefs),
                                entangle_camera_get_model(priv->camera));

    priv->session = entangle_session_new(directory,
                                         entangle_preferences_filename_pattern(priv->prefs));
    entangle_session_load(priv->session);

    entangle_camera_set_progress(priv->camera, ENTANGLE_PROGRESS(manager));
    entangle_session_browser_set_session(priv->sessionBrowser, priv->session);
    entangle_control_panel_set_camera(priv->controlPanel, priv->camera);
    entangle_control_panel_set_camera_scheduler(priv->controlPanel, priv->scheduler);

    g_free(directory);

    entangle_camera_scheduler_start(priv->scheduler);
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


static void entangle_camera_manager_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(object);
    EntangleCameraManagerPrivate *priv = manager->priv;

    ENTANGLE_DEBUG("Set prop %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            entangle_camera_manager_set_camera(manager, g_value_get_object(value));
            break;

        case PROP_PREFERENCES:
            if (priv->prefs) {
                g_signal_handler_disconnect(priv->prefs, priv->sigPrefsNotify);
                g_object_unref(priv->prefs);
            }
            priv->prefs = g_value_get_object(value);
            if (priv->prefs) {
                g_object_ref(priv->prefs);
                priv->sigPrefsNotify = g_signal_connect(priv->prefs,
                                                        "notify",
                                                        G_CALLBACK(entangle_camera_manager_prefs_changed),
                                                        manager);
            }

            entangle_camera_manager_update_colour_transform(manager);
            break;

        case PROP_PLUGIN_MANAGER:
            if (priv->pluginManager)
                g_object_unref(priv->pluginManager);
            priv->pluginManager = g_value_get_object(value);
            if (priv->pluginManager)
                g_object_ref(priv->pluginManager);

            if (priv->prefsDisplay)
                g_object_set(priv->prefsDisplay, "plugin-manager", priv->pluginManager, NULL);

            entangle_camera_manager_update_colour_transform(manager);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_camera_manager_finalize (GObject *object)
{
    EntangleCameraManager *manager = ENTANGLE_CAMERA_MANAGER(object);
    EntangleCameraManagerPrivate *priv = manager->priv;

    ENTANGLE_DEBUG("Finalize manager");

    if (priv->imageLoader)
        g_object_unref(priv->imageLoader);
    if (priv->thumbLoader)
        g_object_unref(priv->thumbLoader);
    if (priv->colourTransform)
        g_object_unref(priv->colourTransform);
    if (priv->scheduler)
        g_object_unref(priv->scheduler);
    if (priv->camera)
        g_object_unref(priv->camera);
    if (priv->pluginManager)
        g_object_unref(priv->pluginManager);
    if (priv->prefs)
        g_object_unref(priv->prefs);
    if (priv->prefsDisplay)
        g_object_unref(priv->prefsDisplay);

    if (priv->imagePresentation)
        g_object_unref(priv->imagePresentation);

    g_hash_table_destroy(priv->popups);

    g_object_unref(priv->glade);

    G_OBJECT_CLASS (entangle_camera_manager_parent_class)->finalize (object);
}


static void entangle_camera_manager_class_init(EntangleCameraManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_camera_manager_finalize;
    object_class->get_property = entangle_camera_manager_get_property;
    object_class->set_property = entangle_camera_manager_set_property;

    g_signal_new("manager-connect",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraManagerClass, manager_connect),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_signal_new("manager-disconnect",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraManagerClass, manager_disconnect),
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
                                    PROP_PREFERENCES,
                                    g_param_spec_object("preferences",
                                                        "Preferences",
                                                        "Application preferences",
                                                        ENTANGLE_TYPE_PREFERENCES,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PLUGIN_MANAGER,
                                    g_param_spec_object("plugin-manager",
                                                        "Plugin manager",
                                                        "Plugin manager",
                                                        ENTANGLE_TYPE_PLUGIN_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleCameraManagerPrivate));
}


EntangleCameraManager *entangle_camera_manager_new(EntanglePreferences *prefs,
                                           EntanglePluginManager *pluginManager)
{
    return ENTANGLE_CAMERA_MANAGER(g_object_new(ENTANGLE_TYPE_CAMERA_MANAGER,
                                            "preferences", prefs,
                                            "plugin-manager", pluginManager,
                                            NULL));
}

static void do_manager_help_summary(GtkMenuItem *src G_GNUC_UNUSED,
                                    EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->summary)
        priv->summary = entangle_camera_info_new(priv->camera,
                                             ENTANGLE_CAMERA_INFO_DATA_SUMMARY);

    entangle_camera_info_show(priv->summary);
}

static void do_manager_help_manual(GtkMenuItem *src G_GNUC_UNUSED,
                                   EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->manual)
        priv->manual = entangle_camera_info_new(priv->camera,
                                            ENTANGLE_CAMERA_INFO_DATA_MANUAL);

    entangle_camera_info_show(priv->manual);
}

static void do_manager_help_driver(GtkMenuItem *src G_GNUC_UNUSED,
                                   EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->driver)
        priv->driver = entangle_camera_info_new(priv->camera,
                                            ENTANGLE_CAMERA_INFO_DATA_DRIVER);

    entangle_camera_info_show(priv->driver);
}

static void entangle_camera_manager_new_session(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *chooser;
    GtkWidget *win;
    gchar *dir;

    win = glade_xml_get_widget(priv->glade, "camera-manager");

    chooser = gtk_file_chooser_dialog_new("Start new session",
                                          GTK_WINDOW(win),
                                          GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_REJECT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser), TRUE);

    dir = g_strdup_printf("%s/%s",
                          entangle_preferences_picture_dir(priv->prefs),
                          entangle_camera_get_model(priv->camera));
    g_mkdir_with_parents(dir, 0777);
    ENTANGLE_DEBUG("Set curent folder '%s'", dir);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
    g_free(dir);

    gtk_widget_hide(chooser);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        EntangleSession *session;
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        session = entangle_session_new(dir, entangle_preferences_filename_pattern(priv->prefs));
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

    win = glade_xml_get_widget(priv->glade, "camera-manager");

    chooser = gtk_file_chooser_dialog_new("Open existing session",
                                          GTK_WINDOW(win),
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_REJECT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser), TRUE);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), FALSE);

    dir = g_strdup_printf("%s/%s",
                          entangle_preferences_picture_dir(priv->prefs),
                          entangle_camera_get_model(priv->camera));
    g_mkdir_with_parents(dir, 0777);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
    g_free(dir);

    gtk_widget_hide(chooser);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        EntangleSession *session;
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        session = entangle_session_new(dir, entangle_preferences_filename_pattern(priv->prefs));
        entangle_session_load(session);
        if (priv->session)
            g_object_unref(priv->session);
        priv->session = session;
        entangle_session_browser_set_session(priv->sessionBrowser, session);
    }

    gtk_widget_destroy(chooser);
}

static void do_toolbar_new_session(GtkToolButton *src G_GNUC_UNUSED,
                                   EntangleCameraManager *manager)
{
    entangle_camera_manager_new_session(manager);
}

static void do_toolbar_open_session(GtkToolButton *src G_GNUC_UNUSED,
                                    EntangleCameraManager *manager)
{
    entangle_camera_manager_open_session(manager);
}

static void do_menu_new_session(GtkImageMenuItem *src G_GNUC_UNUSED,
                                EntangleCameraManager *manager)
{
    entangle_camera_manager_new_session(manager);
}

static void do_menu_open_session(GtkImageMenuItem *src G_GNUC_UNUSED,
                                 EntangleCameraManager *manager)
{
    entangle_camera_manager_open_session(manager);
}

static void do_menu_settings_toggled(GtkCheckMenuItem *src,
                                     EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *settings;
    GtkWidget *toolbar;
    gboolean active;

    settings = glade_xml_get_widget(priv->glade, "settings-box");
    toolbar = glade_xml_get_widget(priv->glade, "toolbar-settings");

    active = gtk_check_menu_item_get_active(src);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolbar), active);

    if (active)
        gtk_widget_show(settings);
    else
        gtk_widget_hide(settings);
}


static void do_toolbar_settings_toggled(GtkToggleToolButton *src,
                                        EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *settings;
    GtkWidget *menu;
    gboolean active;

    settings = glade_xml_get_widget(priv->glade, "settings-box");
    menu = glade_xml_get_widget(priv->glade, "menu-settings");

    active = gtk_toggle_tool_button_get_active(src);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), active);

    if (active)
        gtk_widget_show(settings);
    else
        gtk_widget_hide(settings);
}


static void do_toolbar_cancel_clicked(GtkToolButton *src G_GNUC_UNUSED,
                                      EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    priv->operationCancel = TRUE;
}


static void do_toolbar_capture(GtkToolButton *src G_GNUC_UNUSED,
                               EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCameraTask *task;

    ENTANGLE_DEBUG("starting Capture thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = ENTANGLE_CAMERA_TASK(entangle_camera_task_capture_new());
    entangle_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void do_menu_capture(GtkMenuItem *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCameraTask *task;

    ENTANGLE_DEBUG("starting Capture thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = ENTANGLE_CAMERA_TASK(entangle_camera_task_capture_new());
    entangle_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void do_menu_preview(GtkMenuItem *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCameraTask *task;

    ENTANGLE_DEBUG("starting Preview thread");
    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = ENTANGLE_CAMERA_TASK(entangle_camera_task_preview_new());
    entangle_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void do_menu_monitor(GtkMenuItem *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    EntangleCameraTask *task;

    ENTANGLE_DEBUG("starting monitor thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = ENTANGLE_CAMERA_TASK(entangle_camera_task_monitor_new());
    entangle_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void entangle_camera_manager_setup_capture_menu(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolbarMenu;

    toolbarMenu = glade_xml_get_widget(priv->glade, "toolbar-capture");

    priv->menuCapture = gtk_menu_new();
    priv->menuItemCapture = gtk_menu_item_new_with_label("Capture immediately");
    priv->menuItemPreview = gtk_menu_item_new_with_label("Preview capture");
    priv->menuItemMonitor = gtk_menu_item_new_with_label("Watch for captured images");

    gtk_container_add(GTK_CONTAINER(priv->menuCapture), priv->menuItemCapture);
    gtk_container_add(GTK_CONTAINER(priv->menuCapture), priv->menuItemPreview);
    gtk_container_add(GTK_CONTAINER(priv->menuCapture), priv->menuItemMonitor);

    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(toolbarMenu),
                                  priv->menuCapture);

    g_signal_connect(priv->menuItemCapture, "activate",
                     G_CALLBACK(do_menu_capture), manager);
    g_signal_connect(priv->menuItemPreview, "activate",
                     G_CALLBACK(do_menu_preview), manager);
    g_signal_connect(priv->menuItemMonitor, "activate",
                     G_CALLBACK(do_menu_monitor), manager);

    gtk_widget_show_all(priv->menuCapture);
}


static void do_zoom_widget_sensitivity(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolzoomnormal = glade_xml_get_widget(priv->glade, "toolbar-zoom-normal");
    GtkWidget *toolzoombest = glade_xml_get_widget(priv->glade, "toolbar-zoom-best");
    GtkWidget *toolzoomin = glade_xml_get_widget(priv->glade, "toolbar-zoom-in");
    GtkWidget *toolzoomout = glade_xml_get_widget(priv->glade, "toolbar-zoom-out");

    GtkWidget *menuzoomnormal = glade_xml_get_widget(priv->glade, "menu-zoom-normal");
    GtkWidget *menuzoombest = glade_xml_get_widget(priv->glade, "menu-zoom-best");
    GtkWidget *menuzoomin = glade_xml_get_widget(priv->glade, "menu-zoom-in");
    GtkWidget *menuzoomout = glade_xml_get_widget(priv->glade, "menu-zoom-out");
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

static void do_toolbar_zoom_in(GtkToolButton *src G_GNUC_UNUSED,
                               EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_in(manager);
}

static void do_toolbar_zoom_out(GtkToolButton *src G_GNUC_UNUSED,
                                EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_out(manager);
}

static void do_toolbar_zoom_normal(GtkToolButton *src G_GNUC_UNUSED,
                                   EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_normal(manager);
}

static void do_toolbar_zoom_best(GtkToolButton *src G_GNUC_UNUSED,
                                 EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_best(manager);
}


static void do_menu_zoom_in(GtkImageMenuItem *src G_GNUC_UNUSED,
                            EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_in(manager);
}

static void do_menu_zoom_out(GtkImageMenuItem *src G_GNUC_UNUSED,
                             EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_out(manager);
}

static void do_menu_zoom_normal(GtkImageMenuItem *src G_GNUC_UNUSED,
                                EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_normal(manager);
}

static void do_menu_zoom_best(GtkImageMenuItem *src G_GNUC_UNUSED,
                              EntangleCameraManager *manager)
{
    entangle_camera_manager_zoom_best(manager);
}


static void do_toolbar_fullscreen(GtkToggleToolButton *src,
                                  EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
    GtkWidget *menu = glade_xml_get_widget(priv->glade, "menu-fullscreen");
    GtkWidget *menubar = glade_xml_get_widget(priv->glade, "menubar");

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

static void do_menu_fullscreen(GtkCheckMenuItem *src,
                               EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
    GtkWidget *tool = glade_xml_get_widget(priv->glade, "toolbar-fullscreen");
    GtkWidget *menubar = glade_xml_get_widget(priv->glade, "menubar");

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
    GtkWidget *menu = glade_xml_get_widget(priv->glade, "menu-presentation");

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), FALSE);
}

static void do_menu_presentation(GtkCheckMenuItem *src,
                                 EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (gtk_check_menu_item_get_active(src)) {
        if (!priv->imagePresentation) {
            priv->imagePresentation = entangle_image_popup_new();
            g_signal_connect(priv->imagePresentation, "popup-close", G_CALLBACK(do_presentation_end), manager);
            g_object_set(priv->imagePresentation, "image-loader", priv->imageLoader, NULL);
        }
        entangle_image_popup_show_on_monitor(priv->imagePresentation,
                                             priv->presentationMonitor);
    } else if (priv->imagePresentation) {
        entangle_image_popup_hide(priv->imagePresentation);
        g_object_unref(priv->imagePresentation);
        priv->imagePresentation = NULL;
    }
}


static void do_menu_preferences_activate(GtkCheckMenuItem *src G_GNUC_UNUSED,
                                         EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    if (priv->prefsDisplay == NULL)
        priv->prefsDisplay = entangle_preferences_display_new(priv->prefs,
                                                          priv->pluginManager);

    entangle_preferences_display_show(priv->prefsDisplay);
}


static void do_app_quit(GtkMenuItem *src G_GNUC_UNUSED,
                        EntangleCameraManager *manager G_GNUC_UNUSED)
{
    gtk_main_quit();
}


static void do_help_about(GtkMenuItem *src G_GNUC_UNUSED,
                          EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (!priv->about)
        priv->about = entangle_help_about_new();

    entangle_help_about_show(priv->about);
}


static void do_manager_connect(GtkMenuItem *src G_GNUC_UNUSED,
                               EntangleCameraManager *manager)
{
    g_signal_emit_by_name(manager, "manager-connect", NULL);
}


static void do_manager_disconnect(GtkMenuItem *src G_GNUC_UNUSED,
                                  EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    do_remove_camera(manager);
    entangle_camera_disconnect(priv->camera);
    g_object_unref(priv->camera);
    priv->camera = NULL;
    do_capture_widget_sensitivity(manager);
    g_signal_emit_by_name(manager, "manager-disconnect", NULL);
}


static gboolean do_manager_delete(GtkWidget *widget G_GNUC_UNUSED,
                                  GdkEvent *event G_GNUC_UNUSED,
                                  EntangleCameraManager *manager G_GNUC_UNUSED)
{
    ENTANGLE_DEBUG("Got delete");
    gtk_main_quit();
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
        entangle_image_display_set_filename(priv->imageDisplay,
                                            entangle_image_get_filename(img));
        g_object_unref(img);
    }
}


static void do_popup_remove(gpointer data)
{
    EntangleImagePopup *pol = data;

    entangle_image_popup_hide(pol);
    g_object_unref(pol);
}


static void do_drag_failed(GtkWidget *widget,
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
            int x, y;
            gdk_display_get_pointer(gtk_widget_get_display(widget),
                                    &screen,
                                    &x, &y, NULL);
            GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
            const gchar *filename = entangle_image_get_filename(img);
            EntangleImagePopup *pol;
            if (!(pol = g_hash_table_lookup(priv->popups, filename))) {
                pol = entangle_image_popup_new();
                g_object_set(pol, "image-loader", priv->imageLoader, NULL);
                g_object_set(pol, "image", img, NULL);
                g_object_unref(img);
                g_hash_table_insert(priv->popups, g_strdup(filename), pol);
            }
            ENTANGLE_DEBUG("Popup %p for %s", pol, filename);
            entangle_image_popup_show(pol, GTK_WINDOW(win), x, y);
        }
    }
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
    XID xid;
    GdkDragProtocol protocol;
    GtkTargetEntry targets[] = {
        { g_strdup("demo"), GTK_TARGET_SAME_APP, 0,}
    };

    priv = manager->priv = ENTANGLE_CAMERA_MANAGER_GET_PRIVATE(manager);

    if (access("./entangle.glade", R_OK) == 0) {
        priv->glade = glade_xml_new("entangle.glade", "camera-manager", "entangle");
    } else {
        priv->glade = glade_xml_new(PKGDATADIR "/entangle.glade", "camera-manager", "entangle");
    }

    glade_xml_signal_connect_data(priv->glade, "camera_menu_help_summary", G_CALLBACK(do_manager_help_summary), manager);
    glade_xml_signal_connect_data(priv->glade, "camera_menu_help_manual", G_CALLBACK(do_manager_help_manual), manager);
    glade_xml_signal_connect_data(priv->glade, "camera_menu_help_driver", G_CALLBACK(do_manager_help_driver), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_new_click", G_CALLBACK(do_toolbar_new_session), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_open_click", G_CALLBACK(do_toolbar_open_session), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_capture_click", G_CALLBACK(do_toolbar_capture), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_capture_activate", G_CALLBACK(do_menu_capture), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_preview_activate", G_CALLBACK(do_menu_preview), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_monitor_activate", G_CALLBACK(do_menu_monitor), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_in_click", G_CALLBACK(do_toolbar_zoom_in), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_out_click", G_CALLBACK(do_toolbar_zoom_out), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_best_click", G_CALLBACK(do_toolbar_zoom_best), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_normal_click", G_CALLBACK(do_toolbar_zoom_normal), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_fullscreen_toggle", G_CALLBACK(do_toolbar_fullscreen), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_new_session_activate", G_CALLBACK(do_menu_new_session), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_open_session_activate", G_CALLBACK(do_menu_open_session), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_preferences_activate", G_CALLBACK(do_menu_preferences_activate), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_zoom_in_activate", G_CALLBACK(do_menu_zoom_in), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_zoom_out_activate", G_CALLBACK(do_menu_zoom_out), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_zoom_best_activate", G_CALLBACK(do_menu_zoom_best), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_zoom_normal_activate", G_CALLBACK(do_menu_zoom_normal), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_fullscreen_toggle", G_CALLBACK(do_menu_fullscreen), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_presentation_toggle", G_CALLBACK(do_menu_presentation), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_connect_activate", G_CALLBACK(do_manager_connect), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_disconnect_activate", G_CALLBACK(do_manager_disconnect), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_quit_activate", G_CALLBACK(do_app_quit), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_about_activate", G_CALLBACK(do_help_about), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_settings_toggled", G_CALLBACK(do_menu_settings_toggled), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_settings_toggled", G_CALLBACK(do_toolbar_settings_toggled), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_cancel_clicked", G_CALLBACK(do_toolbar_cancel_clicked), manager);

    win = glade_xml_get_widget(priv->glade, "camera-manager");
    g_signal_connect(win, "delete-event", G_CALLBACK(do_manager_delete), manager);

    menu = glade_xml_get_widget(priv->glade, "menu-monitor");
    monitorMenu = entangle_camera_manager_monitor_menu(manager);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), monitorMenu);

    viewport = glade_xml_get_widget(priv->glade, "image-viewport");

    priv->imageLoader = entangle_image_loader_new();
    priv->thumbLoader = entangle_thumbnail_loader_new(96, 96);

    priv->imageDisplay = entangle_image_display_new();
    priv->sessionBrowser = entangle_session_browser_new();
    priv->controlPanel = entangle_control_panel_new();

    entangle_image_display_set_image_loader(priv->imageDisplay, priv->imageLoader);
    g_object_set(priv->sessionBrowser, "thumbnail-loader", priv->thumbLoader, NULL);

    g_signal_connect(priv->sessionBrowser, "selection-changed",
                     G_CALLBACK(do_session_image_selected), manager);

    imageScroll = glade_xml_get_widget(priv->glade, "image-scroll");
    iconScroll = glade_xml_get_widget(priv->glade, "icon-scroll");
    settingsBox = glade_xml_get_widget(priv->glade, "settings-box");
    settingsViewport = glade_xml_get_widget(priv->glade, "settings-viewport");
    display = glade_xml_get_widget(priv->glade, "display-panel");

    gtk_icon_view_enable_model_drag_source(GTK_ICON_VIEW(priv->sessionBrowser),
                                           GDK_BUTTON1_MASK,
                                           targets,
                                           1,
                                           GDK_ACTION_PRIVATE);

    xid = gdk_x11_drawable_get_xid(GDK_DRAWABLE(gdk_get_default_root_window()));
    if (gdk_drag_get_protocol(xid, &protocol))
        gtk_drag_dest_set_proxy(win,
                                gdk_get_default_root_window(),
                                protocol, TRUE);


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

    ENTANGLE_DEBUG("Adding %p to %p", priv->imageDisplay, viewport);
    gtk_container_add(GTK_CONTAINER(viewport), GTK_WIDGET(priv->imageDisplay));
    gtk_container_add(GTK_CONTAINER(iconScroll), GTK_WIDGET(priv->sessionBrowser));
    gtk_container_add(GTK_CONTAINER(settingsViewport), GTK_WIDGET(priv->controlPanel));

    entangle_camera_manager_setup_capture_menu(manager);

    do_zoom_widget_sensitivity(manager);
    do_capture_widget_sensitivity(manager);
}

void entangle_camera_manager_show(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

    gtk_widget_show(win);
    gtk_widget_show(GTK_WIDGET(priv->controlPanel));
    gtk_widget_show(GTK_WIDGET(priv->imageDisplay));
    gtk_widget_show(GTK_WIDGET(priv->sessionBrowser));
    gtk_window_present(GTK_WINDOW(win));
}

void entangle_camera_manager_hide(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

    ENTANGLE_DEBUG("Removing all popups");
    g_hash_table_remove_all(priv->popups);

    gtk_widget_hide(win);
}

gboolean entangle_camera_manager_visible(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

    return GTK_WIDGET_FLAGS(win) & GTK_VISIBLE;
}


void entangle_camera_manager_set_camera(EntangleCameraManager *manager,
                                    EntangleCamera *cam)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    if (priv->camera) {
        do_remove_camera(manager);
        entangle_camera_disconnect(priv->camera);
        g_object_unref(priv->camera);
        priv->inOperation = FALSE;
    }
    priv->camera = cam;
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


EntanglePreferences *entangle_camera_manager_get_preferences(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    return priv->prefs;
}


EntanglePluginManager *entangle_camera_manager_get_plugin_manager(EntangleCameraManager *manager)
{
    EntangleCameraManagerPrivate *priv = manager->priv;

    return priv->pluginManager;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
