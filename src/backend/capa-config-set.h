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

#ifndef __CAPA_CONFIG_SET_H__
#define __CAPA_CONFIG_SET_H__

#include <glib-object.h>

#include "capa-config-entry.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONFIG_SET            (capa_config_set_get_type ())
#define CAPA_CONFIG_SET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONFIG_SET, CapaConfigSet))
#define CAPA_CONFIG_SET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONFIG_SET, CapaConfigSetClass))
#define CAPA_IS_CONFIG_SET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONFIG_SET))
#define CAPA_IS_CONFIG_SET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONFIG_SET))
#define CAPA_CONFIG_SET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONFIG_SET, CapaConfigSetClass))


typedef struct _CapaConfigSet CapaConfigSet;
typedef struct _CapaConfigSetPrivate CapaConfigSetPrivate;
typedef struct _CapaConfigSetClass CapaConfigSetClass;

struct _CapaConfigSet
{
    GObject parent;

    CapaConfigSetPrivate *priv;
};

struct _CapaConfigSetClass
{
    GObjectClass parent_class;
};


GType capa_config_set_get_type(void) G_GNUC_CONST;

CapaConfigSet *capa_config_set_new(const char *name,
                                   const char *description);


const char *capa_config_set_get_name(CapaConfigSet *set);
const char *capa_config_set_get_description(CapaConfigSet *set);

void capa_config_set_add_entry(CapaConfigSet *set,
                               CapaConfigEntry *entry);

GList *capa_config_set_get_entries(CapaConfigSet *set);

G_END_DECLS

#endif /* __CAPA_CONFIG_SET_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
