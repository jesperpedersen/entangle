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

#include <glib/gi18n.h>

#include "entangle-debug.h"
#include "entangle-window.h"

/**
 * entangle_window_new:
 * @newwintype: the desired window type
 * @oldwintype: the UI file window type
 * @winname: the UI file window name
 *
 * Construct a new window from the UI file.
 * Loads a UI file and finds the top level window with
 * type of @oldwintype and changes it to have type of
 * @newwintype. Then it constructs the window
 *
 * Returns: (transfer full): the new window
 */
EntangleWindow *entangle_window_new(GType newwintype,
                                    GType oldwintype,
                                    const gchar *winname)
{
    GtkBuilder *builder;
    gchar *filename;
    EntangleWindow *win;
    GError *error = NULL;
    gchar *buffer;
    gsize length;

    builder = gtk_builder_new();

    if (access("./entangle", R_OK) == 0)
        filename = g_strdup_printf("%s/entangle-%s.ui", "frontend", winname);
    else
        filename = g_strdup_printf("%s/entangle-%s.ui", PKGDATADIR, winname);

    if (!g_file_get_contents(filename, &buffer, &length, &error))
        g_error(_("Could not load user interface definition file: %s"), error->message);

    gchar *offset = strstr(buffer, g_type_name(oldwintype));
    gchar *tmp;
    *offset = '\0';

    tmp = g_strdup_printf("%s%s%s",
                          buffer,
                          g_type_name(newwintype),
                          offset + strlen(g_type_name(oldwintype)));
    g_free(buffer);
    buffer = tmp;
    length = strlen(buffer);
    //g_print("%s", buffer);

    gtk_builder_add_from_string(builder, buffer, length, &error);
    g_free(filename);
    if (error)
        g_error(_("Could not load user interface definition file: %s"), error->message);

    win = ENTANGLE_WINDOW(gtk_builder_get_object(builder, winname));

    gtk_builder_connect_signals(builder, win);
    g_free(buffer);

    entangle_window_set_builder(win, builder);

    return win;
}

GType
entangle_window_get_type(void)
{
    static GType window_type = 0;

    if (!window_type) {
        window_type =
            g_type_register_static_simple(G_TYPE_INTERFACE, "EntangleWindow",
                                          sizeof(EntangleWindowInterface),
                                          NULL, 0, NULL, 0);

        g_type_interface_add_prerequisite(window_type, G_TYPE_OBJECT);
    }

    return window_type;
}


void entangle_window_set_builder(EntangleWindow *win,
                                 GtkBuilder *builder)
{
    g_return_if_fail(ENTANGLE_IS_WINDOW(win));

    EntangleWindowInterface *winiface = ENTANGLE_WINDOW_GET_INTERFACE(win);
    winiface->set_builder(win, builder);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
