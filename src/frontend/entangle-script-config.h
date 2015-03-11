/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2014 Daniel P. Berrange
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

#ifndef __ENTANGLE_SCRIPT_CONFIG_H__
#define __ENTANGLE_SCRIPT_CONFIG_H__

#include <gtk/gtk.h>

#include "entangle-script.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_SCRIPT_CONFIG            (entangle_script_config_get_type ())
#define ENTANGLE_SCRIPT_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_SCRIPT_CONFIG, EntangleScriptConfig))
#define ENTANGLE_SCRIPT_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_SCRIPT_CONFIG, EntangleScriptConfigClass))
#define ENTANGLE_IS_SCRIPT_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_SCRIPT_CONFIG))
#define ENTANGLE_IS_SCRIPT_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_SCRIPT_CONFIG))
#define ENTANGLE_SCRIPT_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_SCRIPT_CONFIG, EntangleScriptConfigClass))


typedef struct _EntangleScriptConfig EntangleScriptConfig;
typedef struct _EntangleScriptConfigPrivate EntangleScriptConfigPrivate;
typedef struct _EntangleScriptConfigClass EntangleScriptConfigClass;

struct _EntangleScriptConfig
{
    GtkBox parent;

    EntangleScriptConfigPrivate *priv;
};

struct _EntangleScriptConfigClass
{
    GtkBoxClass parent_class;

};

GType entangle_script_config_get_type(void) G_GNUC_CONST;

EntangleScriptConfig* entangle_script_config_new(void);

void entangle_script_config_add_script(EntangleScriptConfig *config,
                                       EntangleScript *script);
void entangle_script_config_remove_script(EntangleScriptConfig *config,
                                          EntangleScript *script);

gboolean entangle_script_config_has_scripts(EntangleScriptConfig *config);

EntangleScript *entangle_script_config_get_selected(EntangleScriptConfig *config);

G_END_DECLS

#endif /* __ENTANGLE_SCRIPT_CONFIG_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
