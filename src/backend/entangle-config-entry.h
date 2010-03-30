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

#ifndef __ENTANGLE_CONFIG_ENTRY_H__
#define __ENTANGLE_CONFIG_ENTRY_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONFIG_ENTRY            (entangle_config_entry_get_type ())
#define ENTANGLE_CONFIG_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONFIG_ENTRY, EntangleConfigEntry))
#define ENTANGLE_CONFIG_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONFIG_ENTRY, EntangleConfigEntryClass))
#define ENTANGLE_IS_CONFIG_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONFIG_ENTRY))
#define ENTANGLE_IS_CONFIG_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONFIG_ENTRY))
#define ENTANGLE_CONFIG_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONFIG_ENTRY, EntangleConfigEntryClass))


typedef struct _EntangleConfigEntry EntangleConfigEntry;
typedef struct _EntangleConfigEntryPrivate EntangleConfigEntryPrivate;
typedef struct _EntangleConfigEntryClass EntangleConfigEntryClass;

typedef enum {
    ENTANGLE_CONFIG_ENTRY_STRING,
    ENTANGLE_CONFIG_ENTRY_INT,
    ENTANGLE_CONFIG_ENTRY_DOUBLE,
    ENTANGLE_CONFIG_ENTRY_BOOL,

    ENTANGLE_CONFIG_ENTRY_STRING_LIST,
    ENTANGLE_CONFIG_ENTRY_INT_LIST,
    ENTANGLE_CONFIG_ENTRY_DOUBLE_LIST,
    ENTANGLE_CONFIG_ENTRY_BOOL_LIST,

    ENTANGLE_CONFIG_ENTRY_STRING_HASH,
    ENTANGLE_CONFIG_ENTRY_INT_HASH,
    ENTANGLE_CONFIG_ENTRY_DOUBLE_HASH,
    ENTANGLE_CONFIG_ENTRY_BOOL_HASH,
} EntangleConfigEntryDatatype;

struct _EntangleConfigEntry
{
    GObject parent;

    EntangleConfigEntryPrivate *priv;
};

struct _EntangleConfigEntryClass
{
    GObjectClass parent_class;
};


GType entangle_config_entry_get_type(void) G_GNUC_CONST;

EntangleConfigEntry *entangle_config_entry_new(EntangleConfigEntryDatatype datatype,
                                       const char *name,
                                       const char *description);

EntangleConfigEntryDatatype entangle_config_entry_get_datatype(EntangleConfigEntry *entry);
const char *entangle_config_entry_get_name(EntangleConfigEntry *entry);
const char *entangle_config_entry_get_description(EntangleConfigEntry *entry);

G_END_DECLS

#endif /* __ENTANGLE_CONFIG_ENTRY_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
