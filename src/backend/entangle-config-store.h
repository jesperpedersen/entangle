/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#ifndef __ENTANGLE_CONFIG_STORE_H__
#define __ENTANGLE_CONFIG_STORE_H__

#include <glib-object.h>

#include "entangle-config-entry.h"
#include "entangle-config-set.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONFIG_STORE            (entangle_config_store_get_type ())
#define ENTANGLE_CONFIG_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONFIG_STORE, EntangleConfigStore))
#define ENTANGLE_IS_STORE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONFIG_STORE))
#define ENTANGLE_CONFIG_STORE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), ENTANGLE_TYPE_CONFIG_STORE, EntangleConfigStoreInterface))


typedef struct _EntangleConfigStore EntangleConfigStore; /* dummy object */
typedef struct _EntangleConfigStoreInterface EntangleConfigStoreInterface;

struct _EntangleConfigStoreInterface
{
    gboolean (*store_has_set_entry)(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry);
    gboolean (*store_clear_set_entry)(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry);
    gboolean (*store_read_set_entry)(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry, GValue *val);
    gboolean (*store_write_set_entry)(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry, GValue *val);
};


GType entangle_config_store_get_type(void) G_GNUC_CONST;

gboolean entangle_config_store_has_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry);
gboolean entangle_config_store_clear_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry);
gboolean entangle_config_store_read_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry, GValue *value);
gboolean entangle_config_store_write_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry, GValue *value);

G_END_DECLS

#endif /* __ENTANGLE_CONFIG_STORE_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
