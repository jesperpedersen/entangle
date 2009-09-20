
#include <string.h>
#include <glade/glade.h>

#include "camera-manager.h"
#include "camera-list.h"

#define CAPA_CAMERA_MANAGER_GET_PRIVATE(obj) \
      (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerPrivate))

struct _CapaCameraManagerPrivate {
  CapaCamera *camera;

  GladeXML *glade;
};

G_DEFINE_TYPE(CapaCameraManager, capa_camera_manager, G_TYPE_OBJECT);

enum {
  PROP_O,
  PROP_CAMERA
};


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
      g_value_set_pointer(value, priv->camera);
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

  fprintf(stderr, "Set prop %d\n", prop_id);

  switch (prop_id)
    {
    case PROP_CAMERA: {
      char *title;
      GtkWidget *win;
      capa_camera_free(priv->camera);
      priv->camera = g_value_get_pointer(value);

      title = g_strdup_printf("%s Camera Manager - Capa",
			      capa_camera_model(priv->camera));

      win = glade_xml_get_widget(priv->glade, "camera-manager");
      gtk_window_set_title(GTK_WINDOW(win), title);
      g_free(title);
    } break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void capa_camera_manager_finalize (GObject *object)
{
  CapaCameraManager *manager = CAPA_CAMERA_MANAGER(object);
  CapaCameraManagerPrivate *priv = manager->priv;

  /* XXX not right - unref */
  capa_camera_free(priv->camera);

  G_OBJECT_CLASS (capa_camera_manager_parent_class)->finalize (object);
}

static void capa_camera_manager_class_init(CapaCameraManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = capa_camera_manager_finalize;
  object_class->get_property = capa_camera_manager_get_property;
  object_class->set_property = capa_camera_manager_set_property;

  g_signal_new("manager-close",
	       G_TYPE_FROM_CLASS(klass),
	       G_SIGNAL_RUN_FIRST,
	       G_STRUCT_OFFSET(CapaCameraManagerClass, manager_close),
	       NULL, NULL,
	       g_cclosure_marshal_VOID__VOID,
	       G_TYPE_NONE,
	       0);

  g_object_class_install_property(object_class,
				  PROP_CAMERA,
				  g_param_spec_pointer("camera",
						       "Camera",
						       "Camera to be managed",
						       G_PARAM_READWRITE |
						       G_PARAM_STATIC_NAME |
						       G_PARAM_STATIC_NICK |
						       G_PARAM_STATIC_BLURB));

  g_type_class_add_private(klass, sizeof(CapaCameraManagerPrivate));
}


CapaCameraManager *capa_camera_manager_new(void)
{
  return CAPA_CAMERA_MANAGER(g_object_new(CAPA_TYPE_CAMERA_MANAGER, NULL));
}

static gboolean do_manager_close(GtkButton *src G_GNUC_UNUSED,
				 GdkEvent *ev G_GNUC_UNUSED,
				 CapaCameraManager *manager)
{
  fprintf(stderr, "manager close\n");
  g_signal_emit_by_name(manager, "manager-close", NULL);
  return TRUE;
}

static void capa_camera_manager_init(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv;

  priv = manager->priv = CAPA_CAMERA_MANAGER_GET_PRIVATE(manager);

  priv->glade = glade_xml_new("capa.glade", "camera-manager", "capa");

  glade_xml_signal_connect_data(priv->glade, "camera_manager_close", G_CALLBACK(do_manager_close), manager);
}

void capa_camera_manager_show(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

  gtk_widget_show(win);
  gtk_window_present(GTK_WINDOW(win));
}

void capa_camera_manager_hide(CapaCameraManager *manager)
{
  CapaCameraManagerPrivate *priv = manager->priv;
  GtkWidget *win = glade_xml_get_widget(priv->glade, "camera-manager");

  gtk_widget_hide(win);
}

