/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#include <config.h>

#include "entangle-debug.h"

gboolean entangle_debug_app = FALSE;
gboolean entangle_debug_gphoto = FALSE;
gint64 entangle_debug_startms = 0;

static void gvir_log_handler(const gchar *log_domain G_GNUC_UNUSED,
                             GLogLevelFlags log_level G_GNUC_UNUSED,
                             const gchar *message,
                             gpointer user_data G_GNUC_UNUSED)
{
    g_printerr("%s\n", message);
}


void entangle_debug_setup(gboolean debug_app,
                          gboolean debug_gphoto)
{
    entangle_debug_app = debug_app;
    entangle_debug_gphoto = debug_gphoto;

#if GLIB_CHECK_VERSION(2, 31, 0)
    if (debug_app || debug_gphoto) {
        g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
                          gvir_log_handler, NULL);
    }
#endif
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
