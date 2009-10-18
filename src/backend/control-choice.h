/*
 *  Capa: Capa Assists Photograph Aquisition
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

#ifndef __CAPA_CONTROL_CHOICE_H__
#define __CAPA_CONTROL_CHOICE_H__

#include <glib-object.h>

#include "control.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL_CHOICE            (capa_control_choice_get_type ())
#define CAPA_CONTROL_CHOICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL_CHOICE, CapaControlChoice))
#define CAPA_CONTROL_CHOICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL_CHOICE, CapaControlChoiceClass))
#define CAPA_IS_CONTROL_CHOICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL_CHOICE))
#define CAPA_IS_CONTROL_CHOICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL_CHOICE))
#define CAPA_CONTROL_CHOICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL_CHOICE, CapaControlChoiceClass))


typedef struct _CapaControlChoice CapaControlChoice;
typedef struct _CapaControlChoicePrivate CapaControlChoicePrivate;
typedef struct _CapaControlChoiceClass CapaControlChoiceClass;

struct _CapaControlChoice
{
    CapaControl parent;

    CapaControlChoicePrivate *priv;
};

struct _CapaControlChoiceClass
{
    CapaControlClass parent_class;
};


GType capa_control_choice_get_type(void) G_GNUC_CONST;
CapaControlChoice* capa_control_choice_new(const char *path,
                                           int id,
                                           const char *label,
                                           const char *info);

void capa_control_choice_add_entry(CapaControlChoice *choice,
                                   const char *entry);

int capa_control_choice_entry_count(CapaControlChoice *choice);
const char *capa_control_choice_entry_get(CapaControlChoice *choice,
                                          int idx);

G_END_DECLS

#endif /* __CAPA_CONTROL_CHOICE_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
