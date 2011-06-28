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

#include "entangle-debug.h"
#include "entangle-session-browser.h"

#define ENTANGLE_SESSION_BROWSER_GET_PRIVATE(obj)                           \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_SESSION_BROWSER, EntangleSessionBrowserPrivate))

struct _EntangleSessionBrowserPrivate {
    EntangleSession *session;
    EntangleThumbnailLoader *loader;

    gulong sigImageAdded;
    gulong sigThumbReady;

    GdkPixbuf *blank;

    GtkListStore *model;
};

G_DEFINE_TYPE(EntangleSessionBrowser, entangle_session_browser, GTK_TYPE_ICON_VIEW);

enum {
    PROP_O,
    PROP_SESSION,
    PROP_LOADER,
};

enum {
    FIELD_IMAGE,
    FIELD_PIXMAP,
    FIELD_LASTMOD,
    FIELD_NAME,

    FIELD_LAST,
};

static void do_thumb_loaded(EntanglePixbufLoader *loader,
                            EntangleImage *image,
                            gpointer data)
{
    EntangleSessionBrowser *browser = data;
    EntangleSessionBrowserPrivate *priv = browser->priv;
    GdkPixbuf *pixbuf;
    GtkTreeIter iter;

    ENTANGLE_DEBUG("Got pixbuf update on %p", image);

    pixbuf = entangle_pixbuf_loader_get_pixbuf(loader, image);
    if (!pixbuf)
        return;

    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->model), &iter))
        return;

    do {
        EntangleImage *thisimage;
        gtk_tree_model_get(GTK_TREE_MODEL(priv->model), &iter, FIELD_IMAGE, &thisimage, -1);

        if (image == thisimage) {
            gtk_list_store_set(priv->model, &iter, FIELD_PIXMAP, pixbuf, -1);
            break;
        }

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->model), &iter));
}

static void do_image_added(EntangleSession *session G_GNUC_UNUSED,
                           EntangleImage *img,
                           gpointer data)
{
    EntangleSessionBrowser *browser = data;
    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkTreeIter iter;
    GtkTreePath *path = NULL;
    int mod = entangle_image_get_last_modified(img);
    gchar *name = g_path_get_basename(entangle_image_get_filename(img));

    ENTANGLE_DEBUG("Request image %s for new image", entangle_image_get_filename(img));
    entangle_pixbuf_loader_load(ENTANGLE_PIXBUF_LOADER(priv->loader), img);

    gtk_list_store_append(priv->model, &iter);

    /* XXX what's our refcount policy going to be for pixbuf.... */
    gtk_list_store_set(priv->model, &iter,
                       FIELD_IMAGE, img,
                       FIELD_PIXMAP, priv->blank,
                       FIELD_LASTMOD, mod,
                       FIELD_NAME, name,
                       -1);
    ENTANGLE_DEBUG("ADD IMAGE EXTRA %p", img);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->model), &iter);

    gtk_icon_view_select_path(GTK_ICON_VIEW(browser), path);
    gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(browser), path, FALSE, 0, 0);

    gtk_tree_path_free(path);

    gtk_widget_queue_resize(GTK_WIDGET(browser));
}



static void do_model_unload(EntangleSessionBrowser *browser)
{
    EntangleSessionBrowserPrivate *priv = browser->priv;
    int count;

    ENTANGLE_DEBUG("Unload model");

    g_signal_handler_disconnect(priv->session,
                                priv->sigImageAdded);
    g_signal_handler_disconnect(priv->loader,
                                priv->sigThumbReady);

    count = entangle_session_image_count(priv->session);
    for (int i = 0 ; i < count ; i++) {
        EntangleImage *img = entangle_session_image_get(priv->session, i);
        entangle_pixbuf_loader_unload(ENTANGLE_PIXBUF_LOADER(priv->loader), img);
    }

    g_object_unref(priv->blank);
    gtk_list_store_clear(priv->model);
}

static void do_model_load(EntangleSessionBrowser *browser)
{
    EntangleSessionBrowserPrivate *priv = browser->priv;
    int count;
    int width;
    int height;

    ENTANGLE_DEBUG("Load model");

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

    count = entangle_session_image_count(priv->session);
    for (int i = 0 ; i < count ; i++) {
        EntangleImage *img = entangle_session_image_get(priv->session, i);
        int mod = entangle_image_get_last_modified(img);
        GtkTreeIter iter;
        gchar *name = g_path_get_basename(entangle_image_get_filename(img));

        gtk_list_store_append(priv->model, &iter);
        ENTANGLE_DEBUG("ADD IMAGE FIRST %p", img);
        /* XXX what's our refcount policy going to be for pixbuf.... */
        gtk_list_store_set(priv->model, &iter,
                           FIELD_IMAGE, img,
                           FIELD_PIXMAP, priv->blank,
                           FIELD_LASTMOD, mod,
                           FIELD_NAME, name,
                           -1);

        entangle_pixbuf_loader_load(ENTANGLE_PIXBUF_LOADER(priv->loader), img);
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

    gtk_tree_model_get(model, a, FIELD_LASTMOD, &ai, -1);
    gtk_tree_model_get(model, b, FIELD_LASTMOD, &bi, -1);

    return ai - bi;
}



static void entangle_session_browser_get_property(GObject *object,
                                              guint prop_id,
                                              GValue *value,
                                              GParamSpec *pspec)
{
    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(object);
    EntangleSessionBrowserPrivate *priv = browser->priv;

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

static void entangle_session_browser_set_property(GObject *object,
                                              guint prop_id,
                                              const GValue *value,
                                              GParamSpec *pspec)
{
    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(object);

    ENTANGLE_DEBUG("Set prop on session browser %d", prop_id);

    switch (prop_id)
        {
        case PROP_SESSION:
            entangle_session_browser_set_session(browser, g_value_get_object(value));
            break;

        case PROP_LOADER:
            entangle_session_browser_set_thumbnail_loader(browser, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_session_browser_finalize (GObject *object)
{
    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(object);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    if (priv->session && priv->loader)
        do_model_unload(browser);

    if (priv->session)
        g_object_unref(priv->session);
    if (priv->loader)
        g_object_unref(priv->loader);

    G_OBJECT_CLASS (entangle_session_browser_parent_class)->finalize (object);
}


static void entangle_session_browser_class_init(EntangleSessionBrowserClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = entangle_session_browser_finalize;
    object_class->get_property = entangle_session_browser_get_property;
    object_class->set_property = entangle_session_browser_set_property;

    g_object_class_install_property(object_class,
                                    PROP_SESSION,
                                    g_param_spec_object("session",
                                                        "Session",
                                                        "Session to be displayed",
                                                        ENTANGLE_TYPE_SESSION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_LOADER,
                                    g_param_spec_object("thumbnail-loader",
                                                        "Thumbnail loader",
                                                        "Thumbnail loader",
                                                        ENTANGLE_TYPE_THUMBNAIL_LOADER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleSessionBrowserPrivate));
}

EntangleSessionBrowser *entangle_session_browser_new(void)
{
    return ENTANGLE_SESSION_BROWSER(g_object_new(ENTANGLE_TYPE_SESSION_BROWSER, NULL));
}


static void entangle_session_browser_init(EntangleSessionBrowser *browser)
{
    EntangleSessionBrowserPrivate *priv;
    const GtkTargetEntry const targets[] = {
        { g_strdup("demo"), GTK_TARGET_OTHER_APP, 1 },
    };
    int ntargets = 1;

    priv = browser->priv = ENTANGLE_SESSION_BROWSER_GET_PRIVATE(browser);
    memset(priv, 0, sizeof *priv);

    priv->model = gtk_list_store_new(FIELD_LAST, ENTANGLE_TYPE_IMAGE, GDK_TYPE_PIXBUF, G_TYPE_INT, G_TYPE_STRING);

    gtk_icon_view_set_text_column(GTK_ICON_VIEW(browser), FIELD_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(browser), FIELD_PIXMAP);
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

    gtk_icon_view_set_item_orientation(GTK_ICON_VIEW(browser), GTK_ORIENTATION_VERTICAL);
    /* XXX gross hack - GtkIconView doesn't seem to have a better
     * way to force everything into a single row. Perhaps we should
     * just right a new widget for our needs */
    gtk_icon_view_set_columns(GTK_ICON_VIEW(browser), 10000);
}


EntangleImage *entangle_session_browser_selected_image(EntangleSessionBrowser *browser)
{
    EntangleSessionBrowserPrivate *priv = browser->priv;
    GList *items;
    EntangleImage *img = NULL;
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


void entangle_session_browser_set_thumbnail_loader(EntangleSessionBrowser *browser,
                                               EntangleThumbnailLoader *loader)
{
    EntangleSessionBrowserPrivate *priv = browser->priv;

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


EntangleThumbnailLoader *entangle_session_browser_get_thumbnail_loader(EntangleSessionBrowser *browser)
{
    EntangleSessionBrowserPrivate *priv = browser->priv;

    return priv->loader;
}


void entangle_session_browser_set_session(EntangleSessionBrowser *browser,
                                      EntangleSession *session)
{
    EntangleSessionBrowserPrivate *priv = browser->priv;

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


EntangleSession *entangle_session_browser_get_session(EntangleSessionBrowser *browser)
{
    EntangleSessionBrowserPrivate *priv = browser->priv;

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
