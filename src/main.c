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
#include "app-display.h"

gboolean capa_debug_app = FALSE;
gboolean capa_debug_gphoto = FALSE;

int main(int argc, char **argv)
{
  CapaAppDisplay *display;
  GOptionGroup *group;
  GOptionContext *context;
  GError *error = NULL;
  static const GOptionEntry entries[] = {
    { "debug-capa", 'd', 0, G_OPTION_ARG_NONE, &capa_debug_app, "Enable debugging of application code", NULL },
    { "debug-gphoto", 'g', 0, G_OPTION_ARG_NONE, &capa_debug_gphoto, "Enable debugging of gphoto library", NULL },
    { NULL, 0, 0, 0, NULL, NULL, NULL },
  };
  static const char *help_msg = "Run 'capa --help' to see full list of options";

  g_thread_init(NULL);

  group = g_option_group_new("capa",
			     "Capa application options",
			     "Show Capa options",
			     NULL, NULL);

  g_option_group_add_entries(group, entries);

  /* Setup command line options */
  context = g_option_context_new("");
  g_option_context_add_group(context, gtk_get_option_group (TRUE));
  g_option_context_add_group(context, group);
  g_option_context_parse(context, &argc, &argv, &error);
  if (error) {
    g_print ("%s\n%s\n", error->message, help_msg);
    g_error_free (error);
    return 1;
  }

  gdk_threads_init();
  gtk_init(&argc, &argv);

  display = capa_app_display_new();

  g_signal_connect(display, "app-closed", gtk_main_quit, NULL);

  capa_app_display_show(display);

  gtk_main();

  g_object_unref(display);
  return 0;
}
