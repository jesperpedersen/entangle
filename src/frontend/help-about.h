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

#ifndef __HELP_ABOUT__
#define __HELP_ABOUT__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_HELP_ABOUT            (capa_help_about_get_type ())
#define CAPA_HELP_ABOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_HELP_ABOUT, CapaHelpAbout))
#define CAPA_HELP_ABOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_HELP_ABOUT, CapaHelpAboutClass))
#define CAPA_IS_HELP_ABOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_HELP_ABOUT))
#define CAPA_IS_HELP_ABOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_HELP_ABOUT))
#define CAPA_HELP_ABOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_HELP_ABOUT, CapaHelpAboutClass))


typedef struct _CapaHelpAbout CapaHelpAbout;
typedef struct _CapaHelpAboutPrivate CapaHelpAboutPrivate;
typedef struct _CapaHelpAboutClass CapaHelpAboutClass;

struct _CapaHelpAbout
{
  GObject parent;

  CapaHelpAboutPrivate *priv;
};

struct _CapaHelpAboutClass
{
  GObjectClass parent_class;

  void (*about_close)(CapaHelpAbout *about);
};


GType capa_help_about_get_type(void) G_GNUC_CONST;

CapaHelpAbout* capa_help_about_new(void);

void capa_help_about_show(CapaHelpAbout *about);
void capa_help_about_hide(CapaHelpAbout *about);

G_END_DECLS

#endif /* __HELP_ABOUT__ */

