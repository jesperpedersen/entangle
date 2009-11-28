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

#define DEBUG_CMS 1

#define CAPA_COLOUR_PROFILE_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_COLOUR_PROFILE, CapaColourProfilePrivate))
#define CAPA_COLOUR_PROFILE_TRANSFORM_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CAPA_TYPE_COLOUR_PROFILE_TRANSFORM, CapaColourProfileTransformPrivate))

struct _CapaColourProfilePrivate {
    GByteArray *data;
    char *filename;
    cmsHPROFILE *profile;
    gboolean dirty;
};

struct _CapaColourProfileTransformPrivate {
    CapaColourProfile *srcProfile;
    CapaColourProfile *dstProfile;
    cmsHTRANSFORM transform;
};

G_DEFINE_TYPE(CapaColourProfile, capa_colour_profile, G_TYPE_OBJECT);
G_DEFINE_TYPE(CapaColourProfileTransform, capa_colour_profile_transform, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_FILENAME,
    PROP_DATA,
};

enum {
    PROP_00,
    PROP_SRC_PROFILE,
    PROP_DST_PROFILE,
};

static void capa_colour_profile_get_property(GObject *object,
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

        case PROP_DATA:
            g_value_set_boxed(value, priv->data);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_colour_profile_set_property(GObject *object,
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
            priv->dirty = TRUE;
            break;

        case PROP_DATA:
            if (priv->data)
                g_byte_array_unref(priv->data);
            priv->data = g_value_dup_boxed(value);
            priv->dirty = TRUE;
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void capa_colour_profile_transform_get_property(GObject *object,
                                                       guint prop_id,
                                                       GValue *value,
                                                       GParamSpec *pspec)
{
    CapaColourProfileTransform *picker = CAPA_COLOUR_PROFILE_TRANSFORM(object);
    CapaColourProfileTransformPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_SRC_PROFILE:
            g_value_set_object(value, priv->srcProfile);
            break;

        case PROP_DST_PROFILE:
            g_value_set_object(value, priv->dstProfile);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void capa_colour_profile_transform_set_property(GObject *object,
                                                       guint prop_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
    CapaColourProfileTransform *picker = CAPA_COLOUR_PROFILE_TRANSFORM(object);
    CapaColourProfileTransformPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_SRC_PROFILE:
            if (priv->srcProfile)
                g_object_unref(G_OBJECT(priv->srcProfile));
            priv->srcProfile = g_value_get_object(value);
            g_object_ref(G_OBJECT(priv->srcProfile));
            break;

        case PROP_DST_PROFILE:
            if (priv->dstProfile)
                g_object_unref(G_OBJECT(priv->dstProfile));
            priv->dstProfile = g_value_get_object(value);
            g_object_ref(G_OBJECT(priv->srcProfile));
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

    if (priv->data)
        g_byte_array_unref(priv->data);
    g_free(priv->filename);
    if (priv->profile)
        cmsCloseProfile(priv->profile);

    G_OBJECT_CLASS (capa_colour_profile_parent_class)->finalize (object);
}


static void capa_colour_profile_transform_finalize (GObject *object)
{
    CapaColourProfileTransform *profile = CAPA_COLOUR_PROFILE_TRANSFORM(object);
    CapaColourProfileTransformPrivate *priv = profile->priv;
    CAPA_DEBUG("Finalize profile transform");

    if (priv->srcProfile)
        g_object_unref(G_OBJECT(priv->srcProfile));
    if (priv->dstProfile)
        g_object_unref(G_OBJECT(priv->dstProfile));

    G_OBJECT_CLASS (capa_colour_profile_transform_parent_class)->finalize (object);
}


static void capa_colour_profile_class_init(CapaColourProfileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_colour_profile_finalize;
    object_class->get_property = capa_colour_profile_get_property;
    object_class->set_property = capa_colour_profile_set_property;

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
    g_object_class_install_property(object_class,
                                    PROP_DATA,
                                    g_param_spec_boxed("data",
                                                       "Profile data",
                                                       "Raw data for the profile",
                                                       G_TYPE_BYTE_ARRAY,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(CapaColourProfilePrivate));

    /* Stop lcms calling exit() */
    cmsErrorAction(LCMS_ERROR_IGNORE);
}

static void capa_colour_profile_transform_class_init(CapaColourProfileTransformClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = capa_colour_profile_transform_finalize;
    object_class->get_property = capa_colour_profile_transform_get_property;
    object_class->set_property = capa_colour_profile_transform_set_property;

    g_object_class_install_property(object_class,
                                    PROP_SRC_PROFILE,
                                    g_param_spec_object("src-profile",
                                                        "Source profile",
                                                        "Source profile",
                                                        CAPA_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_DST_PROFILE,
                                    g_param_spec_object("dst-profile",
                                                        "Destination Profile",
                                                        "Destination Profile",
                                                        CAPA_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));


    g_type_class_add_private(klass, sizeof(CapaColourProfileTransformPrivate));
}


CapaColourProfile *capa_colour_profile_new_file(const char *filename)
{
    return CAPA_COLOUR_PROFILE(g_object_new(CAPA_TYPE_COLOUR_PROFILE,
                                            "filename", filename,
                                            NULL));
}


CapaColourProfile *capa_colour_profile_new_data(GByteArray *data)
{
    return CAPA_COLOUR_PROFILE(g_object_new(CAPA_TYPE_COLOUR_PROFILE,
                                            "data", data,
                                            NULL));
}


static gboolean capa_colour_profile_load(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;

    if (!priv->dirty)
        return TRUE;

    if (priv->profile) {
        cmsCloseProfile(priv->profile);
        priv->profile = NULL;
    }
    if (priv->filename) {
        priv->profile = cmsOpenProfileFromFile(priv->filename, "r");
    } else if (priv->data) {
        priv->profile = cmsOpenProfileFromMem(priv->data->data, priv->data->len);
    } else {
        return FALSE;
    }

    priv->dirty = FALSE;
    return TRUE;
}

static void capa_colour_profile_init(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv;

    priv = profile->priv = CAPA_COLOUR_PROFILE_GET_PRIVATE(profile);

    memset(priv, 0, sizeof(*priv));
}

static void capa_colour_profile_transform_init(CapaColourProfileTransform *profile)
{
    CapaColourProfileTransformPrivate *priv;

    priv = profile->priv = CAPA_COLOUR_PROFILE_TRANSFORM_GET_PRIVATE(profile);

    memset(priv, 0, sizeof(*priv));
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

    if (!capa_colour_profile_load(profile))
        return NULL;

    data = cmsTakeProductName(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_product_desc(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!capa_colour_profile_load(profile))
        return NULL;

    data = cmsTakeProductDesc(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_product_info(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!capa_colour_profile_load(profile))
        return NULL;

    data = cmsTakeProductInfo(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_manufacturer(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!capa_colour_profile_load(profile))
        return NULL;

    data = cmsTakeManufacturer(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_model(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!capa_colour_profile_load(profile))
        return NULL;

    data = cmsTakeModel(priv->profile);
    return g_strdup(data);
}


char *capa_colour_profile_copyright(CapaColourProfile *profile)
{
    CapaColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!capa_colour_profile_load(profile))
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


CapaColourProfileTransform *capa_colour_profile_transform_new(CapaColourProfile *src,
                                                              CapaColourProfile *dst)
{
    return CAPA_COLOUR_PROFILE_TRANSFORM(g_object_new(CAPA_TYPE_COLOUR_PROFILE_TRANSFORM,
                                                      "src-profile", src,
                                                      "dst-profile", dst,
                                                      NULL));
}

GdkPixbuf *capa_colour_profile_transform_apply(CapaColourProfileTransform *trans,
                                               GdkPixbuf *srcpixbuf)
{
    CapaColourProfileTransformPrivate *priv = trans->priv;
    CapaColourProfilePrivate *srcpriv = priv->srcProfile->priv;
    CapaColourProfilePrivate *dstpriv = priv->dstProfile->priv;
    cmsHTRANSFORM transform;
    GdkPixbuf *dstpixbuf;
    guchar *srcpixels;
    guchar *dstpixels;
    int type = capa_colour_profile_pixel_type(srcpixbuf);
    int stride = gdk_pixbuf_get_rowstride(srcpixbuf);
    int height = gdk_pixbuf_get_height(srcpixbuf);
    int width = gdk_pixbuf_get_width(srcpixbuf);

    if (!capa_colour_profile_load(priv->srcProfile) ||
        !capa_colour_profile_load(priv->dstProfile))
        return NULL;

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
#if DEBUG_CMS
                       /* A crude hack which causes the colour transform
                        * to be applied to only half the image in a diagonal */
                       (int)((double)width * (1.0-(double)row/(double)height)));
#else
                       width);
#endif

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
