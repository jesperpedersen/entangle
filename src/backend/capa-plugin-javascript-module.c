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

#include "capa-debug.h"
#include "capa-plugin-manager.h"
#include "capa-plugin-javascript.h"
#include "capa-app.h"

#include <stdio.h>

gboolean plugin_activate(CapaPlugin *plugin, GObject *app);
gboolean plugin_deactivate(CapaPlugin *plugin, GObject *app);


gboolean plugin_activate(CapaPlugin *plugin G_GNUC_UNUSED,
                         GObject *app) {
    CAPA_DEBUG("Register javascript plugin type");
    capa_plugin_manager_register_type(capa_app_get_plugin_manager(CAPA_APP(app)), "javascript",
                                      CAPA_TYPE_PLUGIN_JAVASCRIPT);
    return TRUE;
}

gboolean plugin_deactivate(CapaPlugin *plugin G_GNUC_UNUSED,
                           GObject *app G_GNUC_UNUSED) {
    return TRUE;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
