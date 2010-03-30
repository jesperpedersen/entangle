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

#ifndef __ENTANGLE_CONFIG_SET_H__
#define __ENTANGLE_CONFIG_SET_H__

#include <glib-object.h>

#include "entangle-config-entry.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONFIG_SET            (entangle_config_set_get_type ())
#define ENTANGLE_CONFIG_SET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONFIG_SET, EntangleConfigSet))
#define ENTANGLE_CONFIG_SET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONFIG_SET, EntangleConfigSetClass))
#define ENTANGLE_IS_CONFIG_SET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONFIG_SET))
#define ENTANGLE_IS_CONFIG_SET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONFIG_SET))
#define ENTANGLE_CONFIG_SET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONFIG_SET, EntangleConfigSetClass))


typedef struct _EntangleConfigSet EntangleConfigSet;
typedef struct _EntangleConfigSetPrivate EntangleConfigSetPrivate;
typedef struct _EntangleConfigSetClass EntangleConfigSetClass;

struct _EntangleConfigSet
{
    GObject parent;

    EntangleConfigSetPrivate *priv;
};

struct _EntangleConfigSetClass
{
    GObjectClass parent_class;
};


GType entangle_config_set_get_type(void) G_GNUC_CONST;

EntangleConfigSet *entangle_config_set_new(const char *name,
                                   const char *description);


const char *entangle_config_set_get_name(EntangleConfigSet *set);
const char *entangle_config_set_get_description(EntangleConfigSet *set);

void entangle_config_set_add_entry(EntangleConfigSet *set,
                               EntangleConfigEntry *entry);

GList *entangle_config_set_get_entries(EntangleConfigSet *set);

G_END_DECLS

#endif /* __ENTANGLE_CONFIG_SET_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
