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

#ifndef __CAPA_COLOUR_PROFILE_H__
#define __CAPA_COLOUR_PROFILE_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define CAPA_TYPE_COLOUR_PROFILE            (capa_colour_profile_get_type ())
#define CAPA_COLOUR_PROFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_COLOUR_PROFILE, CapaColourProfile))
#define CAPA_COLOUR_PROFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_COLOUR_PROFILE, CapaColourProfileClass))
#define CAPA_IS_COLOUR_PROFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_COLOUR_PROFILE))
#define CAPA_IS_COLOUR_PROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_COLOUR_PROFILE))
#define CAPA_COLOUR_PROFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_COLOUR_PROFILE, CapaColourProfileClass))


typedef struct _CapaColourProfile CapaColourProfile;
typedef struct _CapaColourProfilePrivate CapaColourProfilePrivate;
typedef struct _CapaColourProfileClass CapaColourProfileClass;

struct _CapaColourProfile
{
    GObject parent;

    CapaColourProfilePrivate *priv;
};

struct _CapaColourProfileClass
{
    GObjectClass parent_class;

};


GType capa_colour_profile_get_type(void) G_GNUC_CONST;
CapaColourProfile* capa_colour_profile_new(const char *filename);

char *capa_colour_profile_product_name(CapaColourProfile *profile);
char *capa_colour_profile_product_desc(CapaColourProfile *profile);
char *capa_colour_profile_product_info(CapaColourProfile *profile);
char *capa_colour_profile_manufacturer(CapaColourProfile *profile);
char *capa_colour_profile_model(CapaColourProfile *profile);
char *capa_colour_profile_copyright(CapaColourProfile *profile);

GdkPixbuf *capa_colour_profile_convert(CapaColourProfile *src,
				       CapaColourProfile *dst,
				       GdkPixbuf *pixbuf);

G_END_DECLS

#endif /* __CAPA_COLOUR_PROFILE_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
