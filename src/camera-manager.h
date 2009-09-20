
#ifndef __CAMERA_MANAGER__
#define __CAMERA_MANAGER__

#include <glib-object.h>

#include "camera.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_MANAGER            (capa_camera_manager_get_type ())
#define CAPA_CAMERA_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManager))
#define CAPA_CAMERA_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerClass))
#define CAPA_IS_CAMERA_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_MANAGER))
#define CAPA_IS_CAMERA_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_MANAGER))
#define CAPA_CAMERA_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_MANAGER, CapaCameraManagerClass))


typedef struct _CapaCameraManager CapaCameraManager;
typedef struct _CapaCameraManagerPrivate CapaCameraManagerPrivate;
typedef struct _CapaCameraManagerClass CapaCameraManagerClass;

struct _CapaCameraManager
{
  GObject parent;

  CapaCameraManagerPrivate *priv;
};

struct _CapaCameraManagerClass
{
  GObjectClass parent_class;

  void (*manager_close)(CapaCameraManager *manager);
};


GType capa_camera_manager_get_type(void) G_GNUC_CONST;
CapaCameraManager* capa_camera_manager_new(void);

void capa_camera_manager_show(CapaCameraManager *manager);
void capa_camera_manager_hide(CapaCameraManager *manager);

G_END_DECLS

#endif /* __CAMERA_MANAGER__ */

