

#ifndef __CAPA_APP_DISPLAY__
#define __CAPA_APP_DISPLAY__

#include <glib-object.h>

#include "app.h"

G_BEGIN_DECLS

#define CAPA_TYPE_APP_DISPLAY            (capa_app_display_get_type ())
#define CAPA_APP_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_APP_DISPLAY, CapaAppDisplay))
#define CAPA_APP_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_APP_DISPLAY, CapaAppDisplayClass))
#define CAPA_IS_APP_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_APP_DISPLAY))
#define CAPA_IS_APP_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_APP_DISPLAY))
#define CAPA_APP_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_APP_DISPLAY, CapaAppDisplayClass))


typedef struct _CapaAppDisplay CapaAppDisplay;
typedef struct _CapaAppDisplayPrivate CapaAppDisplayPrivate;
typedef struct _CapaAppDisplayClass CapaAppDisplayClass;

struct _CapaAppDisplay
{
  GObject parent;

  CapaAppDisplayPrivate *priv;
};

struct _CapaAppDisplayClass
{
  GObjectClass parent_class;

  void (*app_closed)(CapaAppDisplay *picker);
};


GType capa_app_display_get_type(void) G_GNUC_CONST;
CapaAppDisplay* capa_app_display_new(void);

void capa_app_display_show(CapaAppDisplay *display);

G_END_DECLS

#endif /* __CAPA_APP_DISPLAY__ */

