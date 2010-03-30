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

#ifndef __ENTANGLE_CONFIG_STORE_GCONF_H__
#define __ENTANGLE_CONFIG_STORE_GCONF_H__

#include <glib-object.h>

#include "entangle-config-store.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONFIG_STORE_GCONF            (entangle_config_store_gconf_get_type ())
#define ENTANGLE_CONFIG_STORE_GCONF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONFIG_STORE_GCONF, EntangleConfigStoreGConf))
#define ENTANGLE_CONFIG_STORE_GCONF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONFIG_STORE_GCONF, EntangleConfigStoreGConfClass))
#define ENTANGLE_IS_CONFIG_STORE_GCONF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONFIG_STORE_GCONF))
#define ENTANGLE_IS_CONFIG_STORE_GCONF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONFIG_STORE_GCONF))
#define ENTANGLE_CONFIG_STORE_GCONF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONFIG_STORE_GCONF, EntangleConfigStoreGConfClass))


typedef struct _EntangleConfigStoreGConf EntangleConfigStoreGConf;
typedef struct _EntangleConfigStoreGConfPrivate EntangleConfigStoreGConfPrivate;
typedef struct _EntangleConfigStoreGConfClass EntangleConfigStoreGConfClass;

struct _EntangleConfigStoreGConf
{
    GObject parent;

    EntangleConfigStoreGConfPrivate *priv;
};

struct _EntangleConfigStoreGConfClass
{
    GObjectClass parent_class;
};


GType entangle_config_store_gconf_get_type(void) G_GNUC_CONST;

EntangleConfigStoreGConf *entangle_config_store_gconf_new(void);

G_END_DECLS

#endif /* __ENTANGLE_CONFIG_STORE_GCONF_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
