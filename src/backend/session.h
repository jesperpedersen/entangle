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

#ifndef __CAPA_SESSION__
#define __CAPA_SESSION__

#include <glib-object.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "control-group.h"
#include "image.h"

G_BEGIN_DECLS

#define CAPA_TYPE_SESSION            (capa_session_get_type ())
#define CAPA_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAPA_TYPE_SESSION, CapaSession))
#define CAPA_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAPA_TYPE_SESSION, CapaSessionClass))
#define CAPA_IS_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAPA_TYPE_SESSION))
#define CAPA_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAPA_TYPE_SESSION))
#define CAPA_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAPA_TYPE_SESSION, CapaSessionClass))


typedef struct _CapaSession CapaSession;
typedef struct _CapaSessionPrivate CapaSessionPrivate;
typedef struct _CapaSessionClass CapaSessionClass;

struct _CapaSession
{
  GObject parent;

  CapaSessionPrivate *priv;
};

struct _CapaSessionClass
{
  GObjectClass parent_class;
};


GType capa_session_get_type(void) G_GNUC_CONST;

CapaSession *capa_session_new(const char *directory,
			      const char *filenamePattern);

const char *capa_session_directory(CapaSession *session);
const char *capa_session_filename_pattern(CapaSession *session);

char *capa_session_next_filename(CapaSession *session);
char *capa_session_temp_filename(CapaSession *session);

gboolean capa_session_load(CapaSession *session);

void capa_session_add(CapaSession *session, CapaImage *image);

G_END_DECLS

#endif /* __CAPA_SESSION__ */
