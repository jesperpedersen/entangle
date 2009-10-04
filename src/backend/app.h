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

#ifndef __CAPA_APP_H__
#define __CAPA_APP_H__

#include "camera-list.h"

#include "preferences.h"

typedef struct _CapaApp CapaApp;

CapaApp *capa_app_new(void);

void capa_app_free(CapaApp *app);

CapaCameraList *capa_app_cameras(CapaApp *app);

void capa_app_refresh(CapaApp *app);

CapaPreferences *capa_app_preferences(CapaApp *app);

#endif /* __CAPA_APP_H__ */
