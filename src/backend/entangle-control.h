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

#ifndef __ENTANGLE_CONTROL_H__
#define __ENTANGLE_CONTROL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CONTROL            (entangle_control_get_type ())
#define ENTANGLE_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CONTROL, EntangleControl))
#define ENTANGLE_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CONTROL, EntangleControlClass))
#define ENTANGLE_IS_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CONTROL))
#define ENTANGLE_IS_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CONTROL))
#define ENTANGLE_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CONTROL, EntangleControlClass))


typedef struct _EntangleControl EntangleControl;
typedef struct _EntangleControlPrivate EntangleControlPrivate;
typedef struct _EntangleControlClass EntangleControlClass;

struct _EntangleControl
{
    GObject parent;

    EntangleControlPrivate *priv;
};

struct _EntangleControlClass
{
    GObjectClass parent_class;
};


GType entangle_control_get_type(void) G_GNUC_CONST;
EntangleControl* entangle_control_new(const gchar *path,
                                      gint id,
                                      const gchar *label,
                                      const gchar *info,
                                      gboolean readonly);


gint entangle_control_get_id(EntangleControl *control);
const gchar *entangle_control_get_path(EntangleControl *control);
const gchar *entangle_control_get_label(EntangleControl *control);
const gchar *entangle_control_get_info(EntangleControl *control);

gboolean entangle_control_get_dirty(EntangleControl *control);
void entangle_control_set_dirty(EntangleControl *control,
                                gboolean dirty);

gboolean entangle_control_get_readonly(EntangleControl *control);
void entangle_control_set_readonly(EntangleControl *control,
                                   gboolean ro);

G_END_DECLS

#endif /* __ENTANGLE_CONTROL_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
