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

#ifndef __CAPA_CAMERA_FILE_H__
#define __CAPA_CAMERA_FILE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define CAPA_TYPE_CAMERA_FILE            (capa_camera_file_get_type ())
#define CAPA_CAMERA_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_CAMERA_FILE, CapaCameraFile))
#define CAPA_CAMERA_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_CAMERA_FILE, CapaCameraFileClass))
#define CAPA_IS_CAMERA_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_CAMERA_FILE))
#define CAPA_IS_CAMERA_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_CAMERA_FILE))
#define CAPA_CAMERA_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_CAMERA_FILE, CapaCameraFileClass))


typedef struct _CapaCameraFile CapaCameraFile;
typedef struct _CapaCameraFilePrivate CapaCameraFilePrivate;
typedef struct _CapaCameraFileClass CapaCameraFileClass;

struct _CapaCameraFile
{
    GObject parent;

    CapaCameraFilePrivate *priv;
};

struct _CapaCameraFileClass
{
    GObjectClass parent_class;
};

GType capa_camera_file_get_type(void) G_GNUC_CONST;

CapaCameraFile *capa_camera_file_new(const char *folder,
				     const char *name);

const char *capa_camera_file_get_folder(CapaCameraFile *file);
const char *capa_camera_file_get_name(CapaCameraFile *file);

gboolean capa_camera_file_save_path(CapaCameraFile *file,
				    const char *localpath,
				    GError **err);
gboolean capa_camera_file_save_uri(CapaCameraFile *file,
				   const char *uri,
				   GError **err);

GByteArray *capa_camera_file_get_data(CapaCameraFile *file);
void capa_camera_file_set_data(CapaCameraFile *file, GByteArray *data);

const gchar *capa_camera_file_get_mimetype(CapaCameraFile *file);
void capa_camera_file_set_mimetype(CapaCameraFile *file, const gchar *mimetype);

G_END_DECLS

#endif /* __CAPA_CAMERA_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
