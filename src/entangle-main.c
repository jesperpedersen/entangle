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

#include <stdio.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <girepository.h>
#include <glib/gi18n.h>

#include "entangle-debug.h"
#include "entangle-application.h"
#include "entangle-camera-manager.h"


int main(int argc, char **argv)
{
    GOptionGroup *group;
    GOptionContext *optContext;
    GError *error = NULL;
    EntangleApplication *app;
    gboolean debug_app = FALSE;
    gboolean debug_gphoto = FALSE;
    gchar *ins = NULL;
    const GOptionEntry entries[] = {
        { "debug-entangle", 'd', 0, G_OPTION_ARG_NONE, &debug_app, "Enable debugging of application code", NULL },
        { "debug-gphoto", 'g', 0, G_OPTION_ARG_NONE, &debug_gphoto, "Enable debugging of gphoto library", NULL },
        { "introspect-dump", 'i', 0, G_OPTION_ARG_STRING, &ins, "Dump introspection data", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL },
    };
    static const char *help_msg = "Run 'entangle --help' to see full list of options";
    GtkIconTheme *theme;

    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    g_set_application_name("Entangle");

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

    entangle_debug_setup(debug_app, debug_gphoto);

    theme = gtk_icon_theme_get_default();
    gtk_icon_theme_prepend_search_path(theme, PKGDATADIR "/icons");

    app = entangle_application_new();

    g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
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
