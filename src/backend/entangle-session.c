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

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "entangle-debug.h"
#include "entangle-session.h"
#include "entangle-image.h"

#define ENTANGLE_SESSION_GET_PRIVATE(obj)                                   \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_SESSION, EntangleSessionPrivate))

struct _EntangleSessionPrivate {
    char *directory;
    char *filenamePattern;
    gboolean recalculateDigit;
    int nextFilenameDigit;

    GList *images;
};

G_DEFINE_TYPE(EntangleSession, entangle_session, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_DIRECTORY,
    PROP_FILENAME_PATTERN,
};

static void entangle_session_get_property(GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
    EntangleSession *picker = ENTANGLE_SESSION(object);
    EntangleSessionPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_DIRECTORY:
            g_value_set_string(value, priv->directory);
            break;

        case PROP_FILENAME_PATTERN:
            g_value_set_string(value, priv->filenamePattern);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}

static void entangle_session_set_property(GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
    EntangleSession *picker = ENTANGLE_SESSION(object);
    EntangleSessionPrivate *priv = picker->priv;

    switch (prop_id)
        {
        case PROP_DIRECTORY:
            g_free(priv->directory);
            priv->directory = g_value_dup_string(value);
            g_mkdir_with_parents(priv->directory, 0777);
            break;

        case PROP_FILENAME_PATTERN:
            g_free(priv->filenamePattern);
            priv->filenamePattern = g_value_dup_string(value);
            priv->recalculateDigit = TRUE;
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        }
}


static void do_image_unref(gpointer object,
                           gpointer opaque G_GNUC_UNUSED)
{
    EntangleImage *image = object;

    g_object_unref(image);
}

static void entangle_session_finalize(GObject *object)
{
    EntangleSession *session = ENTANGLE_SESSION(object);
    EntangleSessionPrivate *priv = session->priv;

    ENTANGLE_DEBUG("Finalize session %p", object);

    if (priv->images) {
        g_list_foreach(priv->images, do_image_unref, NULL);
        g_list_free(priv->images);
    }

    g_free(priv->filenamePattern);
    g_free(priv->directory);

    G_OBJECT_CLASS (entangle_session_parent_class)->finalize (object);
}


static void entangle_session_class_init(EntangleSessionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_session_finalize;
    object_class->get_property = entangle_session_get_property;
    object_class->set_property = entangle_session_set_property;

    g_signal_new("session-image-added",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleSessionClass, session_image_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_IMAGE);

    g_signal_new("session-image-removed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleSessionClass, session_image_removed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_IMAGE);

    g_object_class_install_property(object_class,
                                    PROP_DIRECTORY,
                                    g_param_spec_string("directory",
                                                        "Session directory",
                                                        "Full path to session file",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_FILENAME_PATTERN,
                                    g_param_spec_string("filename-pattern",
                                                        "Filename patern",
                                                        "Pattern for creating new filenames",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(EntangleSessionPrivate));
}


EntangleSession *entangle_session_new(const char *directory,
                              const char *filenamePattern)
{
    return ENTANGLE_SESSION(g_object_new(ENTANGLE_TYPE_SESSION,
                                     "directory", directory,
                                     "filename-pattern", filenamePattern,
                                     NULL));
}


static void entangle_session_init(EntangleSession *session)
{
    EntangleSessionPrivate *priv;

    priv = session->priv = ENTANGLE_SESSION_GET_PRIVATE(session);

    priv->recalculateDigit = TRUE;
}


const char *entangle_session_directory(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;

    return priv->directory;
}

const char *entangle_session_filename_pattern(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;

    return priv->filenamePattern;
}


static char *entangle_session_get_extension(EntangleCameraFile *file)
{
    const char *name = entangle_camera_file_get_name(file);
    const char *ext;

    ext = g_strrstr(name, ".");

    if (!ext)
        ext = ".jpeg";

    ext++;
    return g_ascii_strdown(ext, -1);
}


static gint entangle_session_next_digit(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;
    gint maxDigit = -1;
    GList *images = priv->images;
    const gchar *template = strchr(priv->filenamePattern, 'X');
    gchar *prefix = g_strndup(priv->filenamePattern, (template - priv->filenamePattern));
    gint prefixlen = strlen(prefix);
    gint templatelen = 0;
    const gchar *postfix;
    gint postfixlen;

    while (*template == 'X') {
        templatelen++;
        template++;
    }

    postfix = template;
    postfixlen = strlen(postfix);

    ENTANGLE_DEBUG("Template '%s' with prefixlen %d, %d digits and postfix %d",
                   priv->filenamePattern, prefixlen, templatelen, postfixlen);

    while (images) {
        EntangleImage *image = images->data;
        const gchar *name = entangle_image_get_filename(image);
        gsize remain = templatelen;
        gint digit = 0;

        if (!g_str_has_prefix(name, priv->directory)) {
            ENTANGLE_DEBUG("File %s does not match directory",
                           entangle_image_get_filename(image));
            goto next;
        }
        name += strlen(priv->directory);
        while (*name == '/')
            name++;

        /* Ignore files not matching the template prefix */
        if (!g_str_has_prefix(name, prefix)) {
            ENTANGLE_DEBUG("File %s does not match prefix",
                           entangle_image_get_filename(image));
            goto next;
        }

        name += prefixlen;

        /* Skip over filename matching digits */
        while (remain && g_ascii_isdigit(*name)) {
            digit *= 10;
            digit += *name - '0';
            name++;
            remain--;
        }

        /* See if unexpectedly got a non-digit before end of template */
        if (remain) {
            ENTANGLE_DEBUG("File %s has too few digits",
                           entangle_image_get_filename(image));
            goto next;
        }

        if (!g_str_has_prefix(name, postfix)) {
            ENTANGLE_DEBUG("File %s does not match postfix",
                           entangle_image_get_filename(image));
            goto next;
        }

        name += postfixlen;

        /* Verify there is a file extension following the digits */
        if (*name != '.') {
            ENTANGLE_DEBUG("File %s has trailing data",
                           entangle_image_get_filename(image));
            goto next;
        }

        if (digit > maxDigit)
            maxDigit = digit;
        ENTANGLE_DEBUG("File %s matches maxDigit is %d",
                       entangle_image_get_filename(image), maxDigit);

    next:
        images = images->next;
    }

    g_free(prefix);

    return maxDigit + 1;
}


char *entangle_session_next_filename(EntangleSession *session,
                                     EntangleCameraFile *file)
{
    EntangleSessionPrivate *priv = session->priv;
    const char *template = strchr(priv->filenamePattern, 'X');
    const char *postfix;
    char *ext = entangle_session_get_extension(file);
    char *prefix;
    char *format;
    char *filename;
    int i;
    int max;
    int ndigits;

    ENTANGLE_DEBUG("NEXT FILENAME '%s'", template);

    if (!template)
        return NULL;

    if (priv->recalculateDigit) {
        priv->nextFilenameDigit = entangle_session_next_digit(session);
        priv->recalculateDigit = FALSE;
    }

    postfix = template;
    while (*postfix == 'X')
        postfix++;

    prefix = g_strndup(priv->filenamePattern,
                       template - priv->filenamePattern);

    ndigits = postfix - template;

    for (max = 1, i = 0 ; i < ndigits ; i++)
        max *= 10;

    format = g_strdup_printf("%%s/%%s%%0%dd%%s.%%s", ndigits);

    ENTANGLE_DEBUG("Format '%s' prefix='%s' postfix='%s' ndigits=%d nextDigits=%d ext=%s",
                   format, prefix, postfix, ndigits, priv->nextFilenameDigit, ext);

    filename = g_strdup_printf(format, priv->directory,
                               prefix, priv->nextFilenameDigit,
                               postfix, ext);

    ENTANGLE_DEBUG("Built '%s'", filename);

    if (access(filename, R_OK) == 0 || errno != ENOENT) {
        ENTANGLE_DEBUG("Filename %s unexpectedly exists", filename);
        g_free(filename);
        filename = NULL;
    }

    priv->nextFilenameDigit++;

    g_free(prefix);
    g_free(format);
    g_free(ext);

    return filename;
}


void entangle_session_add(EntangleSession *session, EntangleImage *image)
{
    EntangleSessionPrivate *priv = session->priv;

    g_object_ref(image);
    priv->images = g_list_prepend(priv->images, image);

    g_signal_emit_by_name(session, "session-image-added", image);
}


void entangle_session_remove(EntangleSession *session, EntangleImage *image)
{
    EntangleSessionPrivate *priv = session->priv;
    GList *tmp = g_list_find(priv->images, image);

    if (!tmp)
        return;

    priv->images = g_list_delete_link(priv->images, tmp);

    g_signal_emit_by_name(session, "session-image-removed", image);
    g_object_unref(image);
}


gboolean entangle_session_load(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;
    GFile *dir = g_file_new_for_path(priv->directory);
    GFileEnumerator *children = g_file_enumerate_children(dir,
                                                          "standard::name,standard::type",
                                                          G_FILE_QUERY_INFO_NONE,
                                                          NULL,
                                                          NULL);
    GFileInfo *childinfo;
    while ((childinfo = g_file_enumerator_next_file(children, NULL, NULL)) != NULL) {
        const gchar *thisname = g_file_info_get_name(childinfo);
        GFile *child = g_file_get_child(dir, thisname);
        if (g_file_info_get_file_type(childinfo) == G_FILE_TYPE_REGULAR ||
            g_file_info_get_file_type(childinfo) == G_FILE_TYPE_SYMBOLIC_LINK) {

            EntangleImage *image = entangle_image_new_file(g_file_get_path(child));

            ENTANGLE_DEBUG("Adding '%s'", g_file_get_path(child));
            entangle_session_add(session, image);

            g_object_unref(image);
        }
        g_object_unref(child);
        g_object_unref(childinfo);
    }

    g_object_unref(children);

    priv->recalculateDigit = TRUE;

    return TRUE;
}

int entangle_session_image_count(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;

    return g_list_length(priv->images);
}

EntangleImage *entangle_session_image_get(EntangleSession *session, int idx)
{
    EntangleSessionPrivate *priv = session->priv;

    return g_list_nth_data(priv->images, idx);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
