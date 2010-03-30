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

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
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
            priv->nextFilenameDigit = 0;
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

char *entangle_session_next_filename(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;
    const char *template = strchr(priv->filenamePattern, 'X');
    const char *postfix;
    char *prefix;
    char *format;
    char *filename;
    int i;
    int max;
    int ndigits;

    ENTANGLE_DEBUG("NEXT FILENAME '%s'", template);

    if (!template)
        return NULL;

    postfix = template;
    while (*postfix == 'X')
        postfix++;

    prefix = g_strndup(priv->filenamePattern,
                       template - priv->filenamePattern);

    ndigits = postfix - template;

    for (max = 1, i = 0 ; i < ndigits ; i++)
        max *= 10;

    format = g_strdup_printf("%%s/%%s%%0%dd%%s", ndigits);

    ENTANGLE_DEBUG("TEST %d (%d) possible with '%s' prefix='%s' postfix='%s'", max, ndigits, format, prefix, postfix);

    for (i = priv->nextFilenameDigit ; i < max ; i++) {
        filename = g_strdup_printf(format, priv->directory,
                                   prefix, i, postfix);

        ENTANGLE_DEBUG("Test filename '%s'", filename);
        if (access(filename, R_OK) < 0) {
            if (errno != ENOENT) {
                g_free(filename);
                filename = NULL;
            }
            /* Cache digit offset to avoid stat() sooo many files next time */
            priv->nextFilenameDigit = i + 1;
            break;
        }
        g_free(filename);
        filename = NULL;
    }

    g_free(prefix);
    g_free(format);
    return filename;
}


char *entangle_session_temp_filename(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;
    char *pattern = g_strdup_printf("%s/%s", priv->directory, "previewXXXXXX");
    int fd = mkstemp(pattern);

    if (fd < 0) {
        g_free(pattern);
        return NULL;
    }

    close(fd);
    return pattern;
}


void entangle_session_add(EntangleSession *session, EntangleImage *image)
{
    EntangleSessionPrivate *priv = session->priv;

    g_object_ref(image);
    priv->images = g_list_prepend(priv->images, image);

    g_signal_emit_by_name(session, "session-image-added", image);
}

gboolean entangle_session_load(EntangleSession *session)
{
    EntangleSessionPrivate *priv = session->priv;
    DIR *dh;
    struct dirent *ent;

    dh = opendir(priv->directory);
    if (!dh)
        return FALSE;

    while ((ent = readdir(dh)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;

        char *filename = g_strdup_printf("%s/%s", priv->directory, ent->d_name);
        EntangleImage *image = entangle_image_new(filename);

        ENTANGLE_DEBUG("Adding '%s'", filename);
        entangle_session_add(session, image);

        g_object_unref(image);
    }
    closedir(dh);

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
