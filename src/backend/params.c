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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"
#include "params.h"

static void do_capa_log(GPLogLevel level G_GNUC_UNUSED,
			const char *domain,
			const char *format,
			va_list args,
			void *data G_GNUC_UNUSED)
{
  char *msg = g_strdup_vprintf(format, args);
  g_debug("%s: %s", domain, msg);
}

CapaParams *capa_params_new(void)
{
  CapaParams *params = g_new0(CapaParams, 1);
  static gboolean log_added = FALSE;

  if (capa_debug_gphoto && !log_added) {
    log_added = TRUE;
    gp_log_add_func(GP_LOG_DEBUG, do_capa_log, NULL);
  }

  params->ctx = gp_context_new();

  if (gp_abilities_list_new(&params->caps) != GP_OK)
    return NULL;

  if (gp_abilities_list_load(params->caps, params->ctx) != GP_OK)
    goto error;

  if (gp_port_info_list_new(&params->ports) != GP_OK)
    goto error;

  if (gp_port_info_list_load(params->ports) != GP_OK)
    goto error;

  CAPA_DEBUG("New params %p", params);

  return params;

 error:
  capa_params_free(params);
  return NULL;
}

void capa_params_refresh(CapaParams *params)
{
  if (params->ports)
    gp_port_info_list_free(params->ports);
  if (gp_port_info_list_new(&params->ports) != GP_OK)
    return;
  if (gp_port_info_list_load(params->ports) != GP_OK)
    return;
}

void capa_params_free(CapaParams *params)
{
  if (!params)
    return;

  CAPA_DEBUG("Free params %p", params);

  if (params->ports)
    gp_port_info_list_free(params->ports);
  if (params->caps)
    gp_abilities_list_free(params->caps);
  gp_context_unref(params->ctx);
  g_free(params);
}

