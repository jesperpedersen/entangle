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
#include "entangle-plugin-manager.h"
#include "entangle-plugin-javascript.h"
#include "entangle-app.h"

#include <stdio.h>

gboolean plugin_activate(EntanglePlugin *plugin, GObject *app);
gboolean plugin_deactivate(EntanglePlugin *plugin, GObject *app);


gboolean plugin_activate(EntanglePlugin *plugin G_GNUC_UNUSED,
                         GObject *app) {
    ENTANGLE_DEBUG("Register javascript plugin type");
    entangle_plugin_manager_register_type(entangle_app_get_plugin_manager(ENTANGLE_APP(app)), "javascript",
                                      ENTANGLE_TYPE_PLUGIN_JAVASCRIPT);
    return TRUE;
}

gboolean plugin_deactivate(EntanglePlugin *plugin G_GNUC_UNUSED,
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
