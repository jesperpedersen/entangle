/*
 *  Capa: Capa Assists Photograph Aquisition
 *
 *  Copyright (C) 2009-2010 Daniel P. Berrange
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
#include "capa-config-store.h"


GType capa_config_store_get_type(void)
{
    static GType store_type = 0;

    if (!store_type) {
        store_type =
            g_type_register_static_simple(G_TYPE_INTERFACE, "CapaConfigStore",
                                          sizeof (CapaConfigStoreInterface),
                                          NULL, 0, NULL, 0);

        g_type_interface_add_prerequisite (store_type, G_TYPE_OBJECT);
    }

    return store_type;
}

gboolean capa_config_store_has_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry)
{
    return CAPA_CONFIG_STORE_GET_INTERFACE(store)->store_has_set_entry(store, set, entry);
}

gboolean capa_config_store_clear_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry)
{
    return CAPA_CONFIG_STORE_GET_INTERFACE(store)->store_clear_set_entry(store, set, entry);
}

gboolean capa_config_store_read_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry, GValue *value)
{
    return CAPA_CONFIG_STORE_GET_INTERFACE(store)->store_read_set_entry(store, set, entry, value);
}

gboolean capa_config_store_write_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry, GValue *value)
{
    return CAPA_CONFIG_STORE_GET_INTERFACE(store)->store_write_set_entry(store, set, entry, value);
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
