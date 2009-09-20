
#include <glib.h>

#include "params.h"


CapaParams *capa_params_new(void)
{
  CapaParams *params = g_new0(CapaParams, 1);

  params->ctx = gp_context_new();

  if (gp_abilities_list_new(&params->caps) != GP_OK)
    return NULL;

  if (gp_abilities_list_load(params->caps, params->ctx) != GP_OK)
    goto error;

  if (gp_port_info_list_new(&params->ports) != GP_OK)
    goto error;

  if (gp_port_info_list_load(params->ports) != GP_OK)
    goto error;

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

  if (params->ports)
    gp_port_info_list_free(params->ports);
  if (params->caps)
    gp_abilities_list_free(params->caps);
  gp_context_unref(params->ctx);
  g_free(params);
}

