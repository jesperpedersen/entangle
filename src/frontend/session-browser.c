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

#include <string.h>

#include "session-browser.h"
#include "session.h"

#define CAPA_SESSION_BROWSER_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_SESSION_BROWSER, CapaSessionBrowserPrivate))

struct _CapaSessionBrowserPrivate {
  CapaSession *session;

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
  fprintf(stderr, "Refresh model\n");
  gtk_list_store_clear(priv->model);

  if (!priv->session) {
    return;
  }

  for (int i = 0 ; i < capa_session_image_count(priv->session) ; i++) {
    CapaImage *img = capa_session_image_get(priv->session, i);
    GdkPixbuf *pixbuf = capa_image_thumbnail(img);
    GtkTreeIter iter;

    gtk_list_store_append(priv->model, &iter);

    /* XXX what's our refcount policy going to be for pixbuf.... */
    gtk_list_store_set(priv->model, &iter, 0, img, 1, pixbuf, -1);

    //g_object_unref(cam);
  }
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

  fprintf(stderr, "Set prop on session browser %d\n", prop_id);

  switch (prop_id)
    {
    case PROP_SESSION:
      if (priv->session)
	g_object_unref(G_OBJECT(priv->session));
      priv->session = g_value_get_object(value);
      g_object_ref(G_OBJECT(priv->session));
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

  priv = browser->priv = CAPA_SESSION_BROWSER_GET_PRIVATE(browser);
  memset(priv, 0, sizeof *priv);

  priv->model = gtk_list_store_new(2, CAPA_TYPE_IMAGE, GDK_TYPE_PIXBUF);

  gtk_icon_view_set_text_column(GTK_ICON_VIEW(browser), -1);
  gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(browser), 1);
  gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(browser), GTK_SELECTION_SINGLE);
  gtk_icon_view_set_model(GTK_ICON_VIEW(browser), GTK_TREE_MODEL(priv->model));

  gtk_icon_view_set_orientation(GTK_ICON_VIEW(browser), GTK_ORIENTATION_HORIZONTAL);
  /* XXX gross hack - GtkIconView doesn't seem to have a better
   * way to force everything into a single row. Perhaps we should
   * just right a new widget for our needs */
  gtk_icon_view_set_columns(GTK_ICON_VIEW(browser), 1000);
}

