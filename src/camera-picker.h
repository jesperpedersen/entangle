
#ifndef __CAMERA_PICKER__
#define __CAMERA_PICKER__

#include <glib-object.h>

#include "camera.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_PICKER            (capa_camera_picker_get_type ())
#define CAPA_CAMERA_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_PICKER, CapaCameraPicker))
#define CAPA_CAMERA_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_PICKER, CapaCameraPickerClass))
#define CAPA_IS_CAMERA_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_PICKER))
#define CAPA_IS_CAMERA_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_PICKER))
#define CAPA_CAMERA_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_PICKER, CapaCameraPickerClass))


typedef struct _CapaCameraPicker CapaCameraPicker;
typedef struct _CapaCameraPickerPrivate CapaCameraPickerPrivate;
typedef struct _CapaCameraPickerClass CapaCameraPickerClass;

struct _CapaCameraPicker
{
  GObject parent;

  CapaCameraPickerPrivate *priv;
};

struct _CapaCameraPickerClass
{
  GObjectClass parent_class;

  void (*picker_connect)(CapaCameraPicker *picker, CapaCamera *cam);
  void (*picker_refresh)(CapaCameraPicker *picker);
  void (*picker_close)(CapaCameraPicker *picker);
};


GType capa_camera_picker_get_type(void) G_GNUC_CONST;
CapaCameraPicker* capa_camera_picker_new(void);

void capa_camera_picker_show(CapaCameraPicker *picker);
void capa_camera_picker_hide(CapaCameraPicker *picker);

G_END_DECLS

#endif /* __CAMERA_PICKER__ */

