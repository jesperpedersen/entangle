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

#include <gtk/gtk.h>
#include <girepository.h>

#include "entangle-debug.h"
#include "entangle-context.h"
#include "entangle-camera-manager.h"

static gchar *ins = NULL;

static void entangle_start(GApplication *app, gpointer opaque)
{
    EntangleCameraManager *manager = opaque;

    entangle_camera_manager_show(manager);
    g_application_hold(app);
}


int main(int argc, char **argv)
{
    GOptionGroup *group;
    GOptionContext *optContext;
    GError *error = NULL;
    GtkApplication *app;
    static const GOptionEntry entries[] = {
        { "debug-entangle", 'd', 0, G_OPTION_ARG_NONE, &entangle_debug_app, "Enable debugging of application code", NULL },
        { "debug-gphoto", 'g', 0, G_OPTION_ARG_NONE, &entangle_debug_gphoto, "Enable debugging of gphoto library", NULL },
        { "introspect-dump", 'i', 0, G_OPTION_ARG_STRING, &ins, "Dump introspection data", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL },
    };
    static const char *help_msg = "Run 'entangle --help' to see full list of options";
    EntangleCameraManager *manager;
    EntangleContext *context;

    g_thread_init(NULL);
    gdk_threads_init();

    group = g_option_group_new("entangle",
                               "Entangle application options",
                               "Show Entangle options",
                               NULL, NULL);

    g_option_group_add_entries(group, entries);

    /* Setup command line options */
    optContext = g_option_context_new("");
    g_option_context_add_group(optContext, gtk_get_option_group(FALSE));
    g_option_context_add_group(optContext, group);
    g_option_context_parse(optContext, &argc, &argv, &error);
    if (error) {
        g_print ("%s\n%s\n", error->message, help_msg);
        g_error_free (error);
        return 1;
    }

    if (ins) {
        g_irepository_dump(ins, NULL);
        return 0;
    }

    if (!gtk_init_check(NULL, NULL))
        return 1;

    app = gtk_application_new("org.entangle_photo.Manager", 0);

    gdk_threads_enter();
    context = entangle_context_new(G_APPLICATION(app));
    manager = entangle_camera_manager_new(context);

    g_signal_connect(app, "activate", G_CALLBACK(entangle_start), manager);

    g_application_run(G_APPLICATION(app), argc, argv);

    gdk_threads_leave();

    g_object_unref(context);
    g_object_unref(manager);
    return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
