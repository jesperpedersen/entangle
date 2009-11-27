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

#include <config.h>

#include <lcms.h>

#include "internal.h"
#include "colour-profile.h"

#define CAPA_COLOUR_PROFILE_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_COLOUR_PROFILE, CapaColourProfilePrivate))

struct _CapaColourProfilePrivate {
    char *filename;
    cmsHPROFILE *profile;
};

G_DEFINE_TYPE(CapaColourProfile, capa_colour_profile, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_FILENAME,
};

static void capa_camera_get_property(GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
    CapaColourProfile *picker = CAPA_COLOUR_PROFILE(object);
    CapaColourProfilePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_FILENAME:
            g_value_set_string(value, priv->filename);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_camera_set_property(GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
    CapaColourProfile *picker = CAPA_COLOUR_PROFILE(object);
    CapaColourProfilePrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_FILENAME:
            g_free(priv->filename);
            priv->filename = g_value_dup_string(value);

            if (priv->profile)
                cmsCloseProfile(priv->profile);
            priv->profile = cmsOpenProfileFromFile(priv->filename, "r");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void capa_colour_profile_finalize (GObject *object)
{
    CapaColourProfile *profile = CAPA_COLOUR_PROFILE(object);
    CapaColourProfilePrivate *priv = profile->priv;
    CAPA_DEBUG("Finalize profile");

    g_free(priv->filename);
    if (priv->profile)
        cmsCloseProfile(priv->profile);

    G_OBJECT_CLASS (capa_colour_profile_parent_class)->finalize (object);
}


static void capa_colour_profile_class_init(CapaColourProfileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_colour_profile_finalize;
    object_class->get_property = capa_camera_get_property;
    object_class->set_property = capa_camera_set_property;

    g_object_class_install_property(object_class,
                                    PROP_FILENAME,
                                    g_param_spec_string("filename",
                                                        "Profile filename",
                                                        "Filename of the profile",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(CapaColourProfilePrivate));
}


CapaColourProfile *capa_colour_profile_new(const char *filename)
{
    return CAPA_COLOUR_PROFILE(g_object_new(CAPA_TYPE_COLOUR_PROFILE,
                                            "filename", filename,
                                            NULL));
}


static void capa_colour_profile_init(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv;

    priv = profile->priv = CAPA_COLOUR_PROFILE_GET_PRIVATE(profile);
}


const char *capa_colour_profile_filename(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    return priv->filename;
}


char *capa_colour_profile_product_name(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!priv->profile)
        return NULL;

    data = cmsTakeProductName(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_product_desc(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!priv->profile)
        return NULL;

    data = cmsTakeProductDesc(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_product_info(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!priv->profile)
        return NULL;

    data = cmsTakeProductInfo(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_manufacturer(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!priv->profile)
        return NULL;

    data = cmsTakeManufacturer(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_model(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!priv->profile)
        return NULL;

    data = cmsTakeModel(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_copyright(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!priv->profile)
        return NULL;

    data = cmsTakeCopyright(priv->profile);
    return g_strdup(data);
}


static int
capa_colour_profile_pixel_type(GdkPixbuf *pixbuf)
{
    int type = COLORSPACE_SH(PT_RGB); /* GdkPixbuf only supports RGB for now */

    /* XXX does n_channels already include the alpha channel ? */
    type |= CHANNELS_SH(gdk_pixbuf_get_n_channels(pixbuf) +
                        (gdk_pixbuf_get_has_alpha(pixbuf) ? 1 : 0));
    type |= BYTES_SH(gdk_pixbuf_get_bits_per_sample(pixbuf) / 8);

    return type;
}

GdkPixbuf *capa_colour_profile_convert(CapaColourProfile *srcprof,
				       CapaColourProfile *dstprof,
				       GdkPixbuf *srcpixbuf)
{
    CapaColourProfilePrivate *srcpriv = srcprof->priv;
    CapaColourProfilePrivate *dstpriv = dstprof->priv;
    cmsHTRANSFORM transform;
    GdkPixbuf *dstpixbuf;
    guchar *srcpixels;
    guchar *dstpixels;
    int type = capa_colour_profile_pixel_type(srcpixbuf);
    int stride = gdk_pixbuf_get_rowstride(srcpixbuf);
    int height = gdk_pixbuf_get_height(srcpixbuf);
    int width = gdk_pixbuf_get_width(srcpixbuf);

    dstpixbuf = gdk_pixbuf_copy(srcpixbuf);

    /* XXX any benefit in caching this transform by adding
     * a separate  GObject  CapaColourTransform ? */
    transform = cmsCreateTransform(srcpriv->profile,
                                   type,
                                   dstpriv->profile,
                                   type,
                                   INTENT_PERCEPTUAL,
                                   0);

    srcpixels = gdk_pixbuf_get_pixels(srcpixbuf);
    dstpixels = gdk_pixbuf_get_pixels(dstpixbuf);

    /* We do it row-wise, since lcms can't cope with a
     * rowstride that isn't equal to width */
    for (int row = 0 ; row < height ; row++)
        cmsDoTransform(transform,
                       srcpixels + (row * stride),
                       dstpixels + (row * stride),
                       width);

    cmsDeleteTransform(transform);

    return dstpixbuf;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
