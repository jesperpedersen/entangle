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

#include <config.h>

#include <stdio.h>

#include <gtk/gtk.h>
#if WITH_GOBJECT_INTROSPECTION
#include <girepository.h>
#endif

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn-launchee.h>
#include <gdk/gdkx.h>

#include "internal.h"
#include "app-display.h"

gboolean capa_debug_app = FALSE;
gboolean capa_debug_gphoto = FALSE;
gint64 capa_debug_startms = 0;
#if WITH_GOBJECT_INTROSPECTION
static gchar *ins = NULL;
#endif

static SnLauncheeContext *sn_context = NULL;
static SnDisplay *sn_display = NULL;

static void
sn_error_trap_push(SnDisplay *display G_GNUC_UNUSED,
                   Display *xdisplay G_GNUC_UNUSED)
{
    gdk_error_trap_push();
}

static void
sn_error_trap_pop(SnDisplay *display G_GNUC_UNUSED,
                  Display *xdisplay G_GNUC_UNUSED)
{
    gdk_error_trap_pop();
}

static void
startup_notification_complete(void)
{
    Display *xdisplay;

    xdisplay = GDK_DISPLAY();
    sn_display = sn_display_new(xdisplay,
                                sn_error_trap_push,
                                sn_error_trap_pop);
    sn_context =
        sn_launchee_context_new_from_environment(sn_display,
                                                 DefaultScreen(xdisplay));
    if (sn_context != NULL) {
        sn_launchee_context_complete(sn_context);
        sn_launchee_context_unref(sn_context);

        sn_display_unref(sn_display);
    }
}


int main(int argc, char **argv)
{
    CapaAppDisplay *display;
    GOptionGroup *group;
    GOptionContext *context;
    GError *error = NULL;
    static const GOptionEntry entries[] = {
        { "debug-capa", 'd', 0, G_OPTION_ARG_NONE, &capa_debug_app, "Enable debugging of application code", NULL },
        { "debug-gphoto", 'g', 0, G_OPTION_ARG_NONE, &capa_debug_gphoto, "Enable debugging of gphoto library", NULL },
#if WITH_GOBJECT_INTROSPECTION
        { "introspect-dump", 'i', 0, G_OPTION_ARG_STRING, &ins, "Dump introspection data", NULL },
#endif
        { NULL, 0, 0, 0, NULL, NULL, NULL },
    };
    static const char *help_msg = "Run 'capa --help' to see full list of options";
    gboolean unique;

    g_thread_init(NULL);
    gdk_threads_init();

    group = g_option_group_new("capa",
                               "Capa application options",
                               "Show Capa options",
                               NULL, NULL);

    g_option_group_add_entries(group, entries);

    /* Setup command line options */
    context = g_option_context_new("");
    g_option_context_add_group(context, gtk_get_option_group(FALSE));
    g_option_context_add_group(context, group);
    g_option_context_parse(context, &argc, &argv, &error);
    if (error) {
        g_print ("%s\n%s\n", error->message, help_msg);
        g_error_free (error);
        return 1;
    }

#if WITH_GOBJECT_INTROSPECTION
    if (ins) {
        g_irepository_dump(ins, NULL);
        return 0;
    }
#endif

    if (!gtk_init_check(NULL, NULL))
        return 1;

    display = capa_app_display_new();

    g_signal_connect(display, "app-closed", gtk_main_quit, NULL);

    unique = capa_app_display_show(display);

    startup_notification_complete();

    if (unique) {
        gdk_threads_enter();
        gtk_main();
        gdk_threads_leave();
    }

    g_object_unref(display);
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
