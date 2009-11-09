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

#include "internal.h"
#include "camera-manager.h"
#include "camera-list.h"
#include "camera-info.h"
#include "camera-progress.h"
#include "image-display.h"
#include "image-polaroid.h"
#include "help-about.h"
#include "session-browser.h"
#include "control-panel.h"
#include "colour-profile.h"


#define CAPA_CAMERA_MANAGER_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerPrivate))

struct _CapaCameraManagerPrivate {
    CapaCamera *camera;
    CapaPreferences *prefs;

    CapaHelpAbout *about;

    CapaCameraInfo *summary;
    CapaCameraInfo *manual;
    CapaCameraInfo *driver;
    CapaCameraInfo *supported;

    CapaCameraProgress *progress;

    CapaImageDisplay *imageDisplay;
    CapaSessionBrowser *sessionBrowser;
    CapaControlPanel *controlPanel;

    GHashTable *polaroids;

    int zoomLevel;

    gulong sigImage;
    gulong sigError;
    gulong sigOpBegin;
    gulong sigOpEnd;

    gboolean inOperation;

    GladeXML *glade;
};

G_DEFINE_TYPE(CapaCameraManager, capa_camera_manager, G_TYPE_OBJECT);

enum {
    PROP_O,
    PROP_CAMERA,
    PROP_PREFERENCES,
};

static void do_capture_widget_sensitivity(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GtkWidget *toolCapture;
    GtkWidget *toolPreview;
    GtkWidget *toolMonitor;
    GtkWidget *settingsScroll;

    toolCapture = glade_xml_get_widget(priv->glade, "toolbar-capture");
    toolPreview = glade_xml_get_widget(priv->glade, "toolbar-preview");
    toolMonitor = glade_xml_get_widget(priv->glade, "toolbar-monitor");
    settingsScroll = glade_xml_get_widget(priv->glade, "settings-scroll");

    gtk_widget_set_sensitive(toolCapture,
                             priv->camera &&
                             capa_camera_has_capture(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(toolPreview,
                             priv->camera &&
                             capa_camera_has_preview(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);
    gtk_widget_set_sensitive(toolMonitor,
                             priv->camera &&
                             capa_camera_has_capture(priv->camera) &&
                             !priv->inOperation ? TRUE : FALSE);

    if (priv->camera && !capa_camera_has_capture(priv->camera)) {
        gtk_widget_set_tooltip_text(toolCapture, "This camera does not support image capture");
        /* XXX is this check correct ? unclear */
        gtk_widget_set_tooltip_text(toolMonitor, "This camera does not support image capture");
    }
    if (priv->camera && !capa_camera_has_preview(priv->camera))
        gtk_widget_set_tooltip_text(toolPreview, "This camera does not support image preview");

    if (priv->camera && !capa_camera_has_settings(priv->camera))
        gtk_widget_hide(settingsScroll);
}


static void do_load_image(CapaCameraManager *manager, CapaImage *image)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GdkPixbuf *srcpixbuf;
    /* XXX make a preference */
    char *srcproffile = getenv("CAPA_ICC_SRC");
    char *dstproffile = getenv("CAPA_ICC_DST");

    srcpixbuf = gdk_pixbuf_new_from_file(capa_image_filename(image), NULL);

    if (access(srcproffile, R_OK) == 0 &&
        access(dstproffile, R_OK) == 0) {
        GdkPixbuf *dstpixbuf;
        CapaColourProfile *srcprof;
        CapaColourProfile *dstprof;
        srcprof = capa_colour_profile_new(srcproffile);
        dstprof = capa_colour_profile_new(dstproffile);

        dstpixbuf = capa_colour_profile_convert(srcprof, dstprof, srcpixbuf);

        g_object_set(G_OBJECT(priv->imageDisplay),
                     "pixbuf", dstpixbuf,
                     NULL);
        g_object_unref(dstpixbuf);
    } else {
        g_object_set(G_OBJECT(priv->imageDisplay),
                     "pixbuf", srcpixbuf,
                     NULL);
    }
    gdk_pixbuf_unref(srcpixbuf);
}

static void do_camera_image(CapaCamera *cam G_GNUC_UNUSED, CapaImage *image, void *data)
{
    CapaCameraManager *manager = data;

    do_load_image(manager, image);
}

static void do_camera_error(CapaCamera *cam G_GNUC_UNUSED, const char *err, void *data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;

    CAPA_DEBUG("Something went wrong '%s' %p", err, priv);
}

static void do_camera_op_begin(CapaCamera *cam G_GNUC_UNUSED, void *data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;

    capa_camera_progress_show(priv->progress, "Capturing image");
}

static void do_camera_op_end(CapaCamera *cam G_GNUC_UNUSED, void *data)
{
    CapaCameraManager *manager = data;
    CapaCameraManagerPrivate *priv = manager->priv;

    capa_camera_progress_hide(priv->progress);
    priv->inOperation = FALSE;
    do_capture_widget_sensitivity(manager);
}

static void do_remove_camera(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    g_object_set(G_OBJECT(priv->camera),
                 "progress", NULL,
                 "session", NULL, NULL);

    g_signal_handler_disconnect(G_OBJECT(priv->camera), priv->sigImage);
    g_signal_handler_disconnect(G_OBJECT(priv->camera), priv->sigError);
    g_signal_handler_disconnect(G_OBJECT(priv->camera), priv->sigOpBegin);
    g_signal_handler_disconnect(G_OBJECT(priv->camera), priv->sigOpEnd);
}

static void do_add_camera(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    char *title;
    GtkWidget *win;
    CapaSession *session;
    char *directory;

    title = g_strdup_printf("%s Camera Manager - Capa",
                            capa_camera_model(priv->camera));

    win = glade_xml_get_widget(priv->glade, "camera-manager");
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);

    priv->sigImage = g_signal_connect(G_OBJECT(priv->camera), "camera-image",
                                      G_CALLBACK(do_camera_image), manager);
    priv->sigError = g_signal_connect(G_OBJECT(priv->camera), "camera-error",
                                      G_CALLBACK(do_camera_error), manager);
    priv->sigOpBegin = g_signal_connect(G_OBJECT(priv->camera), "camera-op-begin",
                                        G_CALLBACK(do_camera_op_begin), manager);
    priv->sigOpEnd = g_signal_connect(G_OBJECT(priv->camera), "camera-op-end",
                                      G_CALLBACK(do_camera_op_end), manager);

    directory = g_strdup_printf("%s/%s/Default Session",
                                capa_preferences_picture_dir(priv->prefs),
                                capa_camera_model(priv->camera));

    session = capa_session_new(directory,
                               capa_preferences_filename_pattern(priv->prefs));
    capa_session_load(session);

    g_object_set(G_OBJECT(priv->camera),
                 "progress", priv->progress,
                 "session", session,
                 NULL);

    g_object_set(G_OBJECT(priv->sessionBrowser),
                 "session", session,
                 NULL);

    g_object_set(G_OBJECT(priv->controlPanel),
                 "camera", priv->camera,
                 NULL);

    g_object_unref(G_OBJECT(session));
    g_free(directory);
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
        case PROP_CAMERA: {
            if (priv->camera) {
                do_remove_camera(manager);
                g_object_unref(G_OBJECT(priv->camera));
            }
            priv->camera = g_value_get_object(value);
            g_object_ref(G_OBJECT(priv->camera));

            do_add_camera(manager);
            do_capture_widget_sensitivity(manager);
        } break;

        case PROP_PREFERENCES:
            if (priv->prefs)
                g_object_unref(G_OBJECT(priv->prefs));
            priv->prefs = g_value_get_object(value);
            g_object_ref(G_OBJECT(priv->prefs));
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

    if (priv->camera)
        g_object_unref(G_OBJECT(priv->camera));
    if (priv->prefs)
        g_object_unref(G_OBJECT(priv->prefs));

    g_hash_table_destroy(priv->polaroids);

    g_object_unref(G_OBJECT(priv->progress));
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

    g_type_class_add_private(klass, sizeof(CapaCameraManagerPrivate));

    capa_camera_set_thread_funcs(gdk_threads_enter,
                                 gdk_threads_leave);
}


CapaCameraManager *capa_camera_manager_new(CapaPreferences *prefs)
{
    return CAPA_CAMERA_MANAGER(g_object_new(CAPA_TYPE_CAMERA_MANAGER,
                                            "preferences", prefs,
                                            NULL));
}

static void do_manager_help_summary(GtkMenuItem *src G_GNUC_UNUSED,
                                    CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (!priv->summary) {
        priv->summary = capa_camera_info_new();
        g_object_set(G_OBJECT(priv->summary),
                     "data", CAPA_CAMERA_INFO_DATA_SUMMARY,
                     "camera", priv->camera,
                     NULL);
    }
    capa_camera_info_show(priv->summary);
}

static void do_manager_help_manual(GtkMenuItem *src G_GNUC_UNUSED,
                                   CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (!priv->manual) {
        priv->manual = capa_camera_info_new();
        g_object_set(G_OBJECT(priv->manual),
                     "data", CAPA_CAMERA_INFO_DATA_MANUAL,
                     "camera", priv->camera,
                     NULL);
    }
    capa_camera_info_show(priv->manual);
}

static void do_manager_help_driver(GtkMenuItem *src G_GNUC_UNUSED,
                                   CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (!priv->driver) {
        priv->driver = capa_camera_info_new();
        g_object_set(G_OBJECT(priv->driver),
                     "data", CAPA_CAMERA_INFO_DATA_DRIVER,
                     "camera", priv->camera,
                     NULL);
    }
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
                          capa_camera_model(priv->camera));
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
        g_object_set(G_OBJECT(priv->camera), "session", session, NULL);
        g_object_set(G_OBJECT(priv->sessionBrowser),
                     "session", session,
                     NULL);

        g_object_unref(session);
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
                          capa_camera_model(priv->camera));
    g_mkdir_with_parents(dir, 0777);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), dir);
    g_free(dir);

    gtk_widget_hide(chooser);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        CapaSession *session;
        dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        session = capa_session_new(dir, capa_preferences_filename_pattern(priv->prefs));
        capa_session_load(session);
        g_object_set(G_OBJECT(priv->camera), "session", session, NULL);
        g_object_set(G_OBJECT(priv->sessionBrowser),
                     "session", session,
                     NULL);

        g_object_unref(session);
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


static void do_toolbar_capture(GtkToolButton *src G_GNUC_UNUSED,
                               CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    CAPA_DEBUG("starting Capture thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);
    capa_camera_capture(priv->camera);
}

static void do_toolbar_preview(GtkToolButton *src G_GNUC_UNUSED,
                               CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    CAPA_DEBUG("starting Preview thread");
    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);
    capa_camera_preview(priv->camera);
}

static void do_toolbar_monitor(GtkToolButton *src G_GNUC_UNUSED,
                               CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    CAPA_DEBUG("starting monitor thread");

    if (priv->inOperation)
        return;

    priv->inOperation = TRUE;
    do_capture_widget_sensitivity(manager);
    capa_camera_monitor(priv->camera);
}

static void do_zoom_widget_sensitivity(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;
    GValue autoscale;
    GtkWidget *toolzoomnormal = glade_xml_get_widget(priv->glade, "toolbar-zoom-normal");
    GtkWidget *toolzoombest = glade_xml_get_widget(priv->glade, "toolbar-zoom-best");
    GtkWidget *toolzoomin = glade_xml_get_widget(priv->glade, "toolbar-zoom-in");
    GtkWidget *toolzoomout = glade_xml_get_widget(priv->glade, "toolbar-zoom-out");

    GtkWidget *menuzoomnormal = glade_xml_get_widget(priv->glade, "menu-zoom-normal");
    GtkWidget *menuzoombest = glade_xml_get_widget(priv->glade, "menu-zoom-best");
    GtkWidget *menuzoomin = glade_xml_get_widget(priv->glade, "menu-zoom-in");
    GtkWidget *menuzoomout = glade_xml_get_widget(priv->glade, "menu-zoom-out");


    memset(&autoscale, 0, sizeof autoscale);
    g_value_init(&autoscale, G_TYPE_BOOLEAN);
    g_object_get_property(G_OBJECT(priv->imageDisplay), "autoscale", &autoscale);

    if (g_value_get_boolean(&autoscale)) {
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

static void capa_camera_manager_zoom_in(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel < 10)
        priv->zoomLevel += 1;
    if (priv->zoomLevel > 0)
        g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0+priv->zoomLevel, NULL);
    else if (priv->zoomLevel < 0)
        g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0/pow(1.5, -priv->zoomLevel), NULL);
    else
        g_object_set(G_OBJECT(priv->imageDisplay), "scale", 0.0, NULL);
    do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_out(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    if (priv->zoomLevel > -10)
        priv->zoomLevel -= 1;
    if (priv->zoomLevel > 0)
        g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0+priv->zoomLevel, NULL);
    else if (priv->zoomLevel < 0)
        g_object_set(G_OBJECT(priv->imageDisplay), "scale", 1.0/pow(1.5, -priv->zoomLevel), NULL);
    else
        g_object_set(G_OBJECT(priv->imageDisplay), "scale", 0.0, NULL);
    do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_normal(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    g_object_set(G_OBJECT(priv->imageDisplay), "autoscale", FALSE, NULL);
    g_object_set(G_OBJECT(priv->imageDisplay), "scale", 0.0, NULL);
    do_zoom_widget_sensitivity(manager);
}

static void capa_camera_manager_zoom_best(CapaCameraManager *manager)
{
    CapaCameraManagerPrivate *priv = manager->priv;

    priv->zoomLevel = 0;
    g_object_set(G_OBJECT(priv->imageDisplay), "autoscale", TRUE, NULL);
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
    g_signal_emit_by_name(manager, "manager-disconnect", NULL);
}


static gboolean do_manager_delete(GtkWidget *widget G_GNUC_UNUSED,
                                  GdkEvent *event G_GNUC_UNUSED,
                                  CapaCameraManager *manager)
{
    CAPA_DEBUG("Got delete");
    g_signal_emit_by_name(manager, "manager-disconnect", NULL);
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
        do_load_image(manager, img);
        g_object_unref(G_OBJECT(img));
    }
}


static void do_polaroid_remove(gpointer data)
{
    CapaImagePolaroid *pol = data;

    capa_image_polaroid_hide(pol);
    g_object_unref(G_OBJECT(pol));
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
                g_object_set(G_OBJECT(pol), "image", img, NULL);
                g_object_unref(G_OBJECT(img));
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

    if (access("./capa.glade", R_OK) == 0)
        priv->glade = glade_xml_new("capa.glade", "camera-manager", "capa");
    else
        priv->glade = glade_xml_new(PKGDATADIR "/capa.glade", "camera-manager", "capa");

    glade_xml_signal_connect_data(priv->glade, "camera_menu_help_summary", G_CALLBACK(do_manager_help_summary), manager);
    glade_xml_signal_connect_data(priv->glade, "camera_menu_help_manual", G_CALLBACK(do_manager_help_manual), manager);
    glade_xml_signal_connect_data(priv->glade, "camera_menu_help_driver", G_CALLBACK(do_manager_help_driver), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_new_click", G_CALLBACK(do_toolbar_new_session), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_open_click", G_CALLBACK(do_toolbar_open_session), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_capture_click", G_CALLBACK(do_toolbar_capture), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_preview_click", G_CALLBACK(do_toolbar_preview), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_monitor_click", G_CALLBACK(do_toolbar_monitor), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_in_click", G_CALLBACK(do_toolbar_zoom_in), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_out_click", G_CALLBACK(do_toolbar_zoom_out), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_best_click", G_CALLBACK(do_toolbar_zoom_best), manager);
    glade_xml_signal_connect_data(priv->glade, "toolbar_zoom_normal_click", G_CALLBACK(do_toolbar_zoom_normal), manager);

    glade_xml_signal_connect_data(priv->glade, "toolbar_fullscreen_toggle", G_CALLBACK(do_toolbar_fullscreen), manager);

    glade_xml_signal_connect_data(priv->glade, "menu_new_session_activate", G_CALLBACK(do_menu_new_session), manager);
    glade_xml_signal_connect_data(priv->glade, "menu_open_session_activate", G_CALLBACK(do_menu_open_session), manager);

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

    win = glade_xml_get_widget(priv->glade, "camera-manager");
    g_signal_connect(win, "delete-event", G_CALLBACK(do_manager_delete), manager);

    priv->progress = capa_camera_progress_new();

    viewport = glade_xml_get_widget(priv->glade, "image-viewport");

    priv->imageDisplay = capa_image_display_new();
    priv->sessionBrowser = capa_session_browser_new();
    priv->controlPanel = capa_control_panel_new();

    g_signal_connect(G_OBJECT(priv->sessionBrowser), "selection-changed",
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


    g_signal_connect(G_OBJECT(priv->sessionBrowser), "drag-failed", G_CALLBACK(do_drag_failed), manager);

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

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
