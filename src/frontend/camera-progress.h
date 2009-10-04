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

#ifndef __CAPA_CAMERA_PROGRESS_H__
#define __CAPA_CAMERA_PROGRESS_H__

#include <glib-object.h>

#include "camera.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_PROGRESS            (capa_camera_progress_get_type ())
#define CAPA_CAMERA_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_PROGRESS, CapaCameraProgress))
#define CAPA_CAMERA_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_PROGRESS, CapaCameraProgressClass))
#define CAPA_IS_CAMERA_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_PROGRESS))
#define CAPA_IS_CAMERA_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_PROGRESS))
#define CAPA_CAMERA_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_PROGRESS, CapaCameraProgressClass))


typedef struct _CapaCameraProgress CapaCameraProgress;
typedef struct _CapaCameraProgressPrivate CapaCameraProgressPrivate;
typedef struct _CapaCameraProgressClass CapaCameraProgressClass;

struct _CapaCameraProgress
{
  GObject parent;

  CapaCameraProgressPrivate *priv;
};

struct _CapaCameraProgressClass
{
  GObjectClass parent_class;

  void (*progress_close)(CapaCameraProgress *progress);
};

GType capa_camera_progress_get_type(void) G_GNUC_CONST;

CapaCameraProgress* capa_camera_progress_new(void);

void capa_camera_progress_show(CapaCameraProgress *progress, const char *title);
void capa_camera_progress_hide(CapaCameraProgress *progress);

G_END_DECLS

#endif /* __CAPA_CAMERA_PROGRESS_H__ */

