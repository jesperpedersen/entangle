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

#include <stdio.h>

#include "entangle-debug.h"
#include "entangle-control-button.h"

#define ENTANGLE_CONTROL_BUTTON_GET_PRIVATE(obj)                            \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONTROL_BUTTON, EntangleControlButtonPrivate))

struct _EntangleControlButtonPrivate {
    gboolean dummy;
};

G_DEFINE_TYPE(EntangleControlButton, entangle_control_button, ENTANGLE_TYPE_CONTROL);


static void entangle_control_button_finalize (GObject *object)
{

    G_OBJECT_CLASS (entangle_control_button_parent_class)->finalize (object);
}

static void entangle_control_button_class_init(EntangleControlButtonClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_control_button_finalize;

    g_type_class_add_private(klass, sizeof(EntangleControlButtonPrivate));
}


EntangleControlButton *entangle_control_button_new(const char *path,
                                           int id,
                                           const char *label,
                                           const char *info)
{
    return ENTANGLE_CONTROL_BUTTON(g_object_new(ENTANGLE_TYPE_CONTROL_BUTTON,
                                            "path", path,
                                            "id", id,
                                            "label", label,
                                            "info", info,
                                            NULL));
}


static void entangle_control_button_init(EntangleControlButton *picker)
{
    EntangleControlButtonPrivate *priv;

    priv = picker->priv = ENTANGLE_CONTROL_BUTTON_GET_PRIVATE(picker);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */