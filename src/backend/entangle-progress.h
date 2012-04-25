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

#ifndef __ENTANGLE_PROGRESS_H__
#define __ENTANGLE_PROGRESS_H__

#include <glib-object.h>

#include "entangle-control-group.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_PROGRESS                (entangle_progress_get_type ())
#define ENTANGLE_PROGRESS(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_PROGRESS, EntangleProgress))
#define ENTANGLE_IS_PROGRESS(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_PROGRESS))
#define ENTANGLE_PROGRESS_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), ENTANGLE_TYPE_PROGRESS, EntangleProgressInterface))

typedef struct _EntangleProgress EntangleProgress; /* dummy object */
typedef struct _EntangleProgressInterface EntangleProgressInterface;

struct _EntangleProgressInterface {
    GTypeInterface parent;

    void (*start) (EntangleProgress *prog, float target, const char *format, va_list args);
    void (*update) (EntangleProgress *prog, float current);
    void (*stop) (EntangleProgress *prog);
};

GType entangle_progress_get_type(void);

void entangle_progress_start(EntangleProgress *prog, float target, const char *format, va_list args);
void entangle_progress_update(EntangleProgress *prog, float current);
void entangle_progress_stop(EntangleProgress *prog);

G_END_DECLS

#endif /* __ENTANGLE_PROGRESS_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
