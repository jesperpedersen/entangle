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

#include <config.h>

#include "internal.h"
#include "progress.h"

void capa_progress_start(CapaProgress *prog, float target, const char *format, va_list args)
{
    CAPA_PROGRESS_GET_INTERFACE(prog)->start(prog, target, format, args);
}

void capa_progress_update(CapaProgress *prog, float current)
{
    CAPA_PROGRESS_GET_INTERFACE(prog)->update(prog, current);
}

void capa_progress_stop(CapaProgress *prog)
{
    CAPA_PROGRESS_GET_INTERFACE(prog)->stop(prog);
}

gboolean capa_progress_cancelled(CapaProgress *prog)
{
    return CAPA_PROGRESS_GET_INTERFACE(prog)->cancelled(prog);
}

GType
capa_progress_get_type (void)
{
    static GType progress_type = 0;

    if (!progress_type) {
        progress_type =
            g_type_register_static_simple (G_TYPE_INTERFACE, "CapaProgress",
                                           sizeof (CapaProgressInterface),
                                           NULL, 0, NULL, 0);

        g_type_interface_add_prerequisite (progress_type, G_TYPE_OBJECT);
    }

    return progress_type;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
