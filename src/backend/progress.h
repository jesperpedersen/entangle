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

#ifndef __CAPA_PROGRESS__
#define __CAPA_PROGRESS__

#include <glib-object.h>

#include "control-group.h"

G_BEGIN_DECLS

#define CAPA_PROGRESS_TYPE                (capa_progress_get_type ())
#define CAPA_PROGRESS(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_PROGRESS_TYPE, CapaProgress))
#define CAPA_IS_PROGRESS(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_PROGRESS_TYPE))
#define CAPA_PROGRESS_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CAPA_PROGRESS_TYPE, CapaProgressInterface))

typedef struct _CapaProgress CapaProgress; /* dummy object */
typedef struct _CapaProgressInterface CapaProgressInterface;

struct _CapaProgressInterface {
  GTypeInterface parent;

  void (*start) (CapaProgress *prog, float target, const char *format, va_list args);
  void (*update) (CapaProgress *prog, float current);
  void (*stop) (CapaProgress *prog);

  gboolean (*cancelled) (CapaProgress *prog);
};

GType capa_progress_get_type(void);

void capa_progress_start(CapaProgress *prog, float target, const char *format, va_list args);
void capa_progress_update(CapaProgress *prog, float current);
void capa_progress_stop(CapaProgress *prog);
gboolean capa_progress_cancelled(CapaProgress *prog);

G_END_DECLS

#endif /* __CAPA_PROGRESS__ */
