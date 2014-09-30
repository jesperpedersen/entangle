/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2009-2014 Daniel P. Berrange
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

#ifndef __ENTANGLE_CAMERA_AUTOMATA_H__
#define __ENTANGLE_CAMERA_AUTOMATA_H__

#include <glib-object.h>

#include "entangle-camera.h"
#include "entangle-session.h"
#include "entangle-image.h"

G_BEGIN_DECLS

#define ENTANGLE_TYPE_CAMERA_AUTOMATA            (entangle_camera_automata_get_type ())
#define ENTANGLE_CAMERA_AUTOMATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ENTANGLE_TYPE_CAMERA_AUTOMATA, EntangleCameraAutomata))
#define ENTANGLE_CAMERA_AUTOMATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ENTANGLE_TYPE_CAMERA_AUTOMATA, EntangleCameraAutomataClass))
#define ENTANGLE_IS_CAMERA_AUTOMATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ENTANGLE_TYPE_CAMERA_AUTOMATA))
#define ENTANGLE_IS_CAMERA_AUTOMATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ENTANGLE_TYPE_CAMERA_AUTOMATA))
#define ENTANGLE_CAMERA_AUTOMATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ENTANGLE_TYPE_CAMERA_AUTOMATA, EntangleCameraAutomataClass))


typedef struct _EntangleCameraAutomata EntangleCameraAutomata;
typedef struct _EntangleCameraAutomataPrivate EntangleCameraAutomataPrivate;
typedef struct _EntangleCameraAutomataClass EntangleCameraAutomataClass;

struct _EntangleCameraAutomata
{
    GObject parent;

    EntangleCameraAutomataPrivate *priv;
};

struct _EntangleCameraAutomataClass
{
    GObjectClass parent_class;

    void (*camera_capture_begin)(EntangleCameraAutomata *automata);
    void (*camera_capture_end)(EntangleCameraAutomata *automata);
};

GType entangle_camera_automata_get_type(void) G_GNUC_CONST;

EntangleCameraAutomata *entangle_camera_automata_new(void);

void entangle_camera_automata_capture_async(EntangleCameraAutomata *automata,
                                            GCancellable *cancel,
                                            GAsyncReadyCallback callback,
                                            gpointer user_data);

gboolean entangle_camera_automata_capture_finish(EntangleCameraAutomata *automata,
                                                 GAsyncResult *res,
                                                 GError **error);

void entangle_camera_automata_preview_async(EntangleCameraAutomata *automata,
                                            GCancellable *cancel,
                                            GCancellable *confirm,
                                            GAsyncReadyCallback callback,
                                            gpointer user_data);

gboolean entangle_camera_automata_preview_finish(EntangleCameraAutomata *automata,
                                                 GAsyncResult *res,
                                                 GError **error);

void entangle_camera_automata_set_camera(EntangleCameraAutomata *automata,
                                         EntangleCamera *camera);
EntangleCamera *entangle_camera_automata_get_camera(EntangleCameraAutomata *automata);

void entangle_camera_automata_set_session(EntangleCameraAutomata *automata,
                                          EntangleSession *session);
EntangleSession *entangle_camera_automata_get_session(EntangleCameraAutomata *automata);

void entangle_camera_automata_set_delete_file(EntangleCameraAutomata *automata,
                                                gboolean value);
gboolean entangle_camera_automata_get_delete_file(EntangleCameraAutomata *automata);

G_END_DECLS

#endif /* __ENTANGLE_CAMERA_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
