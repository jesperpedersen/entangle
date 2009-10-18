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

#ifndef __CAPA_CONTROL_RANGE_H__
#define __CAPA_CONTROL_RANGE_H__

#include <glib-object.h>

#include "control.h"

G_BEGIN_DECLS

#define CAPA_TYPE_CONTROL_RANGE            (capa_control_range_get_type ())
#define CAPA_CONTROL_RANGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CONTROL_RANGE, CapaControlRange))
#define CAPA_CONTROL_RANGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CONTROL_RANGE, CapaControlRangeClass))
#define CAPA_IS_CONTROL_RANGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CONTROL_RANGE))
#define CAPA_IS_CONTROL_RANGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CONTROL_RANGE))
#define CAPA_CONTROL_RANGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CONTROL_RANGE, CapaControlRangeClass))


typedef struct _CapaControlRange CapaControlRange;
typedef struct _CapaControlRangePrivate CapaControlRangePrivate;
typedef struct _CapaControlRangeClass CapaControlRangeClass;

struct _CapaControlRange
{
    CapaControl parent;

    CapaControlRangePrivate *priv;
};

struct _CapaControlRangeClass
{
    CapaControlClass parent_class;
};


GType capa_control_range_get_type(void) G_GNUC_CONST;
CapaControlRange* capa_control_range_new(const char *path,
                                         int id,
                                         const char *label,
                                         const char *info,
                                         float min,
                                         float max,
                                         float step);


float capa_control_range_get_min(CapaControlRange *range);
float capa_control_range_get_max(CapaControlRange *range);
float capa_control_range_get_step(CapaControlRange *range);

G_END_DECLS

#endif /* __CAPA_CONTROL_RANGE_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
