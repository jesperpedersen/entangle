/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#ifndef __ENTANGLE_CANCELLABLE_H__
#define __ENTANGLE_CANCELLABLE_H__

#include <glib-object.h>

#include "entangle-control-group.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CANCELLABLE                (entangle_cancellable_get_type ())
#define ENTANGLE_CANCELLABLE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CANCELLABLE, EntangleCancellable))
#define ENTANGLE_IS_CANCELLABLE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CANCELLABLE))
#define ENTANGLE_CANCELLABLE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), ENTANGLE_TYPE_CANCELLABLE, EntangleCancellableInterface))

typedef struct _EntangleCancellable EntangleCancellable; /* dummy object */
typedef struct _EntangleCancellableInterface EntangleCancellableInterface;

struct _EntangleCancellableInterface {
    GTypeInterface parent;

    void (*cancel) (EntangleCancellable *prog);
    void (*reset) (EntangleCancellable *prog);
    gboolean (*is_cancelled) (EntangleCancellable *prog);
};

GType entangle_cancellable_get_type(void);

void entangle_cancellable_cancel(EntangleCancellable *can);
void entangle_cancellable_reset(EntangleCancellable *can);
gboolean entangle_cancellable_is_cancelled(EntangleCancellable *can);

G_END_DECLS

#endif /* __ENTANGLE_CANCELLABLE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
