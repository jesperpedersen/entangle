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

#ifndef __ENTANGLE_COLOUR_PROFILE_H__
#define __ENTANGLE_COLOUR_PROFILE_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "entangle-colour-profile-enums.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_COLOUR_PROFILE            (entangle_colour_profile_get_type ())
#define ENTANGLE_COLOUR_PROFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_COLOUR_PROFILE, EntangleColourProfile))
#define ENTANGLE_COLOUR_PROFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_COLOUR_PROFILE, EntangleColourProfileClass))
#define ENTANGLE_IS_COLOUR_PROFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_COLOUR_PROFILE))
#define ENTANGLE_IS_COLOUR_PROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_COLOUR_PROFILE))
#define ENTANGLE_COLOUR_PROFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_COLOUR_PROFILE, EntangleColourProfileClass))

#define ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM            (entangle_colour_profile_transform_get_type ())
#define ENTANGLE_COLOUR_PROFILE_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM, EntangleColourProfileTransform))
#define ENTANGLE_COLOUR_PROFILE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM, EntangleColourProfileTransformClass))
#define ENTANGLE_IS_COLOUR_PROFILE_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM))
#define ENTANGLE_IS_COLOUR_PROFILE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM))
#define ENTANGLE_COLOUR_PROFILE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM, EntangleColourProfileTransformClass))


typedef struct _EntangleColourProfile EntangleColourProfile;
typedef struct _EntangleColourProfilePrivate EntangleColourProfilePrivate;
typedef struct _EntangleColourProfileClass EntangleColourProfileClass;

typedef struct _EntangleColourProfileTransform EntangleColourProfileTransform;
typedef struct _EntangleColourProfileTransformPrivate EntangleColourProfileTransformPrivate;
typedef struct _EntangleColourProfileTransformClass EntangleColourProfileTransformClass;

struct _EntangleColourProfile
{
    GObject parent;

    EntangleColourProfilePrivate *priv;
};

struct _EntangleColourProfileClass
{
    GObjectClass parent_class;

};

struct _EntangleColourProfileTransform
{
    GObject parent;

    EntangleColourProfileTransformPrivate *priv;
};

struct _EntangleColourProfileTransformClass
{
    GObjectClass parent_class;

};


typedef enum {
    ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL,
    ENTANGLE_COLOUR_PROFILE_INTENT_REL_COLOURIMETRIC,
    ENTANGLE_COLOUR_PROFILE_INTENT_SATURATION,
    ENTANGLE_COLOUR_PROFILE_INTENT_ABS_COLOURIMETRIC,
} EntangleColourProfileIntent;

GType entangle_colour_profile_get_type(void) G_GNUC_CONST;
GType entangle_colour_profile_transform_get_type(void) G_GNUC_CONST;

EntangleColourProfile *entangle_colour_profile_new_file(const char *filename);
EntangleColourProfile *entangle_colour_profile_new_data(GByteArray *data);

const char *entangle_colour_profile_filename(EntangleColourProfile *profile);

char *entangle_colour_profile_product_name(EntangleColourProfile *profile);
char *entangle_colour_profile_product_desc(EntangleColourProfile *profile);
char *entangle_colour_profile_product_info(EntangleColourProfile *profile);
char *entangle_colour_profile_manufacturer(EntangleColourProfile *profile);
char *entangle_colour_profile_model(EntangleColourProfile *profile);
char *entangle_colour_profile_copyright(EntangleColourProfile *profile);

EntangleColourProfileTransform *entangle_colour_profile_transform_new(EntangleColourProfile *src,
                                                              EntangleColourProfile *dst,
                                                              EntangleColourProfileIntent intent);

GdkPixbuf *entangle_colour_profile_transform_apply(EntangleColourProfileTransform *trans,
                                               GdkPixbuf *pixbuf);

G_END_DECLS

#endif /* __ENTANGLE_COLOUR_PROFILE_H__ */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
