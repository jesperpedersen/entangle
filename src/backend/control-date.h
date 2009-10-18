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

#ifndef __CAPA_CONTROL_DATE_H__
#define __CAPA_CONTROL_DATE_H__

#include <glib-object.h>

#include "control.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL_DATE            (capa_control_date_get_type ())
#define CAPA_CONTROL_DATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL_DATE, CapaControlDate))
#define CAPA_CONTROL_DATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL_DATE, CapaControlDateClass))
#define CAPA_IS_CONTROL_DATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL_DATE))
#define CAPA_IS_CONTROL_DATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL_DATE))
#define CAPA_CONTROL_DATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL_DATE, CapaControlDateClass))


typedef struct _CapaControlDate CapaControlDate;
typedef struct _CapaControlDatePrivate CapaControlDatePrivate;
typedef struct _CapaControlDateClass CapaControlDateClass;

struct _CapaControlDate
{
    CapaControl parent;

    CapaControlDatePrivate *priv;
};

struct _CapaControlDateClass
{
    CapaControlClass parent_class;
};


GType capa_control_date_get_type(void) G_GNUC_CONST;
CapaControlDate* capa_control_date_new(const char *path,
                                       int id,
                                       const char *label,
                                       const char *info);


G_END_DECLS

#endif /* __CAPA_CONTROL_DATE_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
