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

#include <config.h>

#include <libraw/libraw.h>

#include "entangle-debug.h"
#include "entangle-pixbuf.h"

/**
 * entangle_pixbuf_auto_rotate:
 * @src: (transfer none): the pixbuf to be rotated
 * @metadata: (allow-none)(transfer none): the exiv2 metadata for the pixbuf
 *
 * Automatically rotate the pixbuf @src so that it is in its
 * "natural" orientation. Will first try to apply the orientation
 * associated in the GdkPixbuf object. If this doesn't apply any
 * rotation, then will use the exiv metadata to identify the
 * orientation
 *
 * Returns: (transfer full): the rotated pixbuf
 */
GdkPixbuf *entangle_pixbuf_auto_rotate(GdkPixbuf *src,
                                       GExiv2Metadata *metadata)
{
    GdkPixbuf *dest = gdk_pixbuf_apply_embedded_orientation(src);
    GdkPixbuf *temp;

    ENTANGLE_DEBUG("Auto-rotate %p %p\n", src, dest);

    if (dest == src) {
        int transform = 0;

        g_object_unref(dest);

        if (metadata) {
            transform = gexiv2_metadata_get_orientation(metadata);
        } else {
            const char *orientationstr = gdk_pixbuf_get_option(src, "tEXt::Entangle::Orientation");

            /* If not option, then try the gobject data slot */
            if (!orientationstr)
                orientationstr = g_object_get_data(G_OBJECT(src),
                                                   "tEXt::Entangle::Orientation");

            if (orientationstr)
                transform = (int)g_ascii_strtoll(orientationstr, NULL, 10);

            ENTANGLE_DEBUG("Auto-rotate %s\n", orientationstr);
        }

        /* Apply the actual transforms, which involve rotations and flips.
           The meaning of orientation values 1-8 and the required transforms
           are defined by the TIFF and EXIF (for JPEGs) standards. */
        switch (transform) {
        case GEXIV2_ORIENTATION_NORMAL:
            dest = src;
            g_object_ref(dest);
            break;
        case GEXIV2_ORIENTATION_HFLIP:
            dest = gdk_pixbuf_flip(src, TRUE);
            break;
        case GEXIV2_ORIENTATION_ROT_180:
            dest = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
            break;
        case GEXIV2_ORIENTATION_VFLIP:
            dest = gdk_pixbuf_flip(src, FALSE);
            break;
        case GEXIV2_ORIENTATION_ROT_90_HFLIP:
            temp = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
            dest = gdk_pixbuf_flip(temp,TRUE);
            g_object_unref(temp);
            break;
        case GEXIV2_ORIENTATION_ROT_90:
            dest = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
            break;
        case GEXIV2_ORIENTATION_ROT_90_VFLIP:
            temp = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
            dest = gdk_pixbuf_flip(temp, FALSE);
            g_object_unref(temp);
            break;
        case GEXIV2_ORIENTATION_ROT_270:
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

static gboolean entangle_pixbuf_is_raw(EntangleImage *image)
{
    const char *extlist[] = {
        ".cr2", ".nef", ".nrw", ".arw", ".orf", ".dng", ".pef",
        ".crw", ".erf", ".mrw", ".raw", ".rw2", ".raf", NULL
    };
    const char **tmp;
    char *filename = g_utf8_strdown(entangle_image_get_filename(image), -1);
    gboolean ret = TRUE;

    tmp = extlist;
    while (*tmp) {
        const char *ext = *tmp;

        if (g_str_has_suffix(filename, ext))
            goto cleanup;

        tmp++;
    }

    ret = FALSE;
 cleanup:
    g_free(filename);
    return ret;
}


static void img_free(guchar *ignore G_GNUC_UNUSED, gpointer opaque)
{
    libraw_processed_image_t *img = opaque;
    libraw_dcraw_clear_mem(img);
}


static GdkPixbuf *entangle_pixbuf_open_image_master_raw(EntangleImage *image)
{
    GdkPixbuf *result = NULL;
    libraw_data_t *raw = libraw_init(0);
    libraw_processed_image_t *img = NULL;
    int ret;

    if (!raw) {
        ENTANGLE_DEBUG("Failed to initialize libraw");
        goto cleanup;
    }

    /*
      Use default camera parameters to display image
      Real work needs to done by a RAW developer anyway
    */
    raw->params.use_camera_wb = 1;
    raw->params.use_auto_wb = 1;
    raw->params.no_auto_bright = 1;
    raw->params.use_camera_matrix = 1;
    raw->params.user_qual = 0;
    raw->params.fbdd_noiserd = 1;

    ENTANGLE_DEBUG("Open raw %s", entangle_image_get_filename(image));
    if ((ret = libraw_open_file(raw, entangle_image_get_filename(image))) != 0) {
        ENTANGLE_DEBUG("Failed to open raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    ENTANGLE_DEBUG("Unpack raw %s", entangle_image_get_filename(image));
    if ((ret = libraw_unpack(raw)) != 0) {
        ENTANGLE_DEBUG("Failed to unpack raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    ENTANGLE_DEBUG("Process raw %s", entangle_image_get_filename(image));
    if ((ret = libraw_dcraw_process(raw)) != 0) {
        ENTANGLE_DEBUG("Failed to process raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    ENTANGLE_DEBUG("Make mem %s", entangle_image_get_filename(image));
    if ((img = libraw_dcraw_make_mem_image(raw, &ret)) == NULL) {
        ENTANGLE_DEBUG("Failed to extract raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    ENTANGLE_DEBUG("New pixbuf %s", entangle_image_get_filename(image));
    result = gdk_pixbuf_new_from_data(img->data,
                                      GDK_COLORSPACE_RGB,
                                      FALSE, img->bits,
                                      img->width, img->height,
                                      (img->width * img->bits * 3)/8,
                                      img_free, img);

 cleanup:
    libraw_close(raw);
    return result;
}


static GdkPixbuf *entangle_pixbuf_open_image_master_gdk(EntangleImage *image,
                                                        GExiv2Metadata *metadata,
                                                        gboolean applyOrientation)
{
    GdkPixbuf *master;
    GdkPixbuf *result;

    ENTANGLE_DEBUG("Loading %s using GDK Pixbuf", entangle_image_get_filename(image));
    master = gdk_pixbuf_new_from_file(entangle_image_get_filename(image), NULL);

    if (!master)
        return NULL;

    if (applyOrientation) {
        result = entangle_pixbuf_auto_rotate(master, metadata);
        g_object_unref(master);
    } else {
        GExiv2Orientation orient = gexiv2_metadata_get_orientation(metadata);
        /* gdk_pixbuf_save doesn't update internal options and there
           is no set_option method, so abuse gobject data slots :-( */
        g_object_set_data_full(G_OBJECT(master),
                               "tEXt::Entangle::Orientation",
                               g_strdup_printf("%d", orient),
                               g_free);
        result = master;
    }

    return result;
}


static GdkPixbuf *entangle_pixbuf_open_image_master(EntangleImage *image,
                                                    GExiv2Metadata *metadata,
                                                    gboolean applyOrientation)
{
    if (entangle_pixbuf_is_raw(image))
        return entangle_pixbuf_open_image_master_raw(image);
    else
        return entangle_pixbuf_open_image_master_gdk(image, metadata, applyOrientation);
}


static GExiv2PreviewProperties *
entangle_pixbuf_get_largest_preview(GExiv2PreviewProperties **proplist)
{
    GExiv2PreviewProperties *best = NULL;
    gint bestw = 0, besth = 0;

    while (proplist && *proplist) {
        gint w, h;
        w = gexiv2_preview_properties_get_width(*proplist);
        h = gexiv2_preview_properties_get_height(*proplist);
        if (!best || ((w > bestw) && (h > besth))) {
            best = *proplist;
            bestw = w;
            besth = h;
        }
        proplist++;
    }
    ENTANGLE_DEBUG("Largest preview %p %dx%d", best, bestw, besth);
    return best;
}


static GExiv2PreviewProperties *
entangle_pixbuf_get_closest_preview(GExiv2PreviewProperties **proplist,
                                    guint minSize)
{
    GExiv2PreviewProperties *best = NULL;
    gint bestw = 0, besth = 0;

    while (proplist && *proplist) {
        gint w, h;
        w = gexiv2_preview_properties_get_width(*proplist);
        h = gexiv2_preview_properties_get_height(*proplist);
        ENTANGLE_DEBUG("Check %dx%d vs %d (best %p %dx%d",
                       w, h, minSize, best, bestw, besth);
        if (w > minSize && h > minSize) {
            if (!best ||
                ((w < bestw) && (h < besth))) {
                best = *proplist;
                bestw = w;
                besth = h;
            }
        }
        proplist++;
    }
    ENTANGLE_DEBUG("Closest preview %p %dx%d", best, bestw, besth);
    return best;
}


static GdkPixbuf *entangle_pixbuf_open_image_preview_raw(EntangleImage *image,
                                                         GExiv2Metadata *metadata,
                                                         gboolean applyOrientation)
{
    GdkPixbuf *result = NULL;
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    libraw_data_t *raw = libraw_init(0);
    libraw_processed_image_t *img = NULL;
    int ret;

    if (!raw) {
        ENTANGLE_DEBUG("Failed to initialize libraw");
        goto cleanup;
    }

    ENTANGLE_DEBUG("Open preview raw %s", entangle_image_get_filename(image));
    if ((ret = libraw_open_file(raw, entangle_image_get_filename(image))) != 0) {
        ENTANGLE_DEBUG("Failed to open preview raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    ENTANGLE_DEBUG("Unpack preview raw %s", entangle_image_get_filename(image));
    if ((ret = libraw_unpack_thumb(raw)) != 0) {
        ENTANGLE_DEBUG("Failed to unpack preview raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    ENTANGLE_DEBUG("Adjust preview raw %s", entangle_image_get_filename(image));
    if ((ret = libraw_adjust_sizes_info_only(raw)) != 0) {
        ENTANGLE_DEBUG("Failed to adjust preview raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    ENTANGLE_DEBUG("Make preview mem %s", entangle_image_get_filename(image));
    if ((img = libraw_dcraw_make_mem_thumb(raw, &ret)) == NULL) {
        ENTANGLE_DEBUG("Failed to extract preview raw file: %s",
                       libraw_strerror(ret));
        goto cleanup;
    }

    gdk_pixbuf_loader_write(loader, img->data, img->data_size, NULL);
    result = gdk_pixbuf_loader_get_pixbuf(loader);

    if (result) {
        if (applyOrientation) {
            GdkPixbuf *tmp = entangle_pixbuf_auto_rotate(result, metadata);
            g_object_unref(result);
            result = tmp;
        } else {
            GExiv2Orientation orient = gexiv2_metadata_get_orientation(metadata);
            /* gdk_pixbuf_save doesn't update internal options and there
               is no set_option method, so abuse gobject data slots :-( */
            g_object_set_data_full(G_OBJECT(result),
                                   "tEXt::Entangle::Orientation",
                                   g_strdup_printf("%d", orient),
                                   g_free);
        }
    }


 cleanup:
    if (img)
        libraw_dcraw_clear_mem(img);

    libraw_close(raw);

    gdk_pixbuf_loader_close(loader, NULL);

    return result;
}


static GdkPixbuf *entangle_pixbuf_open_image_preview_exiv(EntangleImage *image,
                                                          guint minSize,
                                                          GExiv2Metadata *metadata)
{
    GExiv2PreviewImage *preview = NULL;
    GExiv2PreviewProperties **props;
    GExiv2PreviewProperties *best;
    GdkPixbuf *result = NULL, *master = NULL;
    GdkPixbufLoader *loader = NULL;
    const guint8 *data;
    guint32 datalen;
    GExiv2Orientation orient;

    ENTANGLE_DEBUG("Opening preview %s", entangle_image_get_filename(image));

    props = gexiv2_metadata_get_preview_properties(metadata);

    if (minSize)
        best = entangle_pixbuf_get_closest_preview(props, minSize);
    else
        best = entangle_pixbuf_get_largest_preview(props);
    if (!best) {
        ENTANGLE_DEBUG("No preview properties for %s", entangle_image_get_filename(image));
        goto cleanup;
    }

    preview = gexiv2_metadata_get_preview_image(metadata, best);

    loader = gdk_pixbuf_loader_new_with_mime_type(gexiv2_preview_image_get_mime_type(preview),
                                                  NULL);

    data = gexiv2_preview_image_get_data(preview, &datalen);
    gdk_pixbuf_loader_write(loader,
                            data, datalen,
                            NULL);

    if (!gdk_pixbuf_loader_close(loader, NULL)) {
        ENTANGLE_DEBUG("Failed to load preview image for %s", entangle_image_get_filename(image));
        goto cleanup;
    }

    if (!(master = gdk_pixbuf_loader_get_pixbuf(loader))) {
        ENTANGLE_DEBUG("Failed to parse preview for %s", entangle_image_get_filename(image));
        goto cleanup;
    }

    orient = gexiv2_metadata_get_orientation(metadata);
    /* gdk_pixbuf_save doesn't update internal options and there
       is no set_option method, so abuse gobject data slots :-( */
    g_object_set_data_full(G_OBJECT(master),
                           "tEXt::Entangle::Orientation",
                           g_strdup_printf("%d", orient),
                           g_free);

 cleanup:
    if (loader)
        g_object_unref(loader);
    if (preview)
        g_object_unref(preview);
    return result;
}


static GdkPixbuf *entangle_pixbuf_open_image_preview(EntangleImage *image,
                                                     GExiv2Metadata *metadata,
                                                     gboolean applyOrientation)
{
    GdkPixbuf *result = NULL;
    if (entangle_pixbuf_is_raw(image)) {
        result = entangle_pixbuf_open_image_preview_raw(image, metadata, applyOrientation);
        if (!result && metadata)
            result = entangle_pixbuf_open_image_preview_exiv(image, 256, metadata);
        if (!result)
            result = entangle_pixbuf_open_image_master_raw(image);
    } else {
        result = entangle_pixbuf_open_image_master_gdk(image, metadata, applyOrientation);
    }
    return result;
}


static GdkPixbuf *entangle_pixbuf_open_image_thumbnail(EntangleImage *image,
                                                       GExiv2Metadata *metadata,
                                                       gboolean applyOrientation)
{
    GdkPixbuf *result = NULL;
    if (entangle_pixbuf_is_raw(image))
        result = entangle_pixbuf_open_image_preview_raw(image, metadata, applyOrientation);
    if (!result && metadata)
        result = entangle_pixbuf_open_image_preview_exiv(image, 128, metadata);
    if (!result)
        result = entangle_pixbuf_open_image_master(image, metadata, applyOrientation);
    return result;
}


/**
 * entangle_pixbuf_open_image:
 * @image: the camera image to open
 * @slot: the type of image data to open
 * @applyOrientation: whether to rotate to natural orientation
 * @metadata: (allow-none)(transfer full): filled with metadata object instance
 *
 * If @slot is ENTANGLE_PIXBUF_IMAGE_SLOT_MASTER then the primary
 * image data is loaded.
 *
 * If @slot is ENTANGLE_PIXBUF_IMAGE_SLOT_PREVIEW and the image is
 * a raw file, any embedded preview data is loaded. For non-raw
 * files the primary image data is loaded.
 *
 * If @slot is ENTANGLE_PIXBUF_IMAGE_SLOT_THUMBNAIL and the image is
 * a raw file, any embedded thumbnail data is loaded. For non-raw
 * files any thumbnail in the exiv2 metadata is loaded. If no thumbnail
 * is available, the primary image data is loaded.
 *
 * Returns: (transfer full): the pixbuf for the image slot
 */
GdkPixbuf *entangle_pixbuf_open_image(EntangleImage *image,
                                      EntanglePixbufImageSlot slot,
                                      gboolean applyOrientation,
                                      GExiv2Metadata **metadata)
{
    ENTANGLE_DEBUG("Open image %s %d", entangle_image_get_filename(image), slot);
    GExiv2Metadata *themetadata = gexiv2_metadata_new();
    GdkPixbuf *ret = NULL;

    if (!gexiv2_metadata_open_path(themetadata, entangle_image_get_filename(image), NULL)) {
        g_object_unref(themetadata);
        themetadata = NULL;
    }

    switch (slot) {
    case ENTANGLE_PIXBUF_IMAGE_SLOT_MASTER:
        ret = entangle_pixbuf_open_image_master(image, themetadata, applyOrientation);
        break;

    case ENTANGLE_PIXBUF_IMAGE_SLOT_PREVIEW:
        ret = entangle_pixbuf_open_image_preview(image, themetadata, applyOrientation);
        break;

    case ENTANGLE_PIXBUF_IMAGE_SLOT_THUMBNAIL:
        ret = entangle_pixbuf_open_image_thumbnail(image, themetadata, applyOrientation);
        break;

    default:
        g_warn_if_reached();
        break;
    }
    if (metadata)
        *metadata = themetadata;
    else
        g_object_unref(themetadata);
    return ret;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
