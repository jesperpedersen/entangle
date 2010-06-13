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

#include <config.h>

#include "entangle-debug.h"
#include "entangle-cancellable.h"

void entangle_cancellable_reset(EntangleCancellable *can)
{
    ENTANGLE_CANCELLABLE_GET_INTERFACE(can)->reset(can);
}

void entangle_cancellable_cancel(EntangleCancellable *can)
{
    ENTANGLE_CANCELLABLE_GET_INTERFACE(can)->cancel(can);
}

gboolean entangle_cancellable_is_cancelled(EntangleCancellable *can)
{
    return ENTANGLE_CANCELLABLE_GET_INTERFACE(can)->is_cancelled(can);
}

GType
entangle_cancellable_get_type (void)
{
    static GType cancellable_type = 0;

    if (!cancellable_type) {
        cancellable_type =
            g_type_register_static_simple(G_TYPE_INTERFACE, "EntangleCancellable",
                                          sizeof (EntangleCancellableInterface),
                                          NULL, 0, NULL, 0);

        g_type_interface_add_prerequisite(cancellable_type, G_TYPE_OBJECT);
    }

    return cancellable_type;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
