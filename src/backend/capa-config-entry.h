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

#ifndef __CAPA_CONFIG_ENTRY_H__
#define __CAPA_CONFIG_ENTRY_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_CONFIG_ENTRY            (capa_config_entry_get_type ())
#define CAPA_CONFIG_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONFIG_ENTRY, CapaConfigEntry))
#define CAPA_CONFIG_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONFIG_ENTRY, CapaConfigEntryClass))
#define CAPA_IS_CONFIG_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONFIG_ENTRY))
#define CAPA_IS_CONFIG_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONFIG_ENTRY))
#define CAPA_CONFIG_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONFIG_ENTRY, CapaConfigEntryClass))


typedef struct _CapaConfigEntry CapaConfigEntry;
typedef struct _CapaConfigEntryPrivate CapaConfigEntryPrivate;
typedef struct _CapaConfigEntryClass CapaConfigEntryClass;

typedef enum {
    CAPA_CONFIG_ENTRY_STRING,
    CAPA_CONFIG_ENTRY_INT,
    CAPA_CONFIG_ENTRY_DOUBLE,
    CAPA_CONFIG_ENTRY_BOOL,

    CAPA_CONFIG_ENTRY_STRING_LIST,
    CAPA_CONFIG_ENTRY_INT_LIST,
    CAPA_CONFIG_ENTRY_DOUBLE_LIST,
    CAPA_CONFIG_ENTRY_BOOL_LIST,

    CAPA_CONFIG_ENTRY_STRING_HASH,
    CAPA_CONFIG_ENTRY_INT_HASH,
    CAPA_CONFIG_ENTRY_DOUBLE_HASH,
    CAPA_CONFIG_ENTRY_BOOL_HASH,
} CapaConfigEntryDatatype;

struct _CapaConfigEntry
{
    GObject parent;

    CapaConfigEntryPrivate *priv;
};

struct _CapaConfigEntryClass
{
    GObjectClass parent_class;
};


GType capa_config_entry_get_type(void) G_GNUC_CONST;

CapaConfigEntry *capa_config_entry_new(CapaConfigEntryDatatype datatype,
                                       const char *name,
                                       const char *description);

CapaConfigEntryDatatype capa_config_entry_get_datatype(CapaConfigEntry *entry);
const char *capa_config_entry_get_name(CapaConfigEntry *entry);
const char *capa_config_entry_get_description(CapaConfigEntry *entry);

G_END_DECLS

#endif /* __CAPA_CONFIG_ENTRY_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
