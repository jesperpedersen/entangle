/*
 *  Entangle: Entangle Assists Photograph Aquisition
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

#include "entangle-debug.h"
#include "entangle-colour-profile.h"

#define DEBUG_CMS 0

#define ENTANGLE_COLOUR_PROFILE_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_COLOUR_PROFILE, EntangleColourProfilePrivate))
#define ENTANGLE_COLOUR_PROFILE_TRANSFORM_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM, EntangleColourProfileTransformPrivate))

struct _EntangleColourProfilePrivate {
    GMutex *lock;
    GByteArray *data;
    char *filename;
    cmsHPROFILE *profile;
    gboolean dirty;
};

struct _EntangleColourProfileTransformPrivate {
    EntangleColourProfile *srcProfile;
    EntangleColourProfile *dstProfile;
    EntangleColourProfileIntent renderIntent;
};

G_DEFINE_TYPE(EntangleColourProfile, entangle_colour_profile, G_TYPE_OBJECT);
G_DEFINE_TYPE(EntangleColourProfileTransform, entangle_colour_profile_transform, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_FILENAME,
    PROP_DATA,
};

enum {
    PROP_00,
    PROP_SRC_PROFILE,
    PROP_DST_PROFILE,
    PROP_RENDERING_INTENT,
};

static void entangle_colour_profile_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
    EntangleColourProfile *picker = ENTANGLE_COLOUR_PROFILE(object);
    EntangleColourProfilePrivate *priv = picker->priv;

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

static void entangle_colour_profile_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
    EntangleColourProfile *picker = ENTANGLE_COLOUR_PROFILE(object);
    EntangleColourProfilePrivate *priv = picker->priv;

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


static void entangle_colour_profile_transform_get_property(GObject *object,
                                                       guint prop_id,
                                                       GValue *value,
                                                       GParamSpec *pspec)
{
    EntangleColourProfileTransform *picker = ENTANGLE_COLOUR_PROFILE_TRANSFORM(object);
    EntangleColourProfileTransformPrivate *priv = picker->priv;

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

static void entangle_colour_profile_transform_set_property(GObject *object,
                                                       guint prop_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec)
{
    EntangleColourProfileTransform *picker = ENTANGLE_COLOUR_PROFILE_TRANSFORM(object);
    EntangleColourProfileTransformPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_SRC_PROFILE:
            if (priv->srcProfile)
                g_object_unref(priv->srcProfile);
            priv->srcProfile = g_value_get_object(value);
            if (priv->srcProfile)
                g_object_ref(priv->srcProfile);
            break;

        case PROP_DST_PROFILE:
            if (priv->dstProfile)
                g_object_unref(priv->dstProfile);
            priv->dstProfile = g_value_get_object(value);
            if (priv->dstProfile)
                g_object_ref(priv->dstProfile);
            break;

        case PROP_RENDERING_INTENT:
            priv->renderIntent = g_value_get_enum(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void entangle_colour_profile_finalize (GObject *object)
{
    EntangleColourProfile *profile = ENTANGLE_COLOUR_PROFILE(object);
    EntangleColourProfilePrivate *priv = profile->priv;
    ENTANGLE_DEBUG("Finalize profile");

    if (priv->data)
        g_byte_array_unref(priv->data);
    g_free(priv->filename);
    if (priv->profile)
        cmsCloseProfile(priv->profile);
    g_mutex_free(priv->lock);

    G_OBJECT_CLASS (entangle_colour_profile_parent_class)->finalize (object);
}


static void entangle_colour_profile_transform_finalize (GObject *object)
{
    EntangleColourProfileTransform *profile = ENTANGLE_COLOUR_PROFILE_TRANSFORM(object);
    EntangleColourProfileTransformPrivate *priv = profile->priv;
    ENTANGLE_DEBUG("Finalize profile transform");

    if (priv->srcProfile)
        g_object_unref(priv->srcProfile);
    if (priv->dstProfile)
        g_object_unref(priv->dstProfile);

    G_OBJECT_CLASS (entangle_colour_profile_transform_parent_class)->finalize (object);
}


static void entangle_colour_profile_class_init(EntangleColourProfileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_colour_profile_finalize;
    object_class->get_property = entangle_colour_profile_get_property;
    object_class->set_property = entangle_colour_profile_set_property;

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


    g_type_class_add_private(klass, sizeof(EntangleColourProfilePrivate));

    /* Stop lcms calling exit() */
    cmsErrorAction(LCMS_ERROR_IGNORE);
}

static void entangle_colour_profile_transform_class_init(EntangleColourProfileTransformClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_colour_profile_transform_finalize;
    object_class->get_property = entangle_colour_profile_transform_get_property;
    object_class->set_property = entangle_colour_profile_transform_set_property;

    g_object_class_install_property(object_class,
                                    PROP_SRC_PROFILE,
                                    g_param_spec_object("src-profile",
                                                        "Source profile",
                                                        "Source profile",
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
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
                                                        ENTANGLE_TYPE_COLOUR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_RENDERING_INTENT,
                                    g_param_spec_enum("rendering-intent",
                                                      "Rendering intent",
                                                      "Profile rendering intent",
                                                      ENTANGLE_TYPE_COLOUR_PROFILE_INTENT,
                                                      ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleColourProfileTransformPrivate));
}


EntangleColourProfile *entangle_colour_profile_new_file(const char *filename)
{
    return ENTANGLE_COLOUR_PROFILE(g_object_new(ENTANGLE_TYPE_COLOUR_PROFILE,
                                            "filename", filename,
                                            NULL));
}


EntangleColourProfile *entangle_colour_profile_new_data(GByteArray *data)
{
    return ENTANGLE_COLOUR_PROFILE(g_object_new(ENTANGLE_TYPE_COLOUR_PROFILE,
                                            "data", data,
                                            NULL));
}


static gboolean entangle_colour_profile_load(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    gboolean ret;

    g_mutex_lock(priv->lock);

    if (!priv->dirty) {
        goto cleanup;
    }

    if (priv->profile) {
        cmsCloseProfile(priv->profile);
        priv->profile = NULL;
    }
    if (priv->filename) {
        if (!(priv->profile = cmsOpenProfileFromFile(priv->filename, "r")))
            ENTANGLE_DEBUG("Unable to load profile from %s", priv->filename);
    } else if (priv->data) {
        if (!(priv->profile = cmsOpenProfileFromMem(priv->data->data, priv->data->len)))
            ENTANGLE_DEBUG("Unable to load profile from %p", priv->data);
    }

    priv->dirty = FALSE;

 cleanup:
    ret = priv->profile ? TRUE : FALSE;
    g_mutex_unlock(priv->lock);
    return ret;
}

static void entangle_colour_profile_init(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv;

    priv = profile->priv = ENTANGLE_COLOUR_PROFILE_GET_PRIVATE(profile);
    priv->lock = g_mutex_new();
}

static void entangle_colour_profile_transform_init(EntangleColourProfileTransform *profile)
{
    EntangleColourProfileTransformPrivate *priv;

    priv = profile->priv = ENTANGLE_COLOUR_PROFILE_TRANSFORM_GET_PRIVATE(profile);

    memset(priv, 0, sizeof(*priv));
}


const char *entangle_colour_profile_filename(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    return priv->filename;
}


char *entangle_colour_profile_product_name(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!entangle_colour_profile_load(profile))
        return NULL;

    data = cmsTakeProductName(priv->profile);
    return g_strdup(data);
}


char *entangle_colour_profile_product_desc(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!entangle_colour_profile_load(profile))
        return NULL;

    data = cmsTakeProductDesc(priv->profile);
    return g_strdup(data);
}


char *entangle_colour_profile_product_info(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!entangle_colour_profile_load(profile))
        return NULL;

    data = cmsTakeProductInfo(priv->profile);
    return g_strdup(data);
}


char *entangle_colour_profile_manufacturer(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!entangle_colour_profile_load(profile))
        return NULL;

    data = cmsTakeManufacturer(priv->profile);
    return g_strdup(data);
}


char *entangle_colour_profile_model(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!entangle_colour_profile_load(profile))
        return NULL;

    data = cmsTakeModel(priv->profile);
    return g_strdup(data);
}


char *entangle_colour_profile_copyright(EntangleColourProfile *profile)
{
    EntangleColourProfilePrivate *priv = profile->priv;
    const char *data;

    if (!entangle_colour_profile_load(profile))
        return NULL;

    data = cmsTakeCopyright(priv->profile);
    return g_strdup(data);
}


static int
entangle_colour_profile_pixel_type(GdkPixbuf *pixbuf)
{
    int type = COLORSPACE_SH(PT_RGB); /* GdkPixbuf only supports RGB for now */

    if (gdk_pixbuf_get_has_alpha(pixbuf)) {
        type |= CHANNELS_SH(gdk_pixbuf_get_n_channels(pixbuf)-1);
        type |= EXTRA_SH(1);
    } else {
        type |= CHANNELS_SH(gdk_pixbuf_get_n_channels(pixbuf));
    }
    type |= BYTES_SH(gdk_pixbuf_get_bits_per_sample(pixbuf) / 8);

    return type;
}


EntangleColourProfileTransform *entangle_colour_profile_transform_new(EntangleColourProfile *src,
                                                              EntangleColourProfile *dst,
                                                              EntangleColourProfileIntent intent)
{
    return ENTANGLE_COLOUR_PROFILE_TRANSFORM(g_object_new(ENTANGLE_TYPE_COLOUR_PROFILE_TRANSFORM,
                                                      "src-profile", src,
                                                      "dst-profile", dst,
                                                      "rendering-intent", intent,
                                                      NULL));
}

GdkPixbuf *entangle_colour_profile_transform_apply(EntangleColourProfileTransform *trans,
                                               GdkPixbuf *srcpixbuf)
{
    EntangleColourProfileTransformPrivate *priv = trans->priv;
    EntangleColourProfilePrivate *srcpriv = priv->srcProfile->priv;
    EntangleColourProfilePrivate *dstpriv = priv->dstProfile->priv;
    cmsHTRANSFORM transform;
    GdkPixbuf *dstpixbuf;
    guchar *srcpixels;
    guchar *dstpixels;
    int type = entangle_colour_profile_pixel_type(srcpixbuf);
    int stride = gdk_pixbuf_get_rowstride(srcpixbuf);
    int height = gdk_pixbuf_get_height(srcpixbuf);
    int width = gdk_pixbuf_get_width(srcpixbuf);
    int intent = INTENT_PERCEPTUAL;

    if (!priv->srcProfile ||
        !priv->dstProfile) {
        g_object_ref(srcpixbuf);
        return srcpixbuf;
    }

    if (!entangle_colour_profile_load(priv->srcProfile) ||
        !entangle_colour_profile_load(priv->dstProfile)) {
        g_object_ref(srcpixbuf);
        return srcpixbuf;
    }

    dstpixbuf = gdk_pixbuf_copy(srcpixbuf);

    switch (priv->renderIntent) {
    case ENTANGLE_COLOUR_PROFILE_INTENT_PERCEPTUAL:
        intent = INTENT_PERCEPTUAL;
        break;
    case ENTANGLE_COLOUR_PROFILE_INTENT_REL_COLOURIMETRIC:
        intent = INTENT_RELATIVE_COLORIMETRIC;
        break;
    case ENTANGLE_COLOUR_PROFILE_INTENT_SATURATION:
        intent = INTENT_SATURATION;
        break;
    case ENTANGLE_COLOUR_PROFILE_INTENT_ABS_COLOURIMETRIC:
        intent = INTENT_ABSOLUTE_COLORIMETRIC;
        break;
    }

    transform = cmsCreateTransform(srcpriv->profile,
                                   type,
                                   dstpriv->profile,
                                   type,
                                   intent,
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
