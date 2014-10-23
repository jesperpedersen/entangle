/*
 *  Entangle: Tethered Camera Control & Capture
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

#include "entangle-debug.h"
#include "entangle-session-browser.h"

#define ENTANGLE_SESSION_BROWSER_GET_PRIVATE(obj)                       \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_SESSION_BROWSER, EntangleSessionBrowserPrivate))

typedef struct _EntangleSessionBrowserItem EntangleSessionBrowserItem;
struct _EntangleSessionBrowserItem
{
    /* First member is always the rectangle so it
     * can be cast to a rectangle. */
    GdkRectangle cell_area;

    GtkTreeIter iter;
    gint idx;

    gint col;

    guint selected : 1;
    guint selected_before_rubberbanding : 1;
};

struct _EntangleSessionBrowserPrivate {
    EntangleSession *session;
    EntangleThumbnailLoader *loader;

    GdkRGBA background;
    GdkRGBA highlight;

    GtkCellArea *cell_area;
    GtkCellAreaContext *cell_area_context;

    GtkCellRenderer *pixbuf_cell;

    gulong sigImageAdded;
    gulong sigThumbReady;
    gulong context_changed_id;

    GdkPixbuf *blank;

    GtkTreeModel *model;
    EntangleImage *selected;

    GList *items;

    GtkAdjustment *hadjustment;
    GtkAdjustment *vadjustment;

    GtkTreeRowReference *scroll_to_path;
    gfloat scroll_to_row_align;
    gfloat scroll_to_col_align;
    guint scroll_to_use_align : 1;

    /* GtkScrollablePolicy needs to be checked when
     * driving the scrollable adjustment values */
    guint hscroll_policy : 1;
    guint vscroll_policy : 1;

    guint width;
    guint height;

    GdkWindow *bin_window;

    gint margin;
    gint item_padding;
    gint column_spacing;

    gint dnd_start_x;
    gint dnd_start_y;
};


static void
entangle_session_browser_adjustment_changed(GtkAdjustment *adjustment,
                                            EntangleSessionBrowser *browser);

static void
entangle_session_browser_set_hadjustment(EntangleSessionBrowser *browser,
                                         GtkAdjustment *adjustment);
static void
entangle_session_browser_set_vadjustment(EntangleSessionBrowser *browser,
                                         GtkAdjustment *adjustment);

static void
entangle_session_browser_set_hadjustment_values(EntangleSessionBrowser *browser);
static void
entangle_session_browser_set_vadjustment_values(EntangleSessionBrowser *browser);

static gboolean
entangle_session_browser_draw(GtkWidget *widget,
                              cairo_t *cr);

static void
entangle_session_browser_cell_layout_init(GtkCellLayoutIface *iface);
static void
entangle_session_browser_select_path(EntangleSessionBrowser *browser,
                                     GtkTreePath *path);

static void
entangle_session_browser_scroll_to_path(EntangleSessionBrowser *browser,
                                        GtkTreePath *path,
                                        gboolean     use_align,
                                        gfloat       row_align,
                                        gfloat       col_align);

static void
entangle_session_browser_layout(EntangleSessionBrowser *browser);


G_DEFINE_TYPE_WITH_CODE(EntangleSessionBrowser, entangle_session_browser, GTK_TYPE_DRAWING_AREA,
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_CELL_LAYOUT,
                                              entangle_session_browser_cell_layout_init)
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_SCROLLABLE, NULL))

enum {
    PROP_O,
    PROP_SESSION,
    PROP_LOADER,
    PROP_HADJUSTMENT,
    PROP_VADJUSTMENT,
    PROP_HSCROLL_POLICY,
    PROP_VSCROLL_POLICY,
};

enum {
    FIELD_IMAGE,
    FIELD_PIXMAP,
    FIELD_LASTMOD,
    FIELD_NAME,

    FIELD_LAST,
};

enum {
    SIGNAL_SELECTION_CHANGED,

    SIGNAL_LAST,
};

static guint browser_signals[SIGNAL_LAST] = { 0 };


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

    if (!gtk_tree_model_get_iter_first(priv->model, &iter))
        return;

    do {
        EntangleImage *thisimage;
        gtk_tree_model_get(priv->model, &iter, FIELD_IMAGE, &thisimage, -1);

        if (image == thisimage) {
            g_object_unref(thisimage);
            gtk_list_store_set(GTK_LIST_STORE(priv->model),
                               &iter, FIELD_PIXMAP, pixbuf, -1);
            break;
        }
        g_object_unref(thisimage);
    } while (gtk_tree_model_iter_next(priv->model, &iter));
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

    gtk_list_store_append(GTK_LIST_STORE(priv->model), &iter);

    gtk_list_store_set(GTK_LIST_STORE(priv->model),
                       &iter,
                       FIELD_IMAGE, img,
                       FIELD_PIXMAP, priv->blank,
                       FIELD_LASTMOD, mod,
                       FIELD_NAME, name,
                       -1);
    ENTANGLE_DEBUG("ADD IMAGE EXTRA %p", img);
    path = gtk_tree_model_get_path(priv->model, &iter);

    entangle_session_browser_select_path(browser, path);
    entangle_session_browser_scroll_to_path(browser, path, FALSE, 0, 0);

    gtk_tree_path_free(path);

    gtk_widget_queue_resize(GTK_WIDGET(browser));
}



static void do_image_removed(EntangleSession *session G_GNUC_UNUSED,
                             EntangleImage *img,
                             gpointer data)
{
    EntangleSessionBrowser *browser = data;
    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkTreeIter iter;

    ENTANGLE_DEBUG("Unrequest image %s for new image", entangle_image_get_filename(img));
    entangle_pixbuf_loader_unload(ENTANGLE_PIXBUF_LOADER(priv->loader), img);

    if (!gtk_tree_model_get_iter_first(priv->model, &iter))
        return;

    do {
        EntangleImage *thisimg = NULL;
        GValue value;
        memset(&value, 0, sizeof(value));
        gtk_tree_model_get_value(priv->model, &iter, FIELD_IMAGE, &value);
        thisimg = g_value_get_object(&value);
        if (thisimg == img) {
            gtk_list_store_remove(GTK_LIST_STORE(priv->model), &iter);
            break;
        }
    } while (gtk_tree_model_iter_next(priv->model, &iter));

    gtk_widget_queue_resize(GTK_WIDGET(browser));
}



static void do_model_unload(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    int count;

    ENTANGLE_DEBUG("Unload model");

    g_signal_handler_disconnect(priv->session,
                                priv->sigImageAdded);
    g_signal_handler_disconnect(priv->loader,
                                priv->sigThumbReady);

    count = entangle_session_image_count(priv->session);
    for (int i = 0; i < count; i++) {
        EntangleImage *img = entangle_session_image_get(priv->session, i);
        entangle_pixbuf_loader_unload(ENTANGLE_PIXBUF_LOADER(priv->loader), img);
    }

    g_object_unref(priv->blank);
    gtk_list_store_clear(GTK_LIST_STORE(priv->model));
}

static void do_model_load(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

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
    priv->sigImageAdded = g_signal_connect(priv->session, "session-image-removed",
                                           G_CALLBACK(do_image_removed), browser);
    priv->sigThumbReady = g_signal_connect(priv->loader, "pixbuf-loaded",
                                           G_CALLBACK(do_thumb_loaded), browser);

    count = entangle_session_image_count(priv->session);
    for (int i = 0; i < count; i++) {
        EntangleImage *img = entangle_session_image_get(priv->session, i);
        int mod = entangle_image_get_last_modified(img);
        GtkTreeIter iter;
        gchar *name = g_path_get_basename(entangle_image_get_filename(img));

        gtk_list_store_append(GTK_LIST_STORE(priv->model), &iter);
        ENTANGLE_DEBUG("ADD IMAGE FIRST %p", img);
        gtk_list_store_set(GTK_LIST_STORE(priv->model), &iter,
                           FIELD_IMAGE, img,
                           FIELD_PIXMAP, priv->blank,
                           FIELD_LASTMOD, mod,
                           FIELD_NAME, name,
                           -1);

        entangle_pixbuf_loader_load(ENTANGLE_PIXBUF_LOADER(priv->loader), img);
    }

    if (count) {
        GtkTreePath *path = NULL;
        path = gtk_tree_path_new_from_indices(count - 1, -1);

        entangle_session_browser_select_path(ENTANGLE_SESSION_BROWSER(browser), path);
        entangle_session_browser_scroll_to_path(ENTANGLE_SESSION_BROWSER(browser), path, FALSE, 0, 0);

        gtk_tree_path_free(path);
    }
}


static gint
do_image_sort_name(GtkTreeModel *model,
                   GtkTreeIter  *a,
                   GtkTreeIter  *b,
                   gpointer data G_GNUC_UNUSED)
{
    gchar *ai, *bi;

    gtk_tree_model_get(model, a, FIELD_NAME, &ai, -1);
    gtk_tree_model_get(model, b, FIELD_NAME, &bi, -1);

    return strcmp(ai, bi);
}


static void entangle_session_browser_get_property(GObject *object,
                                                  guint prop_id,
                                                  GValue *value,
                                                  GParamSpec *pspec)
{
    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(object);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    switch (prop_id) {
    case PROP_SESSION:
        g_value_set_object(value, priv->session);
        break;

    case PROP_LOADER:
        g_value_set_object(value, priv->loader);
        break;

    case PROP_HADJUSTMENT:
        g_value_set_object(value, priv->hadjustment);
        break;

    case PROP_VADJUSTMENT:
        g_value_set_object(value, priv->vadjustment);
        break;

    case PROP_HSCROLL_POLICY:
        g_value_set_enum(value, priv->hscroll_policy);
        break;

    case PROP_VSCROLL_POLICY:
        g_value_set_enum(value, priv->vscroll_policy);
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
    EntangleSessionBrowserPrivate *priv = browser->priv;

    ENTANGLE_DEBUG("Set prop on session browser %d", prop_id);

    switch (prop_id) {
    case PROP_SESSION:
        entangle_session_browser_set_session(browser, g_value_get_object(value));
        break;

    case PROP_LOADER:
        entangle_session_browser_set_thumbnail_loader(browser, g_value_get_object(value));
        break;

    case PROP_HADJUSTMENT:
        entangle_session_browser_set_hadjustment(browser, g_value_get_object(value));
        break;

    case PROP_VADJUSTMENT:
        entangle_session_browser_set_vadjustment(browser, g_value_get_object(value));
        break;

    case PROP_HSCROLL_POLICY:
        priv->hscroll_policy = g_value_get_enum(value);
        gtk_widget_queue_resize(GTK_WIDGET(browser));
        break;

    case PROP_VSCROLL_POLICY:
        priv->vscroll_policy = g_value_get_enum(value);
        gtk_widget_queue_resize(GTK_WIDGET(browser));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void
verify_items(EntangleSessionBrowser *browser)
{
    GList *items;
    int i = 0;

    for (items = browser->priv->items; items; items = items->next) {
        EntangleSessionBrowserItem *item = items->data;

        if (item->idx != i)
            ENTANGLE_DEBUG("List item does not match its index: "
                           "item index %d and list index %d\n", item->idx, i);
        i++;
    }
}


static GtkCellArea *
entangle_session_browser_cell_layout_get_area(GtkCellLayout *cell_layout)
{
    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(cell_layout);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    return priv->cell_area;
}


static void
entangle_session_browser_set_cell_data(EntangleSessionBrowser *browser,
                                       EntangleSessionBrowserItem *item)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    gboolean iters_persist;
    GtkTreeIter iter;

    iters_persist = gtk_tree_model_get_flags(browser->priv->model) & GTK_TREE_MODEL_ITERS_PERSIST;

    if (!iters_persist) {
        GtkTreePath *path;

        path = gtk_tree_path_new_from_indices(item->idx, -1);
        if (!gtk_tree_model_get_iter(browser->priv->model, &iter, path))
            return;
        gtk_tree_path_free(path);
    } else {
        iter = item->iter;
    }

    gtk_cell_area_apply_attributes(browser->priv->cell_area,
                                   browser->priv->model,
                                   &iter, FALSE, FALSE);
}


/* This ensures that all widths have been cached in the
 * context and we have proper alignments to go on.
 */
static void
entangle_session_browser_cache_widths(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    GList *items;

    g_signal_handler_block(browser->priv->cell_area_context,
                           browser->priv->context_changed_id);

    for (items = browser->priv->items; items; items = items->next) {
        EntangleSessionBrowserItem *item = items->data;

        /* Only fetch the width of items with invalidated sizes */
        if (item->cell_area.width < 0) {
            entangle_session_browser_set_cell_data(browser, item);
            gtk_cell_area_get_preferred_width(browser->priv->cell_area,
                                              browser->priv->cell_area_context,
                                              GTK_WIDGET(browser), NULL, NULL);
        }
    }

    g_signal_handler_unblock(browser->priv->cell_area_context,
                             browser->priv->context_changed_id);
}


static void
entangle_session_browser_item_invalidate_size(EntangleSessionBrowserItem *item)
{
    item->cell_area.width = -1;
    item->cell_area.height = -1;
}


static void
entangle_session_browser_invalidate_sizes(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    /* Clear all item sizes */
    g_list_foreach(browser->priv->items,
                   (GFunc)entangle_session_browser_item_invalidate_size, NULL);

    /* Reset the context */
    if (browser->priv->cell_area_context) {
        g_signal_handler_block(browser->priv->cell_area_context,
                               browser->priv->context_changed_id);
        gtk_cell_area_context_reset(browser->priv->cell_area_context);
        g_signal_handler_unblock(browser->priv->cell_area_context,
                                 browser->priv->context_changed_id);
    }

    gtk_widget_queue_resize(GTK_WIDGET(browser));
}


static void
entangle_session_browser_context_changed(GtkCellAreaContext *context G_GNUC_UNUSED,
                                         GParamSpec *pspec,
                                         gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(data));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(data);

    if (!strcmp(pspec->name, "minimum-width") ||
        !strcmp(pspec->name, "natural-width") ||
        !strcmp(pspec->name, "minimum-height") ||
        !strcmp(pspec->name, "natural-height"))
        entangle_session_browser_invalidate_sizes(browser);
}


static void
entangle_session_browser_row_changed(GtkTreeModel *model G_GNUC_UNUSED,
                                     GtkTreePath *path,
                                     GtkTreeIter *iter G_GNUC_UNUSED,
                                     gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(data));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(data);

    /* ignore changes in branches */
    if (gtk_tree_path_get_depth(path) > 1)
        return;

    /* An icon view subclass might add it's own model and populate
     * things at init() time instead of waiting for the constructor()
     * to be called
     */
    if (browser->priv->cell_area)
        gtk_cell_area_stop_editing(browser->priv->cell_area, TRUE);

    /* Here we can use a "grow-only" strategy for optimization
     * and only invalidate a single item and queue a relayout
     * instead of invalidating the whole thing.
     *
     * For now EntangleSessionBrowser still cant deal with huge models
     * so just invalidate the whole thing when the model
     * changes.
     */
    entangle_session_browser_invalidate_sizes(browser);

    verify_items(browser);
}


static EntangleSessionBrowserItem *entangle_session_browser_item_new(void)
{
    EntangleSessionBrowserItem *item;

    item = g_slice_new0(EntangleSessionBrowserItem);

    item->cell_area.width  = -1;
    item->cell_area.height = -1;

    return item;
}


static void entangle_session_browser_item_free(EntangleSessionBrowserItem *item)
{
    g_return_if_fail(item != NULL);

    g_slice_free(EntangleSessionBrowserItem, item);
}


static void
entangle_session_browser_row_inserted(GtkTreeModel *model G_GNUC_UNUSED,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(data));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(data);
    gint idx;
    EntangleSessionBrowserItem *item;
    gboolean iters_persist;
    GList *list;

    /* ignore changes in branches */
    if (gtk_tree_path_get_depth(path) > 1)
        return;

    iters_persist = gtk_tree_model_get_flags(browser->priv->model) & GTK_TREE_MODEL_ITERS_PERSIST;

    idx = gtk_tree_path_get_indices(path)[0];

    item = entangle_session_browser_item_new();

    if (iters_persist)
        item->iter = *iter;

    item->idx = idx;

    /* FIXME: We can be more efficient here,
       we can store a tail pointer and use that when
       appending (which is a rather common operation)
    */
    browser->priv->items = g_list_insert(browser->priv->items,
                                         item, idx);

    list = g_list_nth(browser->priv->items, idx + 1);
    for (; list; list = list->next) {
        item = list->data;
        item->idx++;
    }

    verify_items(browser);

    gtk_widget_queue_resize(GTK_WIDGET(browser));
}


static void
entangle_session_browser_row_deleted(GtkTreeModel *model G_GNUC_UNUSED,
                                     GtkTreePath *path,
                                     gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(data));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(data);
    gint idx;
    EntangleSessionBrowserItem *item;
    GList *list, *next;
    gboolean emit = FALSE;

    /* ignore changes in branches */
    if (gtk_tree_path_get_depth(path) > 1)
        return;

    idx = gtk_tree_path_get_indices(path)[0];

    list = g_list_nth(browser->priv->items, idx);
    item = list->data;

    if (browser->priv->cell_area)
        gtk_cell_area_stop_editing(browser->priv->cell_area, TRUE);

    if (item->selected)
        emit = TRUE;

    entangle_session_browser_item_free(item);

    for (next = list->next; next; next = next->next) {
        item = next->data;
        item->idx--;
    }

    browser->priv->items = g_list_delete_link(browser->priv->items, list);

    verify_items(browser);

    gtk_widget_queue_resize(GTK_WIDGET(browser));

    if (emit)
        g_signal_emit(browser, browser_signals[SIGNAL_SELECTION_CHANGED], 0);
}

static void
entangle_session_browser_rows_reordered(GtkTreeModel *model,
                                        GtkTreePath *parent G_GNUC_UNUSED,
                                        GtkTreeIter *iter,
                                        gint *new_order,
                                        gpointer data)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(data));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(data);
    int i;
    int length;
    GList *items = NULL, *list;
    EntangleSessionBrowserItem **item_array;
    gint *order;

    /* ignore changes in branches */
    if (iter != NULL)
        return;

    if (browser->priv->cell_area)
        gtk_cell_area_stop_editing(browser->priv->cell_area, TRUE);

    length = gtk_tree_model_iter_n_children(model, NULL);

    order = g_new(gint, length);
    for (i = 0; i < length; i++)
        order[new_order[i]] = i;

    item_array = g_new(EntangleSessionBrowserItem *, length);
    for (i = 0, list = browser->priv->items; list != NULL; list = list->next, i++)
        item_array[order[i]] = list->data;
    g_free(order);

    for (i = length - 1; i >= 0; i--) {
        item_array[i]->idx = i;
        items = g_list_prepend(items, item_array[i]);
    }

    g_free(item_array);
    g_list_free(browser->priv->items);
    browser->priv->items = items;

    gtk_widget_queue_resize(GTK_WIDGET(browser));

    verify_items(browser);
}


static void
entangle_session_browser_build_items(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    GtkTreeIter iter;
    int i;
    gboolean iters_persist;
    GList *items = NULL;

    iters_persist = gtk_tree_model_get_flags(browser->priv->model) & GTK_TREE_MODEL_ITERS_PERSIST;

    if (!gtk_tree_model_get_iter_first(browser->priv->model,
                                       &iter))
        return;

    i = 0;

    do {
        EntangleSessionBrowserItem *item = entangle_session_browser_item_new();

        if (iters_persist)
            item->iter = iter;

        item->idx = i;
        i++;
        items = g_list_prepend(items, item);
    } while (gtk_tree_model_iter_next(browser->priv->model, &iter));

    browser->priv->items = g_list_reverse(items);
}


static void
entangle_session_browser_realize(GtkWidget *widget)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkAllocation allocation;
    GdkWindow *window;
    GdkWindowAttr attributes;
    gint attributes_mask;
    GtkStyleContext *context;

    gtk_widget_set_realized(widget, TRUE);

    gtk_widget_get_allocation(widget, &allocation);

    /* Make the main, clipping window */
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    window = gdk_window_new(gtk_widget_get_parent_window(widget),
                            &attributes, attributes_mask);
    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, widget);

    gtk_widget_get_allocation(widget, &allocation);

    /* Make the window for the icon view */
    attributes.x = 0;
    attributes.y = 0;
    attributes.width = MAX(priv->width, allocation.width);
    attributes.height = MAX(priv->height, allocation.height);
    attributes.event_mask = (GDK_EXPOSURE_MASK |
                             GDK_SCROLL_MASK |
                             GDK_POINTER_MOTION_MASK |
                             GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_KEY_PRESS_MASK |
                             GDK_KEY_RELEASE_MASK) |
        gtk_widget_get_events(widget);

    priv->bin_window = gdk_window_new(window,
                                      &attributes, attributes_mask);
    gdk_window_set_user_data(priv->bin_window, widget);

    context = gtk_widget_get_style_context(widget);

    gtk_style_context_save(context);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_VIEW);
    gtk_style_context_set_background(context, priv->bin_window);
    gtk_style_context_restore(context);

    gdk_window_show(priv->bin_window);
}


static void
entangle_session_browser_unrealize(GtkWidget *widget)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    gdk_window_set_user_data(priv->bin_window, NULL);
    gdk_window_destroy(priv->bin_window);
    priv->bin_window = NULL;

    GTK_WIDGET_CLASS(entangle_session_browser_parent_class)->unrealize(widget);
}


static void
entangle_session_browser_scroll_to_item(EntangleSessionBrowser *browser,
                                        EntangleSessionBrowserItem *item)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkWidget *widget = GTK_WIDGET(browser);
    GtkAdjustment *hadj, *vadj;
    GtkAllocation allocation;
    gint x, y;
    GdkRectangle item_area;

    item_area.x = item->cell_area.x - priv->item_padding;
    item_area.y = item->cell_area.y - priv->item_padding;
    item_area.width = item->cell_area.width  + priv->item_padding * 2;
    item_area.height = item->cell_area.height + priv->item_padding * 2;

    gdk_window_get_position(priv->bin_window, &x, &y);
    gtk_widget_get_allocation(widget, &allocation);

    hadj = priv->hadjustment;
    vadj = priv->vadjustment;

    if (y + item_area.y < 0)
        gtk_adjustment_set_value(vadj,
                                 gtk_adjustment_get_value(vadj)
                                 + y + item_area.y);
    else if (y + item_area.y + item_area.height > allocation.height)
        gtk_adjustment_set_value(vadj,
                                 gtk_adjustment_get_value(vadj)
                                 + y + item_area.y + item_area.height - allocation.height);

    if (x + item_area.x < 0)
        gtk_adjustment_set_value(hadj,
                                 gtk_adjustment_get_value(hadj)
                                 + x + item_area.x);
    else if (x + item_area.x + item_area.width > allocation.width)
        gtk_adjustment_set_value(hadj,
                                 gtk_adjustment_get_value(hadj)
                                 + x + item_area.x + item_area.width - allocation.width);

    gtk_adjustment_changed(hadj);
    gtk_adjustment_changed(vadj);
}


static EntangleSessionBrowserItem *
entangle_session_browser_get_item_at_coords(EntangleSessionBrowser *browser,
                                            gint x,
                                            gint y,
                                            gboolean only_in_cell,
                                            GtkCellRenderer **cell_at_pos)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GList *items;

    if (cell_at_pos)
        *cell_at_pos = NULL;

    for (items = priv->items; items; items = items->next) {
        EntangleSessionBrowserItem *item = items->data;
        GdkRectangle *item_area = (GdkRectangle *)item;

        if (x >= item_area->x - priv->column_spacing / 2 &&
            x <= item_area->x + item_area->width + priv->column_spacing / 2 &&
            y >= item_area->y &&
            y <= item_area->y + item_area->height) {
            if (only_in_cell || cell_at_pos) {
                GtkCellRenderer *cell = NULL;

                entangle_session_browser_set_cell_data(browser, item);

                if (x >= item_area->x && x <= item_area->x + item_area->width &&
                    y >= item_area->y && y <= item_area->y + item_area->height)
                    cell = gtk_cell_area_get_cell_at_position(priv->cell_area,
                                                              priv->cell_area_context,
                                                              GTK_WIDGET(browser),
                                                              item_area,
                                                              x, y, NULL);

                if (cell_at_pos)
                    *cell_at_pos = cell;

                if (only_in_cell)
                    return cell != NULL ? item : NULL;
                else
                    return item;
            }
            return item;
        }
    }
    return NULL;
}


/**
 * entangle_session_browser_get_image_at_coords:
 * @browser: (transfer none): the session browser
 * @x: the horizontal co-ordinate
 * @y: the vertical co-ordinate
 *
 * Retrieve the image displayed at the co-ordinates (@x, @y)
 *
 * Returns: (transfer none): the image, or NULL
 */
EntangleImage *entangle_session_browser_get_image_at_coords(EntangleSessionBrowser *browser,
                                                            gint x, gint y)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;
    EntangleSessionBrowserItem *item = entangle_session_browser_get_item_at_coords(browser, x, y,
                                                                                   FALSE, NULL);
    EntangleImage *img;
    GValue val;

    if (!item)
        return NULL;

    memset(&val, 0, sizeof val);
    gtk_tree_model_get_value(GTK_TREE_MODEL(priv->model), &item->iter, 0, &val);

    img = g_value_get_object(&val);

    return img;
}


static void
entangle_session_browser_queue_draw_item(EntangleSessionBrowser *browser,
                                         EntangleSessionBrowserItem *item)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GdkRectangle  rect;
    GdkRectangle *item_area = (GdkRectangle *)item;

    rect.x      = item_area->x - priv->item_padding;
    rect.y      = item_area->y - priv->item_padding;
    rect.width  = item_area->width  + priv->item_padding * 2;
    rect.height = item_area->height + priv->item_padding * 2;

    if (priv->bin_window && item_area->width != -1 && item_area->height != -1)
        gdk_window_invalidate_rect(priv->bin_window, &rect, TRUE);
}


static gboolean
entangle_session_browser_unselect_all_internal(EntangleSessionBrowser *browser)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), FALSE);

    EntangleSessionBrowserPrivate *priv = browser->priv;
    gboolean dirty = FALSE;
    GList *items;

    for (items = priv->items; items; items = items->next) {
        EntangleSessionBrowserItem *item = items->data;

        if (item->selected) {
            item->selected = FALSE;
            dirty = TRUE;
            entangle_session_browser_queue_draw_item(browser, item);
        }
    }

    return dirty;
}


static void
entangle_session_browser_select_item(EntangleSessionBrowser *browser,
                                     EntangleSessionBrowserItem *item)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    if (item->selected)
        return;

    item->selected = TRUE;

    g_signal_emit(browser, browser_signals[SIGNAL_SELECTION_CHANGED], 0);

    entangle_session_browser_queue_draw_item(browser, item);
}


static void
entangle_session_browser_unselect_item(EntangleSessionBrowser *browser,
                                       EntangleSessionBrowserItem *item)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    if (!item->selected)
        return;

    item->selected = FALSE;

    g_signal_emit(browser, browser_signals[SIGNAL_SELECTION_CHANGED], 0);

    entangle_session_browser_queue_draw_item(browser, item);
}


static void
entangle_session_browser_select_path(EntangleSessionBrowser *browser,
                                     GtkTreePath *path)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    EntangleSessionBrowserItem *item = NULL;

    if (gtk_tree_path_get_depth(path) > 0)
        item = g_list_nth_data(priv->items,
                               gtk_tree_path_get_indices(path)[0]);

    if (item) {
        entangle_session_browser_unselect_all_internal(browser);
        entangle_session_browser_select_item(browser, item);
    }
}


static void
entangle_session_browser_scroll_to_path(EntangleSessionBrowser *browser,
                                        GtkTreePath *path,
                                        gboolean     use_align,
                                        gfloat       row_align,
                                        gfloat       col_align)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    EntangleSessionBrowserItem *item = NULL;
    GtkWidget *widget = GTK_WIDGET(browser);

    if (gtk_tree_path_get_depth(path) > 0)
        item = g_list_nth_data(priv->items,
                               gtk_tree_path_get_indices(path)[0]);

    if (!item || item->cell_area.width < 0 ||
        !gtk_widget_get_realized(widget)) {
        if (priv->scroll_to_path)
            gtk_tree_row_reference_free(priv->scroll_to_path);

        priv->scroll_to_path = NULL;

        if (path)
            priv->scroll_to_path = gtk_tree_row_reference_new_proxy(G_OBJECT(browser),
                                                                    priv->model, path);

        priv->scroll_to_use_align = use_align;
        priv->scroll_to_row_align = row_align;
        priv->scroll_to_col_align = col_align;

        return;
    }

    if (use_align) {
        GtkAllocation allocation;
        gint x, y;
        gdouble offset;
        GdkRectangle item_area = {
            item->cell_area.x - priv->item_padding,
            item->cell_area.y - priv->item_padding,
            item->cell_area.width  + priv->item_padding * 2,
            item->cell_area.height + priv->item_padding * 2
        };

        gdk_window_get_position(priv->bin_window, &x, &y);

        gtk_widget_get_allocation(widget, &allocation);

        offset = y + item_area.y - row_align * (allocation.height - item_area.height);

        gtk_adjustment_set_value(priv->vadjustment,
                                 gtk_adjustment_get_value(priv->vadjustment) + offset);

        offset = x + item_area.x - col_align * (allocation.width - item_area.width);

        gtk_adjustment_set_value(priv->hadjustment,
                                 gtk_adjustment_get_value(priv->hadjustment) + offset);

        gtk_adjustment_changed(priv->hadjustment);
        gtk_adjustment_changed(priv->vadjustment);
    } else {
        entangle_session_browser_scroll_to_item(browser, item);
    }
}


static gboolean
entangle_session_browser_button_press(GtkWidget *widget,
                                      GdkEventButton *event)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget), FALSE);

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;
    EntangleSessionBrowserItem *item;
    gboolean dirty = FALSE;
    GtkCellRenderer *cell = NULL;

    if (event->window != priv->bin_window)
        return FALSE;

    if (!gtk_widget_has_focus(widget))
        gtk_widget_grab_focus(widget);

    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        item = entangle_session_browser_get_item_at_coords(browser,
                                                           event->x, event->y,
                                                           FALSE,
                                                           &cell);

        /*
         * We consider only the cells' area as the item area if the
         * item is not selected, but if it *is* selected, the complete
         * selection rectangle is considered to be part of the item.
         */
        if (item != NULL && !item->selected) {
            entangle_session_browser_unselect_all_internal(browser);
            dirty = TRUE;
            item->selected = TRUE;
            entangle_session_browser_queue_draw_item(browser, item);
            entangle_session_browser_scroll_to_item(browser, item);
        }
        if (item != NULL) {
            priv->dnd_start_x = event->x;
            priv->dnd_start_y = event->y;
        }
    }

    if (dirty)
        g_signal_emit(browser, browser_signals[SIGNAL_SELECTION_CHANGED], 0);

    return event->button == 1;
}


static gboolean
entangle_session_browser_button_release(GtkWidget *widget,
                                        GdkEventButton *event)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget), FALSE);

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    if (event->button == 1 && event->type == GDK_BUTTON_RELEASE) {
        priv->dnd_start_x = -1;
        priv->dnd_start_y = -1;
    }

    return event->button == 1;
}


static void
entangle_session_browser_convert_widget_to_bin_window_coords(EntangleSessionBrowser *browser,
                                                             gint wx,
                                                             gint wy,
                                                             gint *bx,
                                                             gint *by)
{
    gint x, y;

    g_return_if_fail(ENTANGLE_SESSION_BROWSER(browser));

    if (browser->priv->bin_window)
        gdk_window_get_position(browser->priv->bin_window, &x, &y);
    else
        x = y = 0;

    if (bx)
        *bx = wx - x;
    if (by)
        *by = wy - y;
}


static void
entangle_session_browser_set_tooltip_cell(EntangleSessionBrowser *browser,
                                          GtkTooltip *tooltip,
                                          EntangleSessionBrowserItem *item)
{
    GdkRectangle rect;
    gint x, y;

    g_return_if_fail(ENTANGLE_SESSION_BROWSER(browser));
    g_return_if_fail(GTK_IS_TOOLTIP(tooltip));

    rect.x = item->cell_area.x - browser->priv->item_padding;
    rect.y = item->cell_area.y - browser->priv->item_padding;
    rect.width  = item->cell_area.width  + browser->priv->item_padding * 2;
    rect.height = item->cell_area.height + browser->priv->item_padding * 2;

    if (browser->priv->bin_window) {
        gdk_window_get_position(browser->priv->bin_window, &x, &y);
        rect.x += x;
        rect.y += y;
    }

    gtk_tooltip_set_tip_area(tooltip, &rect);
}


static gboolean
entangle_session_browser_query_tooltip(GtkWidget  *widget,
                                       gint        x,
                                       gint        y,
                                       gboolean    keyboard_mode G_GNUC_UNUSED,
                                       GtkTooltip *tooltip,
                                       gpointer    user_data)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget), FALSE);

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserItem *item;
    GtkCellRenderer *cell = NULL;
    gint bx = 0;
    gint by = 0;

    entangle_session_browser_convert_widget_to_bin_window_coords(browser,
                                                                 x, y,
                                                                 &bx, &by);

    item = entangle_session_browser_get_item_at_coords(browser,
                                                       bx, by,
                                                       FALSE,
                                                       &cell);

    if (item != NULL) {
        GtkTreeModel *model = (GtkTreeModel*)user_data;
        GtkTreeIter iter = item->iter;
        gchar *file_name = NULL;

        gtk_tree_model_get(model, &iter, FIELD_NAME, &file_name, -1);

        if (file_name) {
            gtk_tooltip_set_text(tooltip, file_name);

            entangle_session_browser_set_tooltip_cell(browser, tooltip, item);
            return TRUE;
        }
    }

    return FALSE;
}


static gboolean
entangle_session_browser_scroll(GtkWidget *widget,
                                GdkEventScroll *event)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget), FALSE);

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    switch (event->direction) {
    case GDK_SCROLL_UP:
    case GDK_SCROLL_LEFT:
        gtk_adjustment_set_value(priv->hadjustment,
                                 gtk_adjustment_get_value(priv->hadjustment) -
                                 gtk_adjustment_get_step_increment(priv->hadjustment));
        break;
    case GDK_SCROLL_DOWN:
    case GDK_SCROLL_RIGHT:
        gtk_adjustment_set_value(priv->hadjustment,
                                 gtk_adjustment_get_value(priv->hadjustment) +
                                 gtk_adjustment_get_step_increment(priv->hadjustment));
        break;

    case GDK_SCROLL_SMOOTH:
    default:
        break;
    }

    return TRUE;
}


static gboolean
entangle_session_browser_motion_notify(GtkWidget *widget,
                                       GdkEventMotion *event)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget), FALSE);

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;
    EntangleSessionBrowserItem *item;
    GdkPixbuf *pixbuf;
    GtkTargetEntry tentry[] = {
        { g_strdup("demo"), GTK_TARGET_SAME_APP, 0,}
    };
    GtkTargetList *tlist;
    GdkDragContext *ctx;

    if (priv->dnd_start_y == -1 || priv->dnd_start_x == -1)
        return FALSE;

    if (!gtk_drag_check_threshold(widget,
                                  priv->dnd_start_x,
                                  priv->dnd_start_y,
                                  event->x, event->y))
        return TRUE;

    item = entangle_session_browser_get_item_at_coords(browser,
                                                       priv->dnd_start_x,
                                                       priv->dnd_start_y,
                                                       FALSE,
                                                       NULL);

    if (!item) {
        priv->dnd_start_x = priv->dnd_start_y = -1;
        return FALSE;
    }

    gtk_tree_model_get(priv->model, &item->iter, FIELD_PIXMAP, &pixbuf, -1);

    tlist = gtk_target_list_new(tentry, G_N_ELEMENTS(tentry));

#if GTK_CHECK_VERSION(3,10,0)
    ctx = gtk_drag_begin_with_coordinates(widget,
                                          tlist,
                                          GDK_ACTION_PRIVATE,
                                          1,
                                          (GdkEvent*)event,
                                          -1, -1);
#else
    ctx = gtk_drag_begin(widget,
                         tlist,
                         GDK_ACTION_PRIVATE,
                         1,
                         (GdkEvent*)event);
#endif

    gtk_drag_set_icon_pixbuf(ctx, pixbuf, 0, 0);

    gtk_target_list_unref(tlist);

    return TRUE;
}

static gboolean
entangle_session_browser_key_press(GtkWidget *widget,
                                   GdkEventKey *event)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget), FALSE);

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;
    GList *list, *prev = NULL;

    switch (event->keyval) {
    case GDK_KEY_Right:
        for (list = priv->items; list != NULL; list = list->next) {
            EntangleSessionBrowserItem *item = list->data;

            if (item->selected && list->next) {
                EntangleSessionBrowserItem *next = list->next->data;
                entangle_session_browser_unselect_item(browser, item);
                entangle_session_browser_select_item(browser, next);
                entangle_session_browser_scroll_to_item(browser, next);
                break;
            }
        }
        return TRUE;

    case GDK_KEY_Left:
        for (list = priv->items; list != NULL; list = list->next) {
            EntangleSessionBrowserItem *item = list->data;

            if (item->selected && prev) {
                EntangleSessionBrowserItem *prior = prev->data;
                entangle_session_browser_unselect_item(browser, item);
                entangle_session_browser_select_item(browser, prior);
                entangle_session_browser_scroll_to_item(browser, prior);
                break;
            }
            prev = list;
        }
        return TRUE;

    default:
        return GTK_WIDGET_CLASS(entangle_session_browser_parent_class)->key_press_event(widget, event);
    }
}


static void
entangle_session_browser_size_allocate(GtkWidget *widget,
                                       GtkAllocation *allocation)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    gtk_widget_set_allocation(widget, allocation);

    if (gtk_widget_get_realized(widget)) {
        gdk_window_move_resize(gtk_widget_get_window(widget),
                               allocation->x, allocation->y,
                               allocation->width, allocation->height);
        gdk_window_resize(priv->bin_window,
                          MAX(priv->width, allocation->width),
                          MAX(priv->height, allocation->height));
    }

    entangle_session_browser_layout(browser);

    /* Delay signal emission */
    g_object_freeze_notify(G_OBJECT(priv->hadjustment));
    g_object_freeze_notify(G_OBJECT(priv->vadjustment));

    entangle_session_browser_set_hadjustment_values(browser);
    entangle_session_browser_set_vadjustment_values(browser);

    if (gtk_widget_get_realized(widget) &&
        priv->scroll_to_path) {
        GtkTreePath *path;
        path = gtk_tree_row_reference_get_path(priv->scroll_to_path);
        gtk_tree_row_reference_free(priv->scroll_to_path);
        priv->scroll_to_path = NULL;

        entangle_session_browser_scroll_to_path(browser, path,
                                                priv->scroll_to_use_align,
                                                priv->scroll_to_row_align,
                                                priv->scroll_to_col_align);
        gtk_tree_path_free(path);
    }

    /* Emit any pending signals now */
    g_object_thaw_notify(G_OBJECT(priv->hadjustment));
    g_object_thaw_notify(G_OBJECT(priv->vadjustment));
}


static void entangle_session_browser_destroy(GtkWidget *widget)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget));

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    if (priv->scroll_to_path != NULL) {
        gtk_tree_row_reference_free(priv->scroll_to_path);
        priv->scroll_to_path = NULL;
    }

    if (priv->hadjustment != NULL) {
        g_object_unref(priv->hadjustment);
        priv->hadjustment = NULL;
    }

    if (priv->vadjustment != NULL) {
        g_object_unref(priv->vadjustment);
        priv->vadjustment = NULL;
    }

    GTK_WIDGET_CLASS(entangle_session_browser_parent_class)->destroy(widget);
}


static void entangle_session_browser_finalize(GObject *object)
{
    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(object);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    if (priv->session && priv->loader)
        do_model_unload(browser);

    if (priv->cell_area_context) {
        g_signal_handler_disconnect(priv->cell_area_context, priv->context_changed_id);
        priv->context_changed_id = 0;

        g_object_unref(priv->cell_area_context);
        priv->cell_area_context = NULL;
    }

    if (priv->session)
        g_object_unref(priv->session);
    if (priv->loader)
        g_object_unref(priv->loader);

    G_OBJECT_CLASS(entangle_session_browser_parent_class)->finalize(object);
}


static void
entangle_session_browser_cell_layout_init(GtkCellLayoutIface *iface)
{
    iface->get_area = entangle_session_browser_cell_layout_get_area;
}

static void entangle_session_browser_class_init(EntangleSessionBrowserClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->finalize = entangle_session_browser_finalize;
    object_class->get_property = entangle_session_browser_get_property;
    object_class->set_property = entangle_session_browser_set_property;

    widget_class->destroy = entangle_session_browser_destroy;
    widget_class->realize = entangle_session_browser_realize;
    widget_class->unrealize = entangle_session_browser_unrealize;
    widget_class->draw = entangle_session_browser_draw;
    widget_class->button_press_event = entangle_session_browser_button_press;
    widget_class->button_release_event = entangle_session_browser_button_release;
    widget_class->scroll_event = entangle_session_browser_scroll;
    widget_class->motion_notify_event = entangle_session_browser_motion_notify;
    widget_class->key_press_event = entangle_session_browser_key_press;
    widget_class->size_allocate = entangle_session_browser_size_allocate;

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

    /* Scrollable interface properties */
    g_object_class_override_property(object_class, PROP_HADJUSTMENT,    "hadjustment");
    g_object_class_override_property(object_class, PROP_VADJUSTMENT,    "vadjustment");
    g_object_class_override_property(object_class, PROP_HSCROLL_POLICY, "hscroll-policy");
    g_object_class_override_property(object_class, PROP_VSCROLL_POLICY, "vscroll-policy");

    browser_signals[SIGNAL_SELECTION_CHANGED] =
        g_signal_new("selection-changed",
                     G_TYPE_FROM_CLASS(object_class),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(EntangleSessionBrowserClass, selection_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    g_type_class_add_private(klass, sizeof(EntangleSessionBrowserPrivate));
}


EntangleSessionBrowser *entangle_session_browser_new(void)
{
    return ENTANGLE_SESSION_BROWSER(g_object_new(ENTANGLE_TYPE_SESSION_BROWSER, NULL));
}


static void entangle_session_browser_init(EntangleSessionBrowser *browser)
{
    EntangleSessionBrowserPrivate *priv;

    priv = browser->priv = ENTANGLE_SESSION_BROWSER_GET_PRIVATE(browser);

    priv->model = GTK_TREE_MODEL(gtk_list_store_new(FIELD_LAST,
                                                    ENTANGLE_TYPE_IMAGE,
                                                    GDK_TYPE_PIXBUF,
                                                    G_TYPE_INT,
                                                    G_TYPE_STRING));

    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(priv->model),
                                            do_image_sort_name, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->model),
                                         GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                         GTK_SORT_ASCENDING);

    priv->cell_area = gtk_cell_area_box_new();
    g_object_ref_sink(priv->cell_area);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(priv->cell_area), GTK_ORIENTATION_VERTICAL);

    priv->cell_area_context = gtk_cell_area_create_context(priv->cell_area);
    priv->context_changed_id =
        g_signal_connect(priv->cell_area_context, "notify",
                         G_CALLBACK(entangle_session_browser_context_changed), browser);

    priv->pixbuf_cell = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(browser), priv->pixbuf_cell, FALSE);

    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(browser),
                                   priv->pixbuf_cell,
                                   "pixbuf", FIELD_PIXMAP,
                                   NULL);

    g_object_set(priv->pixbuf_cell,
                 "xalign", 0.5,
                 "yalign", 1.0,
                 NULL);

    gtk_widget_set_has_tooltip(GTK_WIDGET(browser), TRUE);

    g_signal_connect(priv->model,
                     "row-changed",
                     G_CALLBACK(entangle_session_browser_row_changed),
                     browser);
    g_signal_connect(priv->model,
                     "row-inserted",
                     G_CALLBACK(entangle_session_browser_row_inserted),
                     browser);
    g_signal_connect(priv->model,
                     "row-deleted",
                     G_CALLBACK(entangle_session_browser_row_deleted),
                     browser);
    g_signal_connect(priv->model,
                     "rows-reordered",
                     G_CALLBACK(entangle_session_browser_rows_reordered),
                     browser);

    g_signal_connect(GTK_WIDGET(browser),
                     "query-tooltip",
                     G_CALLBACK(entangle_session_browser_query_tooltip),
                     priv->model);

    entangle_session_browser_build_items(browser);
    gtk_widget_queue_resize(GTK_WIDGET(browser));

    priv->margin = 6;
    priv->item_padding = 0;
    priv->column_spacing = 6;

    gtk_widget_set_can_focus(GTK_WIDGET(browser), TRUE);
}


static GList *
entangle_session_browser_get_selected_items(EntangleSessionBrowser *browser)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GList *list;
    GList *selected = NULL;

    for (list = priv->items; list != NULL; list = list->next) {
        EntangleSessionBrowserItem *item = list->data;

        if (item->selected) {
            GtkTreePath *path = gtk_tree_path_new_from_indices(item->idx, -1);

            selected = g_list_prepend(selected, path);
        }
    }

    return selected;
}


/**
 * entangle_session_browser_selected_image:
 * @browser: (transfer none): the session browser
 *
 * Returns: (transfer full): the selected image or NULL
 */
EntangleImage *entangle_session_browser_selected_image(EntangleSessionBrowser *browser)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GList *items;
    EntangleImage *img = NULL;
    GtkTreePath *path;
    GtkTreeIter iter;
    GValue val;

    items = entangle_session_browser_get_selected_items(browser);

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


/**
 * entangle_session_browser_earlier_images:
 * @browser: (transfer none): the session browser
 * @include_selected: true to include the current image in the list
 * @count: maximum number of images to return
 *
 * Get a list of images prior to the currently selected image.
 * If @include_selected is true, then the currently selected
 * image will be included in the returned list
 *
 * Returns: (transfer full)(element-type EntangleImage): the list of images
 */
GList *entangle_session_browser_earlier_images(EntangleSessionBrowser *browser,
                                               gboolean include_selected,
                                               gsize count)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GList *list;
    GList *images = NULL;

    for (list = priv->items; list != NULL; list = list->next) {
        EntangleSessionBrowserItem *item = list->data;

        if (item->selected)
            break;
    }

    if (list) {
        if (!include_selected)
            list = list->prev;

        for (; list != NULL && count; list = list->prev, count--) {
            EntangleSessionBrowserItem *item = list->data;
            GtkTreePath *path = gtk_tree_path_new_from_indices(item->idx, -1);
            GtkTreeIter iter;
            GValue val;

            if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->model), &iter, path))
                goto cleanup;

            memset(&val, 0, sizeof val);
            gtk_tree_model_get_value(GTK_TREE_MODEL(priv->model), &iter, 0, &val);

            images = g_list_append(images, g_value_get_object(&val));
        }
    }

    return images;

 cleanup:
    g_list_foreach(images, (GFunc)(g_object_unref), NULL);
    g_list_free(images);
    return NULL;
}


/**
 * entangle_session_browser_set_thumbnail_loader:
 * @browser: (transfer none): the session browser
 * @loader: (transfer none): the thumbnail loader to use
 *
 * Set the thumbnail loader to use for generating image
 * thumbnails
 */
void entangle_session_browser_set_thumbnail_loader(EntangleSessionBrowser *browser,
                                                   EntangleThumbnailLoader *loader)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

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


/**
 * entangle_session_browser_get_thumbnail_loader:
 * @browser: the session browser
 *
 * Get the thumbnail loader used by the session browser
 *
 * Returns: (transfer none): the session browser
 */
EntangleThumbnailLoader *entangle_session_browser_get_thumbnail_loader(EntangleSessionBrowser *browser)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;

    return priv->loader;
}


/**
 * entangle_session_browser_set_session:
 * @browser: (transfer none): the session browser
 * @session: (transfer none): the session to display
 *
 * Set the session to be displayed
 */
void entangle_session_browser_set_session(EntangleSessionBrowser *browser,
                                          EntangleSession *session)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

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


/**
 * entangle_session_browser_get_session:
 * @browser: (transfer none): the session browser
 *
 * Get the session being displayed
 *
 * Returns: (transfer none): the session
 */
EntangleSession *entangle_session_browser_get_session(EntangleSessionBrowser *browser)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;

    return priv->session;
}

void entangle_session_browser_set_background(EntangleSessionBrowser *browser,
                                             const gchar *background)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;

    gdk_rgba_parse(&priv->background, background);

    GtkWidget *widget = GTK_WIDGET(browser);
    gtk_widget_queue_draw(widget);
}

gchar *entangle_session_browser_get_background(EntangleSessionBrowser *browser)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;

    return gdk_rgba_to_string(&priv->background);
}

void entangle_session_browser_set_highlight(EntangleSessionBrowser *browser,
                                            const gchar *highlight)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;

    gdk_rgba_parse(&priv->highlight, highlight);

    GtkWidget *widget = GTK_WIDGET(browser);
    gtk_widget_queue_draw(widget);
}

gchar *entangle_session_browser_get_highlight(EntangleSessionBrowser *browser)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser), NULL);

    EntangleSessionBrowserPrivate *priv = browser->priv;

    return gdk_rgba_to_string(&priv->highlight);
}

static void
entangle_session_browser_paint_item(EntangleSessionBrowser *browser,
                                    cairo_t *cr,
                                    EntangleSessionBrowserItem *item,
                                    gint x,
                                    gint y)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    GdkRectangle cell_area;
    GtkCellRendererState flags = 0;
    GtkWidget *widget = GTK_WIDGET(browser);
    EntangleSessionBrowserPrivate *priv = browser->priv;

    entangle_session_browser_set_cell_data(browser, item);

    if (item->selected) {
        gint width  = item->cell_area.width  + priv->item_padding * 2;
        gint height = item->cell_area.height + priv->item_padding * 2;

        cairo_save(cr);
        cairo_set_source_rgba(cr, priv->highlight.red, priv->highlight.green, priv->highlight.blue, 1);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    cell_area.x      = x;
    cell_area.y      = y;
    cell_area.width  = item->cell_area.width;
    cell_area.height = item->cell_area.height;

    gtk_cell_area_render(priv->cell_area,
                         priv->cell_area_context,
                         widget, cr, &cell_area, &cell_area, flags,
                         FALSE);
}


static gboolean
entangle_session_browser_draw(GtkWidget *widget,
                              cairo_t *cr)
{
    g_return_val_if_fail(ENTANGLE_IS_SESSION_BROWSER(widget), FALSE);

    EntangleSessionBrowser *browser = ENTANGLE_SESSION_BROWSER(widget);
    EntangleSessionBrowserPrivate *priv = browser->priv;
    GList *icons;
    int ww, wh; /* Available drawing area extents */

    ww = gdk_window_get_width(gtk_widget_get_window(widget));
    wh = gdk_window_get_height(gtk_widget_get_window(widget));

    cairo_set_source_rgb(cr, priv->background.red, priv->background.green, priv->background.blue);
    cairo_rectangle(cr, 0, 0, ww, wh);
    cairo_fill(cr);

    if (!gtk_cairo_should_draw_window(cr, priv->bin_window))
        return FALSE;

    cairo_save(cr);
    gtk_cairo_transform_to_window(cr, widget, priv->bin_window);
    cairo_set_line_width(cr, 1.);

    for (icons = priv->items; icons; icons = icons->next) {
        EntangleSessionBrowserItem *item = icons->data;
        GdkRectangle paint_area;

        paint_area.x      = ((GdkRectangle *)item)->x      - priv->item_padding;
        paint_area.y      = ((GdkRectangle *)item)->y      - priv->item_padding;
        paint_area.width  = ((GdkRectangle *)item)->width  + priv->item_padding * 2;
        paint_area.height = ((GdkRectangle *)item)->height + priv->item_padding * 2;

        cairo_save(cr);
        cairo_set_source_rgba(cr, priv->background.red, priv->background.green, priv->background.blue, priv->background.alpha);
        cairo_rectangle(cr, paint_area.x, paint_area.y, paint_area.width, paint_area.height);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_rectangle(cr, paint_area.x, paint_area.y, paint_area.width, paint_area.height);
        cairo_clip(cr);

        if (gdk_cairo_get_clip_rectangle(cr, NULL))
            entangle_session_browser_paint_item(browser, cr, item,
                                                ((GdkRectangle *)item)->x, ((GdkRectangle *)item)->y);

        cairo_restore(cr);
    }

    cairo_restore(cr);

    return TRUE;
}


static void
entangle_session_browser_layout_row(EntangleSessionBrowser *browser,
                                    gint item_width,
                                    gint *y,
                                    gint *maximum_width)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkWidget *widget = GTK_WIDGET(browser);
    GtkAllocation allocation;
    gint x, current_width;
    GList *items;
    gint col;
    gint max_height = 0;

    x = 0;
    col = 0;
    current_width = 0;

    x += priv->margin;
    current_width += 2 * priv->margin;

    gtk_widget_get_allocation(widget, &allocation);

    /* In the first loop we iterate horizontally until we hit allocation width
     * and collect the aligned height-for-width */
    items = priv->items;
    while (items) {
        EntangleSessionBrowserItem *item = items->data;
        GdkRectangle *item_area = (GdkRectangle *)item;

        item_area->width = item_width;

        current_width += item_area->width + priv->item_padding * 2;

        /* Get this item's particular width & height (all alignments are cached by now) */
        entangle_session_browser_set_cell_data(browser, item);
        gtk_cell_area_get_preferred_height_for_width(priv->cell_area,
                                                     priv->cell_area_context,
                                                     widget, item_width,
                                                     NULL, NULL);

        current_width += priv->column_spacing;

        item_area->y = *y + priv->item_padding;
        item_area->x = x  + priv->item_padding;

        x = current_width - priv->margin;

        if (current_width > *maximum_width)
            *maximum_width = current_width;

        item->col = col;

        col++;
        items = items->next;
    }

    gtk_cell_area_context_get_preferred_height_for_width(priv->cell_area_context, item_width, &max_height, NULL);
    gtk_cell_area_context_allocate(priv->cell_area_context, item_width, max_height);

    /* In the second loop the item height has been aligned and derived and
     * we just set the height and handle rtl layout */
    for (items = priv->items; items != NULL; items = items->next) {
        EntangleSessionBrowserItem *item = items->data;
        GdkRectangle *item_area = (GdkRectangle *)item;

        /* All items in the same row get the same height */
        item_area->height = max_height;
    }

    /* Adjust the new y coordinate. */
    *y += max_height + priv->item_padding * 2;
}


static void
entangle_session_browser_layout(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkAllocation allocation;
    GtkWidget *widget = GTK_WIDGET(browser);
    gint y = 0, maximum_width = 0;
    gint item_width;
    gboolean size_changed = FALSE;

    /* Update the context widths for any invalidated items */
    entangle_session_browser_cache_widths(browser);

    /* Fetch the new item width if needed */
    gtk_cell_area_context_get_preferred_width(priv->cell_area_context,
                                              &item_width, NULL);

    gtk_cell_area_context_allocate(priv->cell_area_context, item_width, -1);

    y += priv->margin;

    entangle_session_browser_layout_row(browser,
                                        item_width,
                                        &y, &maximum_width);

    if (maximum_width != priv->width) {
        priv->width = maximum_width;
        size_changed = TRUE;
    }

    y += priv->margin;

    if (y != priv->height) {
        priv->height = y;
        size_changed = TRUE;
    }

    entangle_session_browser_set_hadjustment_values(browser);
    entangle_session_browser_set_vadjustment_values(browser);

    if (size_changed)
        gtk_widget_queue_resize_no_redraw(widget);

    gtk_widget_get_allocation(widget, &allocation);
    if (gtk_widget_get_realized(widget))
        gdk_window_resize(priv->bin_window,
                          MAX(priv->width, allocation.width),
                          MAX(priv->height, allocation.height));

    gtk_widget_queue_draw(widget);
}


static void
entangle_session_browser_set_hadjustment_values(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkAllocation allocation;
    GtkAdjustment *adj = priv->hadjustment;

    gtk_widget_get_allocation(GTK_WIDGET(browser), &allocation);

    gtk_adjustment_configure(adj,
                             gtk_adjustment_get_value(adj),
                             0.0,
                             MAX(allocation.width, priv->width),
                             allocation.width * 0.1,
                             allocation.width * 0.9,
                             allocation.width);
}


static void
entangle_session_browser_set_vadjustment_values(EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;
    GtkAllocation allocation;
    GtkAdjustment *adj = priv->vadjustment;

    gtk_widget_get_allocation(GTK_WIDGET(browser), &allocation);

    gtk_adjustment_configure(adj,
                             gtk_adjustment_get_value(adj),
                             0.0,
                             MAX(allocation.height, priv->height),
                             allocation.height * 0.1,
                             allocation.height * 0.9,
                             allocation.height);
}


static void
entangle_session_browser_set_hadjustment(EntangleSessionBrowser *browser,
                                         GtkAdjustment *adjustment)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;

    if (adjustment && priv->hadjustment == adjustment)
        return;

    if (priv->hadjustment != NULL) {
        g_signal_handlers_disconnect_matched(priv->hadjustment,
                                             G_SIGNAL_MATCH_DATA,
                                             0, 0, NULL, NULL, browser);
        g_object_unref(priv->hadjustment);
    }

    if (!adjustment)
        adjustment = gtk_adjustment_new(0.0, 0.0, 0.0,
                                        0.0, 0.0, 0.0);

    g_signal_connect(adjustment, "value-changed",
                     G_CALLBACK(entangle_session_browser_adjustment_changed), browser);
    priv->hadjustment = g_object_ref_sink(adjustment);
    entangle_session_browser_set_hadjustment_values(browser);

    g_object_notify(G_OBJECT(browser), "hadjustment");
}


static void
entangle_session_browser_set_vadjustment(EntangleSessionBrowser *browser,
                                         GtkAdjustment *adjustment)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;

    if (adjustment && priv->vadjustment == adjustment)
        return;

    if (priv->vadjustment != NULL) {
        g_signal_handlers_disconnect_matched(priv->vadjustment,
                                             G_SIGNAL_MATCH_DATA,
                                             0, 0, NULL, NULL, browser);
        g_object_unref(priv->vadjustment);
    }

    if (!adjustment)
        adjustment = gtk_adjustment_new(0.0, 0.0, 0.0,
                                        0.0, 0.0, 0.0);

    g_signal_connect(adjustment, "value-changed",
                     G_CALLBACK(entangle_session_browser_adjustment_changed), browser);
    priv->vadjustment = g_object_ref_sink(adjustment);
    entangle_session_browser_set_vadjustment_values(browser);

    g_object_notify(G_OBJECT(browser), "vadjustment");
}


static void
entangle_session_browser_adjustment_changed(GtkAdjustment *adjustment G_GNUC_UNUSED,
                                            EntangleSessionBrowser *browser)
{
    g_return_if_fail(ENTANGLE_IS_SESSION_BROWSER(browser));

    EntangleSessionBrowserPrivate *priv = browser->priv;

    if (gtk_widget_get_realized(GTK_WIDGET(browser))) {
        gdk_window_move(priv->bin_window,
                        - gtk_adjustment_get_value(priv->hadjustment),
                        - gtk_adjustment_get_value(priv->vadjustment));
    }
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
