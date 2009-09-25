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

#ifndef __CAPA_PARAMS__
#define __CAPA_PARAMS__

#include <gphoto2.h>

typedef struct _CapaParams CapaParams;

struct _CapaParams {
  GPContext *ctx;
  CameraAbilitiesList *caps;
  GPPortInfoList *ports;
};


CapaParams *capa_params_new(void);

void capa_params_free(CapaParams *params);

void capa_params_refresh(CapaParams *params);


#endif /* __CAPA_PARAMS__ */
