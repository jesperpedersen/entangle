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

#include "internal.h"
#include "session-browser.h"
#include "session.h"

#define CAPA_SESSION_BROWSER_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_SESSION_BROWSER, CapaSessionBrowserPrivate))

struct _CapaSessionBrowserPrivate {
  CapaSession *session;

  gulong sigImageAdded;

  GtkListStore *model;
};

G_DEFINE_TYPE(CapaSessionBrowser, capa_session_browser, GTK_TYPE_ICON_VIEW);

enum {
  PROP_O,
  PROP_SESSION,
};


static void do_model_refresh(CapaSessionBrowser *browser)
{
  CapaSessionBrowserPrivate *priv = browser->priv;
  CAPA_DEBUG("Refresh model");
  gtk_list_store_clear(priv->model);

  if (!priv->session) {
    return;
  }

  for (int i = 0 ; i < capa_session_image_count(priv->session) ; i++) {
    CapaImage *img = capa_session_image_get(priv->session, i);
    GdkPixbuf *pixbuf = capa_image_thumbnail(img);
    int mod = capa_image_last_modified(img);
    GtkTreeIter iter;

    gtk_list_store_append(priv->model, &iter);

    /* XXX what's our refcount policy going to be for pixbuf.... */
    gtk_list_store_set(priv->model, &iter, 0, img, 1, pixbuf, 2, mod, -1);

    //g_object_unref(cam);
  }
}


static void do_image_added(CapaSession *session G_GNUC_UNUSED,
			   CapaImage *img,
			   gpointer data)
{
  CapaSessionBrowser *manager = data;
  CapaSessionBrowserPrivate *priv = manager->priv;
  GdkPixbuf *pixbuf = capa_image_thumbnail(img);
  GtkTreeIter iter;
  int mod = capa_image_last_modified(img);

  fprintf(stderr, "Got new image for model %s %p\n", capa_image_filename(img), priv);

  gtk_list_store_append(priv->model, &iter);

    /* XXX what's our refcount policy going to be for pixbuf.... */
  gtk_list_store_set(priv->model, &iter, 0, img, 1, pixbuf, 2, mod, -1);

  gtk_widget_queue_resize(GTK_WIDGET(manager));
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
  CapaSessionBrowserPrivate *priv = browser->priv;

  CAPA_DEBUG("Set prop on session browser %d", prop_id);

  switch (prop_id)
    {
    case PROP_SESSION:
      if (priv->session) {
        g_signal_handler_disconnect(G_OBJECT(priv->session),
				    priv->sigImageAdded);
        g_object_unref(G_OBJECT(priv->session));
      }
      priv->session = g_value_get_object(value);
      g_object_ref(G_OBJECT(priv->session));

      priv->sigImageAdded = g_signal_connect(G_OBJECT(priv->session), "session-image-added",
					     G_CALLBACK(do_image_added), browser);

      do_model_refresh(browser);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_session_browser_finalize (GObject *object)
{
  CapaSessionBrowser *browser = CAPA_SESSION_BROWSER(object);
  CapaSessionBrowserPrivate *priv = browser->priv;

  gtk_list_store_clear(priv->model);
  if (priv->session)
    g_object_unref(G_OBJECT(priv->session));

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
