/*
 *  Entangle: Entangle Assists Photograph Aquisition
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


#ifndef __ENTANGLE_DEBUG_H__
#define __ENTANGLE_DEBUG_H__

#include <glib.h>
#include <sys/time.h>

extern gboolean entangle_debug_app;
extern gboolean entangle_debug_gphoto;
extern gint64 entangle_debug_startms;

#define ENTANGLE_DEBUG(fmt, ...)                                        \
    do {                                                                \
        if (G_UNLIKELY(entangle_debug_app)) {                           \
            struct timeval now;                                         \
            gint64 nowms;                                               \
            gettimeofday(&now, NULL);                                   \
            nowms = (now.tv_sec * 1000ll) + (now.tv_usec / 1000ll);     \
            if (entangle_debug_startms == 0)                            \
                entangle_debug_startms = nowms;                         \
            nowms -= entangle_debug_startms;                            \
            g_debug("[%08" G_GINT64_FORMAT " %s:%s:%d] " fmt, nowms, __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
        }                                                               \
    } while (0)

#endif /* __ENTANGLE_DEBUG_H__ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
