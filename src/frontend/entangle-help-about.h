/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2012 Daniel P. Berrange
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

#ifndef __ENTANGLE_HELP_ABOUT_H__
#define __ENTANGLE_HELP_ABOUT_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ENTANGLE_TYPE_HELP_ABOUT            (entangle_help_about_get_type ())
#define ENTANGLE_HELP_ABOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_HELP_ABOUT, EntangleHelpAbout))
#define ENTANGLE_HELP_ABOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_HELP_ABOUT, EntangleHelpAboutClass))
#define ENTANGLE_IS_HELP_ABOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_HELP_ABOUT))
#define ENTANGLE_IS_HELP_ABOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_HELP_ABOUT))
#define ENTANGLE_HELP_ABOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_HELP_ABOUT, EntangleHelpAboutClass))


typedef struct _EntangleHelpAbout EntangleHelpAbout;
typedef struct _EntangleHelpAboutPrivate EntangleHelpAboutPrivate;
typedef struct _EntangleHelpAboutClass EntangleHelpAboutClass;

struct _EntangleHelpAbout
{
    GObject parent;

    EntangleHelpAboutPrivate *priv;
};

struct _EntangleHelpAboutClass
{
    GObjectClass parent_class;

    void (*about_close)(EntangleHelpAbout *about);
};


GType entangle_help_about_get_type(void) G_GNUC_CONST;

EntangleHelpAbout* entangle_help_about_new(void);

GtkWindow *entangle_help_about_get_window(EntangleHelpAbout *about);

void entangle_help_about_show(EntangleHelpAbout *about);
void entangle_help_about_hide(EntangleHelpAbout *about);

G_END_DECLS

#endif /* __ENTANGLE_HELP_ABOUT_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
