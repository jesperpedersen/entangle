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
#include <math.h>
#include <glade/glade.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

#include "capa-debug.h"
#include "capa-camera-manager.h"
#include "capa-camera-scheduler.h"
#include "capa-camera-list.h"
#include "capa-camera-info.h"
#include "capa-session.h"
#include "capa-image-display.h"
#include "capa-image-loader.h"
#include "capa-thumbnail-loader.h"
#include "capa-image-polaroid.h"
#include "capa-help-about.h"
#include "capa-session-browser.h"
#include "capa-control-panel.h"
#include "capa-colour-profile.h"
#include "capa-preferences-display.h"
#include "capa-progress.h"
#include "capa-camera-task-capture.h"
#include "capa-camera-task-preview.h"
#include "capa-camera-task-monitor.h"

#define CAPA_CAMERA_MANAGER_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerPrivate))

struct _CapaCameraManagerPrivate {
    CapaCamera *camera;
    CapaCameraScheduler *scheduler;
    CapaSession *session;

    CapaPreferences *prefs;
    CapaPluginManager *pluginManager;

    CapaHelpAbout *about;

    CapaCameraInfo *summary;
    CapaCameraInfo *manual;
    CapaCameraInfo *driver;
    CapaCameraInfo *supported;

    CapaImageLoader *imageLoader;
    CapaThumbnailLoader *thumbLoader;
    CapaColourProfileTransform *colourTransform;
    CapaImageDisplay *imageDisplay;
    CapaSessionBrowser *sessionBrowser;
    CapaControlPanel *controlPanel;
    CapaPreferencesDisplay *prefsDisplay;

    GHashTable *polaroids;

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

static void capa_camera_progress_interface_init (gpointer g_iface,
                                                 gpointer iface_data);

//G_DEFINE_TYPE(CapaCameraManager, capa_camera_manager, G_TYPE_OBJECT);
G_DEFINE_TYPE_EXTENDED(CapaCameraManager, capa_camera_manager, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(CAPA_TYPE_PROGRESS, capa_camera_progress_interface_init));

enum {
    PROP_O,
    PROP_CAMERA,
    PROP_PREFERENCES,
    PROP_PLUGIN_MANAGER,
};


static CapaColourProfile *capa_camera_manager_monitor_profile(GtkWindow *window)
{
    GdkScreen *screen;
    GByteArray *profileData;
    gchar *atom;
    int monitor = 0;
    GdkAtom type = GDK_NONE;
    gint format = 0;
    gint nitems = 0;
    guint8 *data = NULL;
    CapaColourProfile *profile = NULL;

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

    profile = capa_colour_profile_new_data(profileData);
    g_byte_array_unref(profileData);

 cleanup:
    g_free(data);
    g_free(atom);

    return profile;
}


static CapaColourProfileTransform *capa_camera_manager_colour_transform(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    CapaColourProfileTransform *transform = NULL;

    if (capa_preferences_enable_color_management(priv->prefs)) {
        CapaColourProfile *rgbProfile;
        CapaColourProfile *monitorProfile;
        CapaColourProfileIntent intent;

        rgbProfile = capa_preferences_rgb_profile(priv->prefs);
        intent = capa_preferences_profile_rendering_intent(priv->prefs);
        if (capa_preferences_detect_monitor_profile(priv->prefs)) {
            GtkWidget *window = glade_xml_get_widget(priv->glade, "camera-manager");
            monitorProfile = capa_camera_manager_monitor_profile(GTK_WINDOW(window));
        } else {
            monitorProfile = capa_preferences_monitor_profile(priv->prefs);
        }

        if (monitorProfile)
            transform = capa_colour_profile_transform_new(rgbProfile, monitorProfile, intent);
    }

    return transform;
}


static void capa_camera_manager_update_colour_transform(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (priv->colourTransform)
        g_object_unref(priv->colourTransform);

    priv->colourTransform = capa_camera_manager_colour_transform(manager);
    if (priv->imageLoader)
        capa_pixbuf_loader_set_colour_transform(CAPA_PIXBUF_LOADER(priv->imageLoader),
                                                priv->colourTransform);
    if (priv->thumbLoader)
        capa_pixbuf_loader_set_colour_transform(CAPA_PIXBUF_LOADER(priv->thumbLoader),
                                                priv->colourTransform);
}


static void capa_camera_manager_prefs_changed(GObject *object G_GNUC_UNUSED,
                                              GParamSpec *spec,
                                              gpointer opaque)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(opaque);

    if (strcmp(spec->name, "colour-managed-display") == 0 ||
        strcmp(spec->name, "rgb-profile") == 0 ||
        strcmp(spec->name, "monitor-profile") == 0 ||
        strcmp(spec->name, "detect-system-profile") == 0 ||
        strcmp(spec->name, "profile-rendering-intent") == 0) {
        capa_camera_manager_update_colour_transform(manager);
    }
}


static void do_capture_widget_sensitivity(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
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
                             capa_camera_get_has_capture(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(priv->menuItemCapture,
                             priv->camera &&
                             capa_camera_get_has_capture(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(priv->menuItemPreview,
                             priv->camera &&
                             capa_camera_get_has_preview(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(priv->menuItemMonitor,
                             priv->camera &&
                             capa_camera_get_has_capture(priv->camera) &&
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

    if (priv->camera && !capa_camera_get_has_capture(priv->camera)) {
        gtk_widget_set_tooltip_text(toolCapture, "This camera does not support image capture");
        gtk_widget_set_tooltip_text(priv->menuItemCapture, "This camera does not support image capture");
        /* XXX is this check correct ? unclear if some cameras can support wait-for-downloads
         * mode, but not be able to trigger the shutter for immediate capture */
        gtk_widget_set_tooltip_text(priv->menuItemMonitor, "This camera does not support image capture");
    }
    if (priv->camera && !capa_camera_get_has_preview(priv->camera))
        gtk_widget_set_tooltip_text(priv->menuItemPreview, "This camera does not support image preview");

    if (priv->camera && capa_camera_get_has_settings(priv->camera))
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


static void do_camera_file_download(CapaCamera *cam G_GNUC_UNUSED, CapaCameraFile *file, void *data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;
    CapaImage *image;
    gchar *localpath;

    CAPA_DEBUG("File download %p %p %p", cam, file, data);

    localpath = capa_session_next_filename(priv->session);

    if (!capa_camera_file_save_path(file, localpath, NULL)) {
        CAPA_DEBUG("Failed save path");
        goto cleanup;
    }
    CAPA_DEBUG("Saved to %s", localpath);
    image = capa_image_new(localpath);

    gdk_threads_enter();
    capa_session_add(priv->session, image);

    capa_image_display_set_filename(priv->imageDisplay, capa_image_filename(image));
    gdk_threads_leave();

    g_object_unref(image);

 cleanup:
    g_free(localpath);
}


static void do_camera_file_preview(CapaCamera *cam G_GNUC_UNUSED, CapaCameraFile *file, void *data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;
    GdkPixbuf *pixbuf;
    GByteArray *bytes;
    GInputStream *is;

    CAPA_DEBUG("File preview %p %p %p", cam, file, data);

    bytes = capa_camera_file_get_data(file);
    is = g_memory_input_stream_new_from_data(bytes->data, bytes->len, NULL);

    pixbuf = gdk_pixbuf_new_from_stream(is, NULL, NULL);

    gdk_threads_enter();
    capa_image_display_set_pixbuf(priv->imageDisplay, pixbuf);
    gdk_threads_leave();

    g_object_unref(pixbuf);
    g_object_unref(is);
}


static void do_camera_task_begin(CapaCamera *cam G_GNUC_UNUSED, CapaCameraTask *task G_GNUC_UNUSED, void *data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;

    priv->operationCancel = FALSE;
}

static void do_camera_task_end(CapaCamera *cam G_GNUC_UNUSED, CapaCameraTask *task G_GNUC_UNUSED, void *data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;

    priv->inOperation = FALSE;
    priv->operationCancel = FALSE;
    do_capture_widget_sensitivity(manager);
}

static void do_capa_camera_progress_start(CapaProgress *iface, float target, const char *format, va_list args)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(iface);
    CapaCameraManagerPrivate *priv = manager->priv;
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

static void do_capa_camera_progress_update(CapaProgress *iface, float current)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(iface);
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;

    gdk_threads_enter();

    mtr = glade_xml_get_widget(priv->glade, "toolbar-progress");

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), current / priv->operationTarget);

    gdk_threads_leave();
}

static void do_capa_camera_progress_stop(CapaProgress *iface)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(iface);
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *mtr;

    gdk_threads_enter();

    mtr = glade_xml_get_widget(priv->glade, "toolbar-progress");

    gtk_widget_set_tooltip_text(mtr, "");
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mtr), "");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mtr), 0);

    gdk_threads_leave();
}

static gboolean do_capa_camera_progress_cancelled(CapaProgress *iface)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(iface);
    CapaCameraManagerPrivate *priv = manager->priv;

    CAPA_DEBUG("Cancel called");

    return priv->operationCancel;
}

static void capa_camera_progress_interface_init (gpointer g_iface,
                                                 gpointer iface_data G_GNUC_UNUSED)
{
    CapaProgressInterface *iface = g_iface;
    iface->start = do_capa_camera_progress_start;
    iface->update = do_capa_camera_progress_update;
    iface->stop = do_capa_camera_progress_stop;
    iface->cancelled = do_capa_camera_progress_cancelled;
}

static void do_remove_camera(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    char *title;
    GtkWidget *win;

    title = g_strdup_printf("Camera Manager - Capa");
    win = glade_xml_get_widget(priv->glade, "camera-manager");
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);

    capa_session_browser_set_session(priv->sessionBrowser, NULL);
    capa_control_panel_set_camera(priv->controlPanel, NULL);
    capa_camera_set_progress(priv->camera, NULL);

    capa_image_display_set_filename(priv->imageDisplay, NULL);
    capa_image_display_set_pixbuf(priv->imageDisplay, NULL);

    g_signal_handler_disconnect(priv->camera, priv->sigFilePreview);
    g_signal_handler_disconnect(priv->camera, priv->sigFileDownload);
    g_signal_handler_disconnect(priv->scheduler, priv->sigTaskBegin);
    g_signal_handler_disconnect(priv->scheduler, priv->sigTaskEnd);

    capa_camera_scheduler_end(priv->scheduler);
    g_object_unref(priv->scheduler);
    g_object_unref(priv->session);
    priv->scheduler = NULL;
    priv->session = NULL;
}

static void do_add_camera(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    char *title;
    GtkWidget *win;
    char *directory;

    priv->scheduler = capa_camera_scheduler_new(priv->camera);

    title = g_strdup_printf("%s Camera Manager - Capa",
                            capa_camera_get_model(priv->camera));

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
                                capa_preferences_picture_dir(priv->prefs),
                                capa_camera_get_model(priv->camera));

    priv->session = capa_session_new(directory,
                                     capa_preferences_filename_pattern(priv->prefs));
    capa_session_load(priv->session);

    capa_camera_set_progress(priv->camera, CAPA_PROGRESS(manager));
    capa_session_browser_set_session(priv->sessionBrowser, priv->session);
    capa_control_panel_set_camera(priv->controlPanel, priv->camera);

    g_free(directory);

    capa_camera_scheduler_start(priv->scheduler);
}


static void capa_camera_manager_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(object);
    CapaCameraManagerPrivate *priv = manager->priv;

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


static void capa_camera_manager_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(object);
    CapaCameraManagerPrivate *priv = manager->priv;

    CAPA_DEBUG("Set prop %d", prop_id);

    switch (prop_id)
        {
        case PROP_CAMERA:
            capa_camera_manager_set_camera(manager, g_value_get_object(value));
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
                                                        G_CALLBACK(capa_camera_manager_prefs_changed),
                                                        manager);
            }

            capa_camera_manager_update_colour_transform(manager);
            break;

        case PROP_PLUGIN_MANAGER:
            if (priv->pluginManager)
                g_object_unref(priv->pluginManager);
            priv->pluginManager = g_value_get_object(value);
            if (priv->pluginManager)
                g_object_ref(priv->pluginManager);

            if (priv->prefsDisplay)
                g_object_set(priv->prefsDisplay, "plugin-manager", priv->pluginManager, NULL);

            capa_camera_manager_update_colour_transform(manager);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_camera_manager_finalize (GObject *object)
{
    CapaCameraManager *manager = CAPA_CAMERA_MANAGER(object);
    CapaCameraManagerPrivate *priv = manager->priv;

    CAPA_DEBUG("Finalize manager");

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

    g_hash_table_destroy(priv->polaroids);

    g_object_unref(priv->glade);

    G_OBJECT_CLASS (capa_camera_manager_parent_class)->finalize (object);
}


static void capa_camera_manager_class_init(CapaCameraManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_camera_manager_finalize;
    object_class->get_property = capa_camera_manager_get_property;
    object_class->set_property = capa_camera_manager_set_property;

    g_signal_new("manager-connect",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraManagerClass, manager_connect),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_signal_new("manager-disconnect",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(CapaCameraManagerClass, manager_disconnect),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE,
                 0);

    g_object_class_install_property(object_class,
                                    PROP_CAMERA,
                                    g_param_spec_object("camera",
                                                        "Camera",
                                                        "Camera to be managed",
                                                        CAPA_TYPE_CAMERA,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PREFERENCES,
                                    g_param_spec_object("preferences",
                                                        "Preferences",
                                                        "Application preferences",
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

    g_type_class_add_private(klass, sizeof(CapaCameraManagerPrivate));
}


CapaCameraManager *capa_camera_manager_new(CapaPreferences *prefs,
                                           CapaPluginManager *pluginManager)
{
    return CAPA_CAMERA_MANAGER(g_object_new(CAPA_TYPE_CAMERA_MANAGER,
                                            "preferences", prefs,
                                            "plugin-manager", pluginManager,
                                            NULL));
}

static void do_manager_help_summary(GtkMenuItem *src G_GNUC_UNUSED,
                                    CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (!priv->summary)
        priv->summary = capa_camera_info_new(priv->camera,
                                             CAPA_CAMERA_INFO_DATA_SUMMARY);

    capa_camera_info_show(priv->summary);
}

static void do_manager_help_manual(GtkMenuItem *src G_GNUC_UNUSED,
                                   CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (!priv->manual)
        priv->manual = capa_camera_info_new(priv->camera,
                                            CAPA_CAMERA_INFO_DATA_MANUAL);

    capa_camera_info_show(priv->manual);
}

static void do_manager_help_driver(GtkMenuItem *src G_GNUC_UNUSED,
                                   CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (!priv->driver)
        priv->driver = capa_camera_info_new(priv->camera,
                                            CAPA_CAMERA_INFO_DATA_DRIVER);

    capa_camera_info_show(priv->driver);
}

static void capa_camera_manager_new_session(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
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
                          capa_preferences_picture_dir(priv->prefs),
                          capa_camera_get_model(priv->camera));
    g_mkdir_with_parents(dir, 0777);
    CAPA_DEBUG("Set curent folder '%s'", dir);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
    g_free(dir);

    gtk_widget_hide(chooser);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        CapaSession *session;
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        session = capa_session_new(dir, capa_preferences_filename_pattern(priv->prefs));
        capa_session_load(session);
        if (priv->session)
            g_object_unref(priv->session);
        priv->session = session;
        capa_session_browser_set_session(priv->sessionBrowser, session);
    }

    gtk_widget_destroy(chooser);
}

static void capa_camera_manager_open_session(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
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
                          capa_preferences_picture_dir(priv->prefs),
                          capa_camera_get_model(priv->camera));
    g_mkdir_with_parents(dir, 0777);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
    g_free(dir);

    gtk_widget_hide(chooser);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        CapaSession *session;
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        session = capa_session_new(dir, capa_preferences_filename_pattern(priv->prefs));
        capa_session_load(session);
        if (priv->session)
            g_object_unref(priv->session);
        priv->session = session;
        capa_session_browser_set_session(priv->sessionBrowser, session);
    }

    gtk_widget_destroy(chooser);
}

static void do_toolbar_new_session(GtkToolButton *src G_GNUC_UNUSED,
                                   CapaCameraManager *manager)
{
    capa_camera_manager_new_session(manager);
}

static void do_toolbar_open_session(GtkToolButton *src G_GNUC_UNUSED,
                                    CapaCameraManager *manager)
{
    capa_camera_manager_open_session(manager);
}

static void do_menu_new_session(GtkImageMenuItem *src G_GNUC_UNUSED,
                                CapaCameraManager *manager)
{
    capa_camera_manager_new_session(manager);
}

static void do_menu_open_session(GtkImageMenuItem *src G_GNUC_UNUSED,
                                 CapaCameraManager *manager)
{
    capa_camera_manager_open_session(manager);
}

static void do_menu_settings_toggled(GtkCheckMenuItem *src,
                                     CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
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
                                        CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
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
                                      CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    priv->operationCancel = TRUE;
}


static void do_toolbar_capture(GtkToolButton *src G_GNUC_UNUSED,
                               CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    CapaCameraTask *task;

    CAPA_DEBUG("starting Capture thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = CAPA_CAMERA_TASK(capa_camera_task_capture_new());
    capa_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void do_menu_capture(GtkMenuItem *src G_GNUC_UNUSED,
                            CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    CapaCameraTask *task;

    CAPA_DEBUG("starting Capture thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = CAPA_CAMERA_TASK(capa_camera_task_capture_new());
    capa_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void do_menu_preview(GtkMenuItem *src G_GNUC_UNUSED,
                            CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    CapaCameraTask *task;

    CAPA_DEBUG("starting Preview thread");
    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = CAPA_CAMERA_TASK(capa_camera_task_preview_new());
    capa_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void do_menu_monitor(GtkMenuItem *src G_GNUC_UNUSED,
                            CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    CapaCameraTask *task;

    CAPA_DEBUG("starting monitor thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);

    task = CAPA_CAMERA_TASK(capa_camera_task_monitor_new());
    capa_camera_scheduler_queue(priv->scheduler, task);
    g_object_unref(task);
}

static void capa_camera_manager_setup_capture_menu(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
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


static void do_zoom_widget_sensitivity(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolzoomnormal = glade_xml_get_widget(priv->glade, "toolbar-zoom-normal");
    GtkWidget *toolzoombest = glade_xml_get_widget(priv->glade, "toolbar-zoom-best");
    GtkWidget *toolzoomin = glade_xml_get_widget(priv->glade, "toolbar-zoom-in");
    GtkWidget *toolzoomout = glade_xml_get_widget(priv->glade, "toolbar-zoom-out");

    GtkWidget *menuzoomnormal = glade_xml_get_widget(priv->glade, "menu-zoom-normal");
    GtkWidget *menuzoombest = glade_xml_get_widget(priv->glade, "menu-zoom-best");
    GtkWidget *menuzoomin = glade_xml_get_widget(priv->glade, "menu-zoom-in");
    GtkWidget *menuzoomout = glade_xml_get_widget(priv->glade, "menu-zoom-out");
    gboolean autoscale;

    autoscale = capa_image_display_get_autoscale(priv->imageDisplay);

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

static void capa_camera_manager_update_zoom(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel > 0)
        capa_image_display_set_scale(priv->imageDisplay, 1.0+priv->zoomLevel);
    else if (priv->zoomLevel < 0)
        capa_image_display_set_scale(priv->imageDisplay, 1.0/pow(1.5, -priv->zoomLevel));
    else
        capa_image_display_set_scale(priv->imageDisplay, 0.0);
    do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_in(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel < 10)
        priv->zoomLevel += 1;

    capa_camera_manager_update_zoom(manager);
}

static void capa_camera_manager_zoom_out(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel > -10)
        priv->zoomLevel -= 1;

    capa_camera_manager_update_zoom(manager);
}

static void capa_camera_manager_zoom_normal(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    capa_image_display_set_scale(priv->imageDisplay, 0.0);
    capa_image_display_set_autoscale(priv->imageDisplay, FALSE);
    do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_best(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    capa_image_display_set_autoscale(priv->imageDisplay, TRUE);
    do_zoom_widget_sensitivity(manager);
}

static void do_toolbar_zoom_in(GtkToolButton *src G_GNUC_UNUSED,
                               CapaCameraManager *manager)
{
    capa_camera_manager_zoom_in(manager);
}

static void do_toolbar_zoom_out(GtkToolButton *src G_GNUC_UNUSED,
                                CapaCameraManager *manager)
{
    capa_camera_manager_zoom_out(manager);
}

static void do_toolbar_zoom_normal(GtkToolButton *src G_GNUC_UNUSED,
                                   CapaCameraManager *manager)
{
    capa_camera_manager_zoom_normal(manager);
}

static void do_toolbar_zoom_best(GtkToolButton *src G_GNUC_UNUSED,
                                 CapaCameraManager *manager)
{
    capa_camera_manager_zoom_best(manager);
}


static void do_menu_zoom_in(GtkImageMenuItem *src G_GNUC_UNUSED,
                            CapaCameraManager *manager)
{
    capa_camera_manager_zoom_in(manager);
}

static void do_menu_zoom_out(GtkImageMenuItem *src G_GNUC_UNUSED,
                             CapaCameraManager *manager)
{
    capa_camera_manager_zoom_out(manager);
}

static void do_menu_zoom_normal(GtkImageMenuItem *src G_GNUC_UNUSED,
                                CapaCameraManager *manager)
{
    capa_camera_manager_zoom_normal(manager);
}

static void do_menu_zoom_best(GtkImageMenuItem *src G_GNUC_UNUSED,
                              CapaCameraManager *manager)
{
    capa_camera_manager_zoom_best(manager);
}


static void do_toolbar_fullscreen(GtkToggleToolButton *src,
                                  CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
    GtkWidget *menu = glade_xml_get_widget(priv->glade, "menu-fullscreen");

    if (gtk_toggle_tool_button_get_active(src))
        gtk_window_fullscreen(GTK_WINDOW(win));
    else
        gtk_window_unfullscreen(GTK_WINDOW(win));

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) !=
        gtk_toggle_tool_button_get_active(src))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu),
                                       gtk_toggle_tool_button_get_active(src));
}

static void do_menu_fullscreen(GtkCheckMenuItem *src,
                               CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
    GtkWidget *tool = glade_xml_get_widget(priv->glade, "toolbar-fullscreen");

    if (gtk_check_menu_item_get_active(src))
        gtk_window_fullscreen(GTK_WINDOW(win));
    else
        gtk_window_unfullscreen(GTK_WINDOW(win));

    if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(tool)) !=
        gtk_check_menu_item_get_active(src))
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool),
                                          gtk_check_menu_item_get_active(src));
}


static void do_menu_preferences_activate(GtkCheckMenuItem *src G_GNUC_UNUSED,
                                         CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    if (priv->prefsDisplay == NULL)
        priv->prefsDisplay = capa_preferences_display_new(priv->prefs,
                                                          priv->pluginManager);

    capa_preferences_display_show(priv->prefsDisplay);
}


static void do_app_quit(GtkMenuItem *src G_GNUC_UNUSED,
                        CapaCameraManager *manager G_GNUC_UNUSED)
{
    gtk_main_quit();
}


static void do_help_about(GtkMenuItem *src G_GNUC_UNUSED,
                          CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (!priv->about)
        priv->about = capa_help_about_new();

    capa_help_about_show(priv->about);
}


static void do_manager_connect(GtkMenuItem *src G_GNUC_UNUSED,
                               CapaCameraManager *manager)
{
    g_signal_emit_by_name(manager, "manager-connect", NULL);
}


static void do_manager_disconnect(GtkMenuItem *src G_GNUC_UNUSED,
                                  CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    do_remove_camera(manager);
    capa_camera_disconnect(priv->camera);
    g_object_unref(priv->camera);
    priv->camera = NULL;
    do_capture_widget_sensitivity(manager);
    g_signal_emit_by_name(manager, "manager-disconnect", NULL);
}


static gboolean do_manager_delete(GtkWidget *widget G_GNUC_UNUSED,
                                  GdkEvent *event G_GNUC_UNUSED,
                                  CapaCameraManager *manager G_GNUC_UNUSED)
{
    CAPA_DEBUG("Got delete");
    gtk_main_quit();
    return TRUE;
}


static void do_session_image_selected(GtkIconView *view G_GNUC_UNUSED,
                                      gpointer data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;
    CapaImage *img = capa_session_browser_selected_image(priv->sessionBrowser);

    CAPA_DEBUG("Image selection changed");
    if (img) {
        CAPA_DEBUG("Try load");
        capa_image_display_set_filename(priv->imageDisplay, capa_image_filename(img));
        g_object_unref(img);
    }
}


static void do_polaroid_remove(gpointer data)
{
    CapaImagePolaroid *pol = data;

    capa_image_polaroid_hide(pol);
    g_object_unref(pol);
}


static void do_drag_failed(GtkWidget *widget,
                           GdkDragContext *ctx G_GNUC_UNUSED,
                           GtkDragResult res,
                           gpointer data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;

    if (res == GTK_DRAG_RESULT_NO_TARGET) {
        CapaImage *img = capa_session_browser_selected_image(priv->sessionBrowser);
        if (img) {
            GdkScreen *screen;
            int x, y;
            gdk_display_get_pointer(gtk_widget_get_display(widget),
                                    &screen,
                                    &x, &y, NULL);
            GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");
            const gchar *filename = capa_image_filename(img);
            CapaImagePolaroid *pol;
            if (!(pol = g_hash_table_lookup(priv->polaroids, filename))) {
                pol = capa_image_polaroid_new();
                g_object_set(pol, "image-loader", priv->imageLoader, NULL);
                g_object_set(pol, "image", img, NULL);
                g_object_unref(img);
                g_hash_table_insert(priv->polaroids, g_strdup(filename), pol);
            }
            CAPA_DEBUG("Polaroid %p for %s", pol, filename);
            capa_image_polaroid_show(pol, GTK_WINDOW(win), x, y);
        }
    }
}

static void capa_camera_manager_init(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv;
    GtkWidget *viewport;
    GtkWidget *display;
    GtkWidget *imageScroll;
    GtkWidget *iconScroll;
    GtkWidget *settingsBox;
    GtkWidget *settingsViewport;
    GtkWidget *win;
    XID xid;
    GdkDragProtocol protocol;
    GtkTargetEntry targets[] = {
        { g_strdup("demo"), GTK_TARGET_SAME_APP, 0,}
    };

    priv = manager->priv = CAPA_CAMERA_MANAGER_GET_PRIVATE(manager);

    if (access("./capa.glade", R_OK) == 0) {
        priv->glade = glade_xml_new("capa.glade", "camera-manager", "capa");
    } else {
        priv->glade = glade_xml_new(PKGDATADIR "/capa.glade", "camera-manager", "capa");
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

    glade_xml_signal_connect_data(priv->glade, "menu_connect_activate", G_CALLBACK(do_manager_connect), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_disconnect_activate", G_CALLBACK(do_manager_disconnect), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_quit_activate", G_CALLBACK(do_app_quit), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_about_activate", G_CALLBACK(do_help_about), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_settings_toggled", G_CALLBACK(do_menu_settings_toggled), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_settings_toggled", G_CALLBACK(do_toolbar_settings_toggled), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_cancel_clicked", G_CALLBACK(do_toolbar_cancel_clicked), manager);

    win = glade_xml_get_widget(priv->glade, "camera-manager");
    g_signal_connect(win, "delete-event", G_CALLBACK(do_manager_delete), manager);

    viewport = glade_xml_get_widget(priv->glade, "image-viewport");

    priv->imageLoader = capa_image_loader_new();
    priv->thumbLoader = capa_thumbnail_loader_new(96, 96);

    priv->imageDisplay = capa_image_display_new();
    priv->sessionBrowser = capa_session_browser_new();
    priv->controlPanel = capa_control_panel_new();

    capa_image_display_set_image_loader(priv->imageDisplay, priv->imageLoader);
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
    gtk_widget_set_size_request(iconScroll, 140, 140);

    priv->polaroids = g_hash_table_new_full(g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            do_polaroid_remove);

    CAPA_DEBUG("Adding %p to %p", priv->imageDisplay, viewport);
    gtk_container_add(GTK_CONTAINER(viewport), GTK_WIDGET(priv->imageDisplay));
    gtk_container_add(GTK_CONTAINER(iconScroll), GTK_WIDGET(priv->sessionBrowser));
    gtk_container_add(GTK_CONTAINER(settingsViewport), GTK_WIDGET(priv->controlPanel));

    capa_camera_manager_setup_capture_menu(manager);

    do_zoom_widget_sensitivity(manager);
    do_capture_widget_sensitivity(manager);
}

void capa_camera_manager_show(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

    gtk_widget_show(win);
    gtk_widget_show(GTK_WIDGET(priv->controlPanel));
    gtk_widget_show(GTK_WIDGET(priv->imageDisplay));
    gtk_widget_show(GTK_WIDGET(priv->sessionBrowser));
    gtk_window_present(GTK_WINDOW(win));
}

void capa_camera_manager_hide(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

    CAPA_DEBUG("Removing all polaroids");
    g_hash_table_remove_all(priv->polaroids);

    gtk_widget_hide(win);
}

gboolean capa_camera_manager_visible(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

    return GTK_WIDGET_FLAGS(win) & GTK_VISIBLE;
}


void capa_camera_manager_set_camera(CapaCameraManager *manager,
                                    CapaCamera *cam)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (priv->camera) {
        do_remove_camera(manager);
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


CapaCamera *capa_camera_manager_get_camera(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    return priv->camera;
}


CapaPreferences *capa_camera_manager_get_preferences(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    return priv->prefs;
}


CapaPluginManager *capa_camera_manager_get_plugin_manager(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

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
