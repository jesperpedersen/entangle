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
#include "entangle-confirmable.h"

void entangle_confirmable_reset(EntangleConfirmable *con)
{
    ENTANGLE_CONFIRMABLE_GET_INTERFACE(con)->reset(con);
}

void entangle_confirmable_confirm(EntangleConfirmable *con)
{
    ENTANGLE_CONFIRMABLE_GET_INTERFACE(con)->confirm(con);
}

gboolean entangle_confirmable_is_confirmed(EntangleConfirmable *con)
{
    return ENTANGLE_CONFIRMABLE_GET_INTERFACE(con)->is_confirmed(con);
}

GType
entangle_confirmable_get_type (void)
{
    static GType confirmable_type = 0;

    if (!confirmable_type) {
        confirmable_type =
            g_type_register_static_simple(G_TYPE_INTERFACE, "EntangleConfirmable",
                                          sizeof (EntangleConfirmableInterface),
                                          NULL, 0, NULL, 0);

        g_type_interface_add_prerequisite(confirmable_type, G_TYPE_OBJECT);
    }

    return confirmable_type;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
