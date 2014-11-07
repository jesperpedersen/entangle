/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2015 Daniel P. Berrange
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

#ifndef __ENTANGLE_SESSION_H__
#define __ENTANGLE_SESSION_H__

#include <glib-object.h>

#include "entangle-image.h"
#include "entangle-camera-file.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_SESSION            (entangle_session_get_type ())
#define ENTANGLE_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_SESSION, EntangleSession))
#define ENTANGLE_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_SESSION, EntangleSessionClass))
#define ENTANGLE_IS_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_SESSION))
#define ENTANGLE_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_SESSION))
#define ENTANGLE_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_SESSION, EntangleSessionClass))


typedef struct _EntangleSession EntangleSession;
typedef struct _EntangleSessionPrivate EntangleSessionPrivate;
typedef struct _EntangleSessionClass EntangleSessionClass;

struct _EntangleSession
{
    GObject parent;

    EntangleSessionPrivate *priv;
};

struct _EntangleSessionClass
{
    GObjectClass parent_class;

    void (*session_image_added)(EntangleSession *session, EntangleImage *image);
    void (*session_image_removed)(EntangleSession *session, EntangleImage *image);
};


GType entangle_session_get_type(void) G_GNUC_CONST;

EntangleSession *entangle_session_new(const char *directory,
                                      const char *filenamePattern);

const char *entangle_session_directory(EntangleSession *session);
const char *entangle_session_filename_pattern(EntangleSession *session);

char *entangle_session_next_filename(EntangleSession *session,
                                     EntangleCameraFile *file);

gboolean entangle_session_load(EntangleSession *session);

void entangle_session_add(EntangleSession *session, EntangleImage *image);
void entangle_session_remove(EntangleSession *session, EntangleImage *image);

int entangle_session_image_count(EntangleSession *session);

EntangleImage *entangle_session_image_get(EntangleSession *session, int idx);

G_END_DECLS

#endif /* __ENTANGLE_SESSION_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
