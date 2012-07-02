/*
 *  Entangle: Tethered Camera Control & Capture
 *
 *  Copyright (C) 2012 Daniel P. Berrange
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

#include "entangle-dpms.h"
#include "entangle-debug.h"

#include <gtk/gtk.h>

#define ENTANGLE_ERROR(err, msg...)                                     \
    g_set_error((err),                                                  \
                g_quark_from_string("entangle-dpms"),                   \
                0,                                                      \
                msg)

#if defined(HAVE_XEXT) && defined(GDK_WINDOWING_X11)
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>

#include <gdk/gdkx.h>
#endif

gboolean entangle_dpms_set_blanking(gboolean enabled,
				    GError **error)
{
    GdkDisplay *dpy = gdk_display_get_default();

    ENTANGLE_DEBUG("Toggle set blanking %d", enabled);

#if defined(HAVE_XEXT) && defined(GDK_WINDOWING_X11)
    if (GDK_IS_X11_DISPLAY(dpy)) {
        Display *xdpy = gdk_x11_display_get_xdisplay(dpy);
        int ignore1, ignore2;

        if (!DPMSQueryExtension(xdpy, &ignore1, &ignore2) ||
            !DPMSCapable(xdpy)) {
            ENTANGLE_ERROR(error, "%s", "Screen blanking is not available on this display");
            return FALSE;
        }

        DPMSEnable(xdpy);
        DPMSForceLevel(xdpy, enabled ? DPMSModeStandby : DPMSModeOn);

        return TRUE;
    }
#endif

    ENTANGLE_ERROR(error, "%s", "Screen blanking is not implemented on this platform");
    return FALSE;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
