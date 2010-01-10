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


#ifndef __CAPA_DEBUG_H__
#define __CAPA_DEBUG_H__

#include <glib.h>
#include <sys/time.h>

extern gboolean capa_debug_app;
extern gboolean capa_debug_gphoto;
extern gint64 capa_debug_startms;

#define CAPA_DEBUG(fmt, ...)						\
  do {									\
    if (G_UNLIKELY(capa_debug_app)) {					\
      struct timeval now;						\
      gint64 nowms;							\
      gettimeofday(&now, NULL);						\
      nowms = (now.tv_sec * 1000ll) + (now.tv_usec / 1000ll);		\
      if (capa_debug_startms == 0)					\
	capa_debug_startms = nowms;					\
      nowms -= capa_debug_startms;					\
      g_debug("[%08llu %s:%s:%d] " fmt, nowms, __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
    }									\
  } while (0)

#endif /* __CAPA_DEBUG_H__ */
