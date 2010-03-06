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

#ifndef __CAPA_CONFIG_STORE_H__
#define __CAPA_CONFIG_STORE_H__

#include <glib-object.h>

#include "capa-config-entry.h"
#include "capa-config-set.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONFIG_STORE            (capa_config_store_get_type ())
#define CAPA_CONFIG_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONFIG_STORE, CapaConfigStore))
#define CAPA_IS_STORE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONFIG_STORE))
#define CAPA_CONFIG_STORE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CAPA_TYPE_CONFIG_STORE, CapaConfigStoreInterface))


typedef struct _CapaConfigStore CapaConfigStore; /* dummy object */
typedef struct _CapaConfigStoreInterface CapaConfigStoreInterface;

struct _CapaConfigStoreInterface
{
    gboolean (*store_has_set_entry)(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry);
    gboolean (*store_clear_set_entry)(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry);
    gboolean (*store_read_set_entry)(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry, GValue *val);
    gboolean (*store_write_set_entry)(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry, GValue *val);
};


GType capa_config_store_get_type(void) G_GNUC_CONST;

gboolean capa_config_store_has_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry);
gboolean capa_config_store_clear_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry);
gboolean capa_config_store_read_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry, GValue *value);
gboolean capa_config_store_write_set_entry(CapaConfigStore *store, CapaConfigSet *set, CapaConfigEntry *entry, GValue *value);

G_END_DECLS

#endif /* __CAPA_CONFIG_STORE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
