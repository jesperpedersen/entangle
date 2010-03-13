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
#include "capa-session-browser.h"

#define CAPA_SESSION_BROWSER_GET_PRIVATE(obj)                           \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_SESSION_BROWSER, CapaSessionBrowserPrivate))

struct _CapaSessionBrowserPrivate {
    CapaSession *session;
    CapaThumbnailLoader *loader;

    gulong sigImageAdded;
    gulong sigThumbReady;

    GdkPixbuf *blank;

    GtkListStore *model;
};

G_DEFINE_TYPE(CapaSessionBrowser, capa_session_browser, GTK_TYPE_ICON_VIEW);

enum {
    PROP_O,
    PROP_SESSION,
    PROP_LOADER,
};


static void do_thumb_loaded(CapaPixbufLoader *loader,
                            const char *filename,
                            gpointer data)
{
    CapaSessionBrowser *browser = data;
    CapaSessionBrowserPrivate *priv = browser->priv;
    GdkPixbuf *pixbuf = capa_pixbuf_loader_get_pixbuf(loader, filename);
    GtkTreeIter iter;

    CAPA_DEBUG("Got pixbuf update on %s", filename);

    if (!pixbuf)
        return;

    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->model), &iter))
        return;

    do {
        CapaImage *img;
        gtk_tree_model_get(GTK_TREE_MODEL(priv->model), &iter, 0, &img, -1);

        if (strcmp(capa_image_filename(img), filename) == 0) {
            gtk_list_store_set(priv->model, &iter, 1, pixbuf, -1);
            break;
        }

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->model), &iter));
}

static void do_image_added(CapaSession *session G_GNUC_UNUSED,
                           CapaImage *img,
                           gpointer data)
{
    CapaSessionBrowser *browser = data;
    CapaSessionBrowserPrivate *priv = browser->priv;
    GtkTreeIter iter;
    GtkTreePath *path = NULL;
    int mod = capa_image_last_modified(img);

    CAPA_DEBUG("Request image %s for new image", capa_image_filename(img));
    capa_pixbuf_loader_load(CAPA_PIXBUF_LOADER(priv->loader),
                            capa_image_filename(img));

    gtk_list_store_append(priv->model, &iter);

    /* XXX what's our refcount policy going to be for pixbuf.... */
    gtk_list_store_set(priv->model, &iter, 0, img, 1, priv->blank, 2, mod, -1);
    CAPA_DEBUG("ADD IMAGE EXTRA %p", img);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->model), &iter);

    gtk_icon_view_select_path(GTK_ICON_VIEW(browser), path);
    gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(browser), path, FALSE, 0, 0);

    gtk_tree_path_free(path);

    gtk_widget_queue_resize(GTK_WIDGET(browser));
}



static void do_model_unload(CapaSessionBrowser *browser)
{
    CapaSessionBrowserPrivate *priv = browser->priv;
    int count;

    CAPA_DEBUG("Unload model");

    g_signal_handler_disconnect(priv->session,
                                priv->sigImageAdded);
    g_signal_handler_disconnect(priv->loader,
                                priv->sigThumbReady);

    count = capa_session_image_count(priv->session);
    for (int i = 0 ; i < count ; i++) {
        CapaImage *img = capa_session_image_get(priv->session, i);
        capa_pixbuf_loader_unload(CAPA_PIXBUF_LOADER(priv->loader),
                                  capa_image_filename(img));
    }

    g_object_unref(priv->blank);
    gtk_list_store_clear(priv->model);
}

static void do_model_load(CapaSessionBrowser *browser)
{
    CapaSessionBrowserPrivate *priv = browser->priv;
    int count;
    int width;
    int height;

    CAPA_DEBUG("Load model");

    g_object_get(priv->loader,
                 "width", &width,
                 "height", &height,
                 NULL);

    priv->blank = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    gdk_pixbuf_fill(priv->blank, 0x000000FF);

    priv->sigImageAdded = g_signal_connect(priv->session, "session-image-added",
                                           G_CALLBACK(do_image_added), browser);
    priv->sigThumbReady = g_signal_connect(priv->loader, "pixbuf-loaded",
                                           G_CALLBACK(do_thumb_loaded), browser);

    count = capa_session_image_count(priv->session);
    for (int i = 0 ; i < count ; i++) {
        CapaImage *img = capa_session_image_get(priv->session, i);
        int mod = capa_image_last_modified(img);
        GtkTreeIter iter;

        gtk_list_store_append(priv->model, &iter);
        CAPA_DEBUG("ADD IMAGE FIRST %p", img);
        /* XXX what's our refcount policy going to be for pixbuf.... */
        gtk_list_store_set(priv->model, &iter, 0, img, 1, priv->blank, 2, mod, -1);

        capa_pixbuf_loader_load(CAPA_PIXBUF_LOADER(priv->loader),
                                capa_image_filename(img));
        //g_object_unref(cam);
    }

    if (count) {
        GtkTreePath *path = NULL;
        path = gtk_tree_path_new_from_indices(count - 1, -1);

        gtk_icon_view_select_path(GTK_ICON_VIEW(browser), path);
        gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(browser), path, FALSE, 0, 0);

        gtk_tree_path_free(path);
    }
}


static gint
do_image_sort_modified(GtkTreeModel *model,
                       GtkTreeIter  *a,
                       GtkTreeIter  *b,
                       gpointer data G_GNUC_UNUSED)
{
    gint ai, bi;

    gtk_tree_model_get(model, a, 2, &ai, -1);
    gtk_tree_model_get(model, b, 2, &bi, -1);

    return ai - bi;
}



static void capa_session_browser_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    CapaSessionBrowser *browser = CAPA_SESSION_BROWSER(object);
    CapaSessionBrowserPrivate *priv = browser->priv;

    switch (prop_id)
        {
        case PROP_SESSION:
            g_value_set_object(value, priv->session);
            break;

        case PROP_LOADER:
            g_value_set_object(value, priv->loader);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_session_browser_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    CapaSessionBrowser *browser = CAPA_SESSION_BROWSER(object);

    CAPA_DEBUG("Set prop on session browser %d", prop_id);

    switch (prop_id)
        {
        case PROP_SESSION:
            capa_session_browser_set_session(browser, g_value_get_object(value));
            break;

        case PROP_LOADER:
            capa_session_browser_set_thumbnail_loader(browser, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_session_browser_finalize (GObject *object)
{
    CapaSessionBrowser *browser = CAPA_SESSION_BROWSER(object);
    CapaSessionBrowserPrivate *priv = browser->priv;

    if (priv->session && priv->loader)
        do_model_unload(browser);

    if (priv->session)
        g_object_unref(priv->session);
    if (priv->loader)
        g_object_unref(priv->loader);

    G_OBJECT_CLASS (capa_session_browser_parent_class)->finalize (object);
}


static void capa_session_browser_class_init(CapaSessionBrowserClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = capa_session_browser_finalize;
    object_class->get_property = capa_session_browser_get_property;
    object_class->set_property = capa_session_browser_set_property;

    g_object_class_install_property(object_class,
                                    PROP_SESSION,
                                    g_param_spec_object("session",
                                                        "Session",
                                                        "Session to be displayed",
                                                        CAPA_TYPE_SESSION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_LOADER,
                                    g_param_spec_object("thumbnail-loader",
                                                        "Thumbnail loader",
                                                        "Thumbnail loader",
                                                        CAPA_TYPE_THUMBNAIL_LOADER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(CapaSessionBrowserPrivate));
}

CapaSessionBrowser *capa_session_browser_new(void)
{
    return CAPA_SESSION_BROWSER(g_object_new(CAPA_TYPE_SESSION_BROWSER, NULL));
}


static void capa_session_browser_init(CapaSessionBrowser *browser)
{
    CapaSessionBrowserPrivate *priv;
    const GtkTargetEntry const targets[] = {
        { g_strdup("demo"), GTK_TARGET_OTHER_APP, 1 },
    };
    int ntargets = 1;

    priv = browser->priv = CAPA_SESSION_BROWSER_GET_PRIVATE(browser);
    memset(priv, 0, sizeof *priv);

    priv->model = gtk_list_store_new(3, CAPA_TYPE_IMAGE, GDK_TYPE_PIXBUF, G_TYPE_INT);

    gtk_icon_view_set_text_column(GTK_ICON_VIEW(browser), -1);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(browser), 1);
    gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(browser), GTK_SELECTION_SINGLE);
    gtk_icon_view_set_model(GTK_ICON_VIEW(browser), GTK_TREE_MODEL(priv->model));

    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(priv->model),
                                            do_image_sort_modified, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->model),
                                         GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                         GTK_SORT_ASCENDING);

    gtk_icon_view_enable_model_drag_source(GTK_ICON_VIEW(browser),
                                           GDK_BUTTON1_MASK,
                                           targets,
                                           ntargets,
                                           GDK_ACTION_PRIVATE);

    gtk_icon_view_set_orientation(GTK_ICON_VIEW(browser), GTK_ORIENTATION_HORIZONTAL);
    /* XXX gross hack - GtkIconView doesn't seem to have a better
     * way to force everything into a single row. Perhaps we should
     * just right a new widget for our needs */
    gtk_icon_view_set_columns(GTK_ICON_VIEW(browser), 1000);
}


CapaImage *capa_session_browser_selected_image(CapaSessionBrowser *browser)
{
    CapaSessionBrowserPrivate *priv = browser->priv;
    GList *items;
    CapaImage *img = NULL;
    GtkTreePath *path;
    GtkTreeIter iter;
    GValue val;

    items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(browser));

    if (!items)
        return NULL;

    path = g_list_nth_data(items, 0);
    if (!path)
        goto cleanup;

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->model), &iter, path))
        goto cleanup;

    memset(&val, 0, sizeof val);
    gtk_tree_model_get_value(GTK_TREE_MODEL(priv->model), &iter, 0, &val);

    img = g_value_get_object(&val);

 cleanup:
    g_list_foreach(items, (GFunc)(gtk_tree_path_free), NULL);
    g_list_free(items);
    return img;
}


void capa_session_browser_set_thumbnail_loader(CapaSessionBrowser *browser,
                                               CapaThumbnailLoader *loader)
{
    CapaSessionBrowserPrivate *priv = browser->priv;

    if (priv->loader) {
        if (priv->session)
            do_model_unload(browser);

        g_object_unref(priv->loader);
    }
    priv->loader = loader;
    if (priv->loader) {
        g_object_ref(priv->loader);

        if (priv->session)
            do_model_load(browser);
    }
}


CapaThumbnailLoader *capa_session_browser_get_thumbnail_loader(CapaSessionBrowser *browser)
{
    CapaSessionBrowserPrivate *priv = browser->priv;

    return priv->loader;
}


void capa_session_browser_set_session(CapaSessionBrowser *browser,
                                      CapaSession *session)
{
    CapaSessionBrowserPrivate *priv = browser->priv;

    if (priv->session) {
        if (priv->loader)
            do_model_unload(browser);
        g_object_unref(priv->session);
    }
    priv->session = session;
    if (priv->session) {
        g_object_ref(priv->session);

        if (priv->loader)
            do_model_load(browser);
    }
}


CapaSession *capa_session_browser_get_session(CapaSessionBrowser *browser)
{
    CapaSessionBrowserPrivate *priv = browser->priv;

    return priv->session;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */