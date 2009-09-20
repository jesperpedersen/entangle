
#include <stdio.h>

#include <gtk/gtk.h>
#include "app-display.h"

int main(int argc, char **argv)
{
  CapaApp *app;
  CapaAppDisplay *display;

  app = capa_app_new();

  CapaCameraList *cams = capa_app_detect_cameras(app);

  for (int i = 0 ; i < capa_camera_list_count(cams) ; i++) {
    CapaCamera *cam = capa_camera_list_get(cams, i);
    fprintf(stderr, "%s %s\n",
	    capa_camera_model(cam),
	    capa_camera_port(cam));
  }


  gtk_init(&argc, &argv);

  display = capa_app_display_new();

  g_signal_connect(display, "app-closed", gtk_main_quit, NULL);

  capa_app_display_show(display);

  gtk_main();

  g_object_unref(display);
  capa_app_free(app);
  return 0;
}
