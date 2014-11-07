/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2015 Daniel P. Berrange
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

#ifndef __ENTANGLE_CONTROL_DATE_H__
#define __ENTANGLE_CONTROL_DATE_H__

#include <glib-object.h>

#include "entangle-control.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONTROL_DATE            (entangle_control_date_get_type ())
#define ENTANGLE_CONTROL_DATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONTROL_DATE, EntangleControlDate))
#define ENTANGLE_CONTROL_DATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONTROL_DATE, EntangleControlDateClass))
#define ENTANGLE_IS_CONTROL_DATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONTROL_DATE))
#define ENTANGLE_IS_CONTROL_DATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONTROL_DATE))
#define ENTANGLE_CONTROL_DATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONTROL_DATE, EntangleControlDateClass))


typedef struct _EntangleControlDate EntangleControlDate;
typedef struct _EntangleControlDatePrivate EntangleControlDatePrivate;
typedef struct _EntangleControlDateClass EntangleControlDateClass;

struct _EntangleControlDate
{
    EntangleControl parent;

    EntangleControlDatePrivate *priv;
};

struct _EntangleControlDateClass
{
    EntangleControlClass parent_class;
};


GType entangle_control_date_get_type(void) G_GNUC_CONST;
EntangleControlDate* entangle_control_date_new(const char *path,
                                               int id,
                                               const char *label,
                                               const char *info,
                                               gboolean readonly);


G_END_DECLS

#endif /* __ENTANGLE_CONTROL_DATE_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
