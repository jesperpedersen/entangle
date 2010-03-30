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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "entangle-debug.h"
#include "entangle-thumbnail-loader.h"
#include "entangle-colour-profile.h"

#define ENTANGLE_THUMBNAIL_LOADER_GET_PRIVATE(obj)                                     \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_THUMBNAIL_LOADER, EntangleThumbnailLoaderPrivate))


struct _EntangleThumbnailLoaderPrivate {
    guint width;
    guint height;
};

G_DEFINE_TYPE(EntangleThumbnailLoader, entangle_thumbnail_loader, ENTANGLE_TYPE_PIXBUF_LOADER);

enum {
    PROP_0,
    PROP_WIDTH,
    PROP_HEIGHT,
};


static void entangle_thumbnail_loader_get_property(GObject *object,
					       guint prop_id,
					       GValue *value,
					       GParamSpec *pspec)
{
    EntangleThumbnailLoader *loader = ENTANGLE_THUMBNAIL_LOADER(object);
    EntangleThumbnailLoaderPrivate *priv = loader->priv;

    switch (prop_id)
        {
        case PROP_WIDTH:
            g_value_set_int(value, priv->width);
            break;

        case PROP_HEIGHT:
            g_value_set_int(value, priv->height);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_thumbnail_loader_set_property(GObject *object,
                                               guint prop_id,
                                               const GValue *value,
                                               GParamSpec *pspec)
{
    EntangleThumbnailLoader *loader = ENTANGLE_THUMBNAIL_LOADER(object);
    EntangleThumbnailLoaderPrivate *priv = loader->priv;

    switch (prop_id)
        {
        case PROP_WIDTH:
            priv->width = g_value_get_int(value);
            break;

        case PROP_HEIGHT:
            priv->height = g_value_get_int(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static char *entangle_thumbnail_loader_path_to_uri(const char *filename)
{
    char *path = g_uri_escape_string(filename,
                                     G_URI_RESERVED_CHARS_ALLOWED_IN_PATH,
                                     TRUE);

    return g_strdup_printf("file://%s", path);
}


static char *entangle_thumbnail_loader_uri_to_md5(const char *uri)
{
    return g_compute_checksum_for_data(G_CHECKSUM_MD5,
                                       (const guchar *)uri,
                                       strlen(uri));
}

static char *entangle_thumbnail_loader_uri_to_thumb(const char *uri)
{
    char *md5 = entangle_thumbnail_loader_uri_to_md5(uri);
    char *thumb;

    thumb = g_strdup_printf("%s/.thumbnails/normal/%s.png",
                            g_get_home_dir(), md5);
    g_free(md5);

    return thumb;
}


static GdkPixbuf *entangle_thumbnail_loader_generate(const char *filename,
                                                 const char *uri,
                                                 const char *thumbname,
                                                 time_t mtime)
{
    GdkPixbuf *master;
    GdkPixbuf *thumb = NULL;
    int sw, sh;
    int tw, th;
    int iw, ih;
    double ta;
    double ia;
    char mtimeStr[21];
    char widthStr[21];
    char heightStr[21];
    char *thumbnametmp;
    const char *orientationstr;

    master = gdk_pixbuf_new_from_file(filename, NULL);

    if (!master)
        return NULL;

    sw = sh = 128;
    iw = gdk_pixbuf_get_width(master);
    ih = gdk_pixbuf_get_height(master);

    ia = (double)iw / (double)ih;
    ta = (double)sw / (double)sh;

    if (ia > ta) {
        tw = sw;
        th = (int)((double)sw / ia);
    } else {
        tw = (int)((double)sh * ia);
        th = sh;
    }

    thumb = gdk_pixbuf_scale_simple(master, tw, th, GDK_INTERP_BILINEAR);

    g_snprintf(widthStr, sizeof(widthStr), "%d", iw);
    g_snprintf(heightStr, sizeof(heightStr), "%d", ih);
    g_snprintf(mtimeStr, sizeof(mtimeStr), "%ld", mtime);

    ENTANGLE_DEBUG("Image %dx%d -> %dx%d", iw, ih, tw, th);

    thumbnametmp = g_strdup_printf("%s.entangle-tmp", thumbname);

    orientationstr = gdk_pixbuf_get_option (master, "orientation");

    if (gdk_pixbuf_save(thumb, thumbnametmp,
                        "png", NULL,
                        "tEXt::Thumb::Image::Width", widthStr,
                        "tEXt::Thumb::Image::Height", heightStr,
                        "tEXt::Thumb::URI", uri,
                        "tEXt::Thumb::MTime", mtimeStr,
                        "tEXt::Software", "Entangle",
                        "tEXt::Entangle::Orientation", orientationstr ? orientationstr : "0",
                        NULL)) {
        chmod(thumbnametmp, 0600);
        if (rename(thumbnametmp, thumbname) < 0)
            unlink(thumbnametmp);
    }
    g_free(thumbnametmp);

    g_object_unref(master);

    return thumb;
}


static GdkPixbuf *entangle_thumbnail_loader_auto_rotate(GdkPixbuf *src)
{
    GdkPixbuf *dest = gdk_pixbuf_apply_embedded_orientation(src);
    GdkPixbuf *temp;

    g_object_unref(src);

    if (dest == src) {
        const char *orientationstr;
        int transform = 0;
        orientationstr = gdk_pixbuf_get_option(src, "tEXt::Entangle::Orientation");

        if (orientationstr)
            transform = (int)g_ascii_strtoll(orientationstr, NULL, 10);

        /* Apply the actual transforms, which involve rotations and flips.
           The meaning of orientation values 1-8 and the required transforms
           are defined by the TIFF and EXIF (for JPEGs) standards. */
        switch (transform) {
        case 1:
                dest = src;
                g_object_ref(dest);
                break;
        case 2:
                dest = gdk_pixbuf_flip(src, TRUE);
                break;
        case 3:
                dest = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
                break;
        case 4:
                dest = gdk_pixbuf_flip(src, FALSE);
                break;
        case 5:
                temp = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
                dest = gdk_pixbuf_flip(temp,TRUE);
                g_object_unref(temp);
                break;
        case 6:
                dest = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
                break;
        case 7:
                temp = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
                dest = gdk_pixbuf_flip(temp, FALSE);
                g_object_unref(temp);
                break;
        case 8:
                dest = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
                break;
        default:
                /* if no orientation tag was present */
                dest = src;
                g_object_ref(dest);
                break;
        }

    }

    return dest;
}

static GdkPixbuf *entangle_thumbnail_loader_pixbuf_load(EntanglePixbufLoader *loader,
                                                    const char *filename)
{
    EntangleThumbnailLoader *tloader = ENTANGLE_THUMBNAIL_LOADER(loader);
    EntangleThumbnailLoaderPrivate *priv = tloader->priv;
    char *uri = entangle_thumbnail_loader_path_to_uri(filename);
    char *thumbname = entangle_thumbnail_loader_uri_to_thumb(uri);
    GdkPixbuf *result = NULL;
    GdkPixbuf *thumb = NULL;
    struct stat sb;

    if (stat(filename, &sb) < 0) {
        ENTANGLE_DEBUG("File %s does not exist", filename);
        goto cleanup;
    }

    ENTANGLE_DEBUG("Want thumbnail %s for %s %ld", thumbname, uri, sb.st_mtime);

    thumb = gdk_pixbuf_new_from_file(thumbname, NULL);
    if (thumb) {
        const char *thisuri = gdk_pixbuf_get_option(thumb, "tEXt::Thumb::URI");
        const char *thismtimeStr = gdk_pixbuf_get_option(thumb, "tEXt::Thumb::MTime");
        time_t thismtime;

        thismtime = thismtimeStr ? atol(thismtimeStr) : 0;

        ENTANGLE_DEBUG("Check thumbnail %s for %s %s",
                   thumbname, thisuri, thismtimeStr);

        if (!thisuri || !thismtimeStr ||
            strcmp(uri, thisuri) != 0 ||
            thismtime != sb.st_mtime) {
            gdk_pixbuf_unref(thumb);
            thumb = NULL;
        }
    }


    if (!thumb) {
        unlink(thumbname);
        ENTANGLE_DEBUG("Generate thumbnail %s for %s %ld",
                   thumbname, uri, sb.st_mtime);
        thumb = entangle_thumbnail_loader_generate(filename, uri, thumbname,
                                               sb.st_mtime);
    }

    if (thumb) {
        GdkPixbuf *tmp = entangle_thumbnail_loader_auto_rotate(thumb);
        g_object_unref(thumb);
        thumb = tmp;
    }

    if (thumb) {
        int tw, th;

        tw = gdk_pixbuf_get_width(thumb);
        th = gdk_pixbuf_get_height(thumb);

        if (tw != priv->width &&
            th != priv->height) {
            double ta, ra;
            int rw, rh;
            double sw, sh;

            ta = (double)tw / (double)th;
            ra = (double)priv->width / (double)priv->height;

            if (ta > ra) {
                rw = priv->width;
                rh = (int)((double)priv->width / ta);
            } else {
                rw = (int)((double)priv->height * ta);
                rh = priv->height;
            }

            sw = (double)rw / (double)tw;
            sh = (double)rh / (double)th;
            result = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                                    priv->width, priv->height);
            gdk_pixbuf_fill(result, 0x000000FF);

            gdk_pixbuf_scale(thumb, result,
                             (priv->width - rw) / 2,
                             (priv->height - rh) / 2,
                             rw, rh,
                             (priv->width - rw) / 2,
                             (priv->height - rh) / 2,
                             sw, sh,
                             GDK_INTERP_BILINEAR);
            g_object_unref(thumb);
        } else {
            result = thumb;
        }
    }

 cleanup:
    g_free(uri);
    g_free(thumbname);

    return result;
}


static void entangle_thumbnail_loader_class_init(EntangleThumbnailLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    EntanglePixbufLoaderClass *loader_class = ENTANGLE_PIXBUF_LOADER_CLASS(klass);

    object_class->get_property = entangle_thumbnail_loader_get_property;
    object_class->set_property = entangle_thumbnail_loader_set_property;

    loader_class->pixbuf_load = entangle_thumbnail_loader_pixbuf_load;

    g_object_class_install_property(object_class,
                                    PROP_WIDTH,
                                    g_param_spec_int("width",
                                                     "Width",
                                                     "Maximum thumbnail width",
                                                     1, 400, 128,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB));
    g_object_class_install_property(object_class,
                                    PROP_HEIGHT,
                                    g_param_spec_int("height",
                                                     "Height",
                                                     "Maximum thumbnail height",
                                                     1, 400, 128,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleThumbnailLoaderPrivate));
}


EntangleThumbnailLoader *entangle_thumbnail_loader_new(int width,
                                               int height)
{
    return ENTANGLE_THUMBNAIL_LOADER(g_object_new(ENTANGLE_TYPE_THUMBNAIL_LOADER,
                                              "width", width,
                                              "height", height,
                                              NULL));
}


static void entangle_thumbnail_loader_init(EntangleThumbnailLoader *loader)
{
    EntangleThumbnailLoaderPrivate *priv;

    priv = loader->priv = ENTANGLE_THUMBNAIL_LOADER_GET_PRIVATE(loader);
    memset(priv, 0, sizeof(*priv));

    priv->width = priv->height = 128;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
