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

#include <stdio.h>

#include <gtk/gtk.h>
#include "app-display.h"

int main(int argc, char **argv)
{
  CapaApp *app;
  CapaAppDisplay *display;

  gtk_init(&argc, &argv);

  app = capa_app_new();

  CapaCameraList *cams = capa_app_detect_cameras(app);

  for (int i = 0 ; i < capa_camera_list_count(cams) ; i++) {
    CapaCamera *cam = capa_camera_list_get(cams, i);
    fprintf(stderr, "%s %s\n",
	    capa_camera_model(cam),
	    capa_camera_port(cam));
  }

  display = capa_app_display_new();

  g_signal_connect(display, "app-closed", gtk_main_quit, NULL);

  capa_app_display_show(display);

  gtk_main();

  g_object_unref(display);
  capa_app_free(app);
  return 0;
}
