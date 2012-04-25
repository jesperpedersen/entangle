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
#include <stdio.h>
#include <string.h>

#include "entangle-debug.h"
#include "entangle-camera-list.h"

#define ENTANGLE_CAMERA_LIST_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CAMERA_LIST, EntangleCameraListPrivate))

struct _EntangleCameraListPrivate {
    size_t ncamera;
    EntangleCamera **cameras;
};

G_DEFINE_TYPE(EntangleCameraList, entangle_camera_list, G_TYPE_OBJECT);


static void entangle_camera_list_finalize (GObject *object)
{
    EntangleCameraList *list = ENTANGLE_CAMERA_LIST(object);
    EntangleCameraListPrivate *priv = list->priv;
    ENTANGLE_DEBUG("Finalize list");

    for (int i = 0 ; i < priv->ncamera ; i++) {
        ENTANGLE_DEBUG("Unref camera in list %p", priv->cameras[i]);
        g_object_unref(priv->cameras[i]);
    }
    g_free(priv->cameras);

    G_OBJECT_CLASS (entangle_camera_list_parent_class)->finalize (object);
}


static void entangle_camera_list_class_init(EntangleCameraListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_camera_list_finalize;

    g_signal_new("camera-added",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraListClass, camera_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA);

    g_signal_new("camera-removed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(EntangleCameraListClass, camera_removed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1,
                 ENTANGLE_TYPE_CAMERA);


    g_type_class_add_private(klass, sizeof(EntangleCameraListPrivate));
}


EntangleCameraList *entangle_camera_list_new(void)
{
    return ENTANGLE_CAMERA_LIST(g_object_new(ENTANGLE_TYPE_CAMERA_LIST, NULL));
}


static void entangle_camera_list_init(EntangleCameraList *list)
{
    list->priv = ENTANGLE_CAMERA_LIST_GET_PRIVATE(list);
}


int entangle_camera_list_count(EntangleCameraList *list)
{
    EntangleCameraListPrivate *priv = list->priv;

    return priv->ncamera;
}

void entangle_camera_list_add(EntangleCameraList *list,
                          EntangleCamera *cam)
{
    EntangleCameraListPrivate *priv = list->priv;

    priv->cameras = g_renew(EntangleCamera *, priv->cameras, priv->ncamera+1);
    priv->cameras[priv->ncamera++] = cam;
    g_object_ref(cam);

    g_signal_emit_by_name(list, "camera-added", cam);
    ENTANGLE_DEBUG("Added camera %p", cam);
}

void entangle_camera_list_remove(EntangleCameraList *list,
                             EntangleCamera *cam)
{
    EntangleCameraListPrivate *priv = list->priv;

    for (int i = 0 ; i < priv->ncamera ; i++) {
        if (priv->cameras[i] == cam) {
            if (i < (priv->ncamera-1))
                memmove(priv->cameras + i,
                        priv->cameras + i + 1,
                        sizeof(*priv->cameras) * (priv->ncamera - i - 1));
            priv->ncamera--;
        }
    }

    ENTANGLE_DEBUG("Removed camera %p from list", cam);
    g_signal_emit_by_name(list, "camera-removed", cam);

    g_object_unref(cam);
}

EntangleCamera *entangle_camera_list_get(EntangleCameraList *list,
                                 int entry)
{
    EntangleCameraListPrivate *priv = list->priv;

    if (entry < 0 || entry >= priv->ncamera)
        return NULL;

    return priv->cameras[entry];
}

EntangleCamera *entangle_camera_list_find(EntangleCameraList *list,
                                  const char *port)
{
    EntangleCameraListPrivate *priv = list->priv;
    int i;

    for (i = 0 ; i < priv->ncamera ; i++) {
        const char *thisport = entangle_camera_get_port(priv->cameras[i]);

        ENTANGLE_DEBUG("Compare '%s' '%s'", port, thisport);

        if (strcmp(thisport, port) == 0)
            return priv->cameras[i];
    }

    return NULL;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
