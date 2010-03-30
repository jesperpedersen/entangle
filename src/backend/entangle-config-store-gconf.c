/*
 *  Entangle: Entangle Assists Photograph Aquisition
 *
 *  Copyright (C) 2009-2010 Daniel P. Berrange
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

#include <string.h>
#include <gconf/gconf-client.h>

#include "entangle-debug.h"
#include "entangle-config-store-gconf.h"

#define ENTANGLE_CONFIG_STORE_GCONF_GET_PRIVATE(obj)                               \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), ENTANGLE_TYPE_CONFIG_STORE_GCONF, EntangleConfigStoreGConfPrivate))

struct _EntangleConfigStoreGConfPrivate {
    GConfClient *gconf;
};

static void entangle_config_store_gconf_interface_init(gpointer g_iface,
                                                   gpointer iface_data);

G_DEFINE_TYPE_EXTENDED(EntangleConfigStoreGConf, entangle_config_store_gconf, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(ENTANGLE_TYPE_CONFIG_STORE, entangle_config_store_gconf_interface_init));


static void entangle_config_store_gconf_finalize(GObject *object)
{
    EntangleConfigStoreGConf *config = ENTANGLE_CONFIG_STORE_GCONF(object);
    EntangleConfigStoreGConfPrivate *priv = config->priv;

    ENTANGLE_DEBUG("Finalize config %p", object);

    g_object_unref(priv->gconf);

    G_OBJECT_CLASS (entangle_config_store_gconf_parent_class)->finalize (object);
}


static void entangle_config_store_gconf_class_init(EntangleConfigStoreGConfClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = entangle_config_store_gconf_finalize;

    g_type_class_add_private(klass, sizeof(EntangleConfigStoreGConfPrivate));
}


EntangleConfigStoreGConf *entangle_config_store_gconf_new(void)
{
    return ENTANGLE_CONFIG_STORE_GCONF(g_object_new(ENTANGLE_TYPE_CONFIG_STORE_GCONF, NULL));
}


static void entangle_config_store_gconf_init(EntangleConfigStoreGConf *store)
{
    EntangleConfigStoreGConfPrivate *priv;

    priv = store->priv = ENTANGLE_CONFIG_STORE_GCONF_GET_PRIVATE(store);
    memset(priv, 0, sizeof(*priv));
}


static gchar *entangle_config_store_gconf_make_key(EntangleConfigStore *store G_GNUC_UNUSED, EntangleConfigSet *set, EntangleConfigEntry *entry)
{
    return g_strdup_printf("/apps/entangle/%s/%s",
                           entangle_config_set_get_name(set),
                           entangle_config_entry_get_name(entry));
}


static gboolean entangle_config_store_gconf_has_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry)
{
    EntangleConfigStoreGConf *gconf = ENTANGLE_CONFIG_STORE_GCONF(store);
    EntangleConfigStoreGConfPrivate *priv = ENTANGLE_CONFIG_STORE_GCONF_GET_PRIVATE(gconf);
    gchar *key = entangle_config_store_gconf_make_key(store, set, entry);
    GConfValue *val;
    gboolean has;

    val = gconf_client_get_without_default(priv->gconf, key, NULL);

    has = val != NULL ? TRUE : FALSE;

    if (val)
        g_object_unref(val);

    g_free(key);
    return has;
}


static gboolean entangle_config_store_gconf_clear_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry)
{
    EntangleConfigStoreGConf *gconf = ENTANGLE_CONFIG_STORE_GCONF(store);
    EntangleConfigStoreGConfPrivate *priv = ENTANGLE_CONFIG_STORE_GCONF_GET_PRIVATE(gconf);
    gchar *key = entangle_config_store_gconf_make_key(store, set, entry);

    gconf_client_unset(priv->gconf, key, NULL);

    g_free(key);
    return TRUE;
}


static gboolean entangle_config_store_gconfvalue_to_gvalue(int datatype, GConfValue *gconfvalue, GValue *gvalue)
{
    switch (datatype) {
    case ENTANGLE_CONFIG_ENTRY_STRING:
        g_value_set_string(gvalue, gconf_value_get_string(gconfvalue));
        break;

    case ENTANGLE_CONFIG_ENTRY_INT:
        g_value_set_int(gvalue, gconf_value_get_int(gconfvalue));
        break;

    case ENTANGLE_CONFIG_ENTRY_DOUBLE:
        g_value_set_double(gvalue, gconf_value_get_float(gconfvalue));
        break;

    case ENTANGLE_CONFIG_ENTRY_BOOL:
        g_value_set_boolean(gvalue, gconf_value_get_bool(gconfvalue));
        break;


    case ENTANGLE_CONFIG_ENTRY_STRING_LIST: {
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GPtrArray *garray = g_ptr_array_new();
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            const gchar *str = gconf_value_get_string(gconflistvalue);

            g_ptr_array_add(garray, g_strdup(str));

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, garray);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_INT_LIST: {
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GArray *garray = g_array_new(FALSE, TRUE, sizeof(int));
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            int val = gconf_value_get_int(gconflistvalue);

            g_array_append_val(garray, val);

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, garray);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_DOUBLE_LIST:{
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GArray *garray = g_array_new(FALSE, TRUE, sizeof(double));
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            double val = gconf_value_get_float(gconflistvalue);

            g_array_append_val(garray, val);

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, garray);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_BOOL_LIST:{
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GArray *garray = g_array_new(FALSE, TRUE, sizeof(gboolean));
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            gboolean val = gconf_value_get_bool(gconflistvalue);

            g_array_append_val(garray, val);

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, garray);
    }   break;


    case ENTANGLE_CONFIG_ENTRY_STRING_HASH: {
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GHashTable *ghash = g_hash_table_new(g_str_hash, g_str_equal);
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            GConfValue *car = gconf_value_get_car(gconflistvalue);
            GConfValue *cdr = gconf_value_get_cdr(gconflistvalue);
            const gchar *key = gconf_value_get_string(car);
            const gchar *val = gconf_value_get_string(cdr);

            g_hash_table_insert(ghash, g_strdup(key), g_strdup(val));

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, ghash);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_INT_HASH: {
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GHashTable *ghash = g_hash_table_new(g_str_hash, g_str_equal);
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            GConfValue *car = gconf_value_get_car(gconflistvalue);
            GConfValue *cdr = gconf_value_get_cdr(gconflistvalue);
            const gchar *key = gconf_value_get_string(car);
            int val = gconf_value_get_int(cdr);

            g_hash_table_insert(ghash, g_strdup(key), GINT_TO_POINTER(val));

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, ghash);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_DOUBLE_HASH: {
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GHashTable *ghash = g_hash_table_new(g_str_hash, g_str_equal);
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            GConfValue *car = gconf_value_get_car(gconflistvalue);
            GConfValue *cdr = gconf_value_get_cdr(gconflistvalue);
            const gchar *key = gconf_value_get_string(car);
            double val = gconf_value_get_float(cdr);
            double *valptr = g_new(double, 1);
            *valptr = val;

            g_hash_table_insert(ghash, g_strdup(key), valptr);

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, ghash);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_BOOL_HASH: {
        GSList *gconflist = gconf_value_get_list(gconfvalue);
        GHashTable *ghash = g_hash_table_new(g_str_hash, g_str_equal);
        while (gconflist) {
            GConfValue *gconflistvalue = gconflist->data;
            GConfValue *car = gconf_value_get_car(gconflistvalue);
            GConfValue *cdr = gconf_value_get_cdr(gconflistvalue);
            const gchar *key = gconf_value_get_string(car);
            gboolean val = gconf_value_get_bool(cdr);

            g_hash_table_insert(ghash, g_strdup(key), GINT_TO_POINTER((int)val));

            gconflist = gconflist->next;
        }
        g_value_set_boxed(gvalue, ghash);
    }   break;

    default:
        return FALSE;
    }

    return TRUE;
}


static gboolean entangle_config_store_gconf_read_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry, GValue *value)
{
    EntangleConfigStoreGConf *gconf = ENTANGLE_CONFIG_STORE_GCONF(store);
    EntangleConfigStoreGConfPrivate *priv = ENTANGLE_CONFIG_STORE_GCONF_GET_PRIVATE(gconf);
    gchar *key = entangle_config_store_gconf_make_key(store, set, entry);
    GConfValue *gconfvalue;
    gboolean ret = FALSE;

    gconfvalue = gconf_client_get_without_default(priv->gconf, key, NULL);

    if (gconfvalue) {
        ret = entangle_config_store_gconfvalue_to_gvalue(entangle_config_entry_get_datatype(entry), gconfvalue, value);
        g_object_unref(gconfvalue);
    }

    g_free(key);
    return ret;
}


static gboolean entangle_config_store_gvalue_to_gconfvalue(int datatype, GValue *gvalue, GConfValue **gconfvalue)
{
    switch (datatype) {
    case ENTANGLE_CONFIG_ENTRY_STRING:
        *gconfvalue = gconf_value_new(GCONF_VALUE_STRING);
        gconf_value_set_string(*gconfvalue, g_value_get_string(gvalue));
        break;

    case ENTANGLE_CONFIG_ENTRY_INT:
        *gconfvalue = gconf_value_new(GCONF_VALUE_INT);
        gconf_value_set_int(*gconfvalue, g_value_get_int(gvalue));
        break;

    case ENTANGLE_CONFIG_ENTRY_DOUBLE:
        *gconfvalue = gconf_value_new(GCONF_VALUE_FLOAT);
        gconf_value_set_float(*gconfvalue, g_value_get_double(gvalue));
        break;

    case ENTANGLE_CONFIG_ENTRY_BOOL:
        *gconfvalue = gconf_value_new(GCONF_VALUE_BOOL);
        gconf_value_set_bool(*gconfvalue, g_value_get_boolean(gvalue));
        break;


    case ENTANGLE_CONFIG_ENTRY_STRING_LIST: {
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GPtrArray *garray = g_value_get_boxed(gvalue);
        for (int i = 0 ; i < garray->len ; i++) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_STRING);
            const gchar *val = g_ptr_array_index(garray, i);

            gconf_value_set_string(gconflistvalue, val);

            gconflist = g_slist_append(gconflist, gconflistvalue);
        }
        gconf_value_set_list(*gconfvalue, gconflist);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_INT_LIST: {
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GArray *garray = g_value_get_boxed(gvalue);
        for (int i = 0 ; i < garray->len ; i++) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_INT);
            int val = g_array_index(garray, int, i);

            gconf_value_set_int(gconflistvalue, val);

            gconflist = g_slist_append(gconflist, gconflistvalue);
        }
        gconf_value_set_list(*gconfvalue, gconflist);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_DOUBLE_LIST:{
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GArray *garray = g_value_get_boxed(gvalue);
        for (int i = 0 ; i < garray->len ; i++) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_INT);
            double val = g_array_index(garray, double, i);

            gconf_value_set_float(gconflistvalue, val);

            gconflist = g_slist_append(gconflist, gconflistvalue);
        }
        gconf_value_set_list(*gconfvalue, gconflist);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_BOOL_LIST:{
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GArray *garray = g_value_get_boxed(gvalue);
        for (int i = 0 ; i < garray->len ; i++) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_INT);
            gboolean val = g_array_index(garray, gboolean, i);

            gconf_value_set_bool(gconflistvalue, val);

            gconflist = g_slist_append(gconflist, gconflistvalue);
        }
        gconf_value_set_list(*gconfvalue, gconflist);
    }   break;


    case ENTANGLE_CONFIG_ENTRY_STRING_HASH: {
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GHashTable *ghash = g_value_get_boxed(gvalue);
        GList *ghashkeys = g_hash_table_get_keys(ghash);
        GList *tmp = ghashkeys;
        while (tmp) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_PAIR);
            GConfValue *car = gconf_value_new(GCONF_VALUE_STRING);
            GConfValue *cdr = gconf_value_new(GCONF_VALUE_STRING);
            const gchar *key = tmp->data;
            const gchar *val = g_hash_table_lookup(ghash, key);

            gconf_value_set_string(car, key);
            gconf_value_set_string(car, val);

            gconf_value_set_car(gconflistvalue, car);
            gconf_value_set_cdr(gconflistvalue, cdr);

            gconflist = g_slist_append(gconflist, gconflistvalue);

            tmp = tmp->next;
        }
        gconf_value_set_list(*gconfvalue, gconflist);
        g_list_free(ghashkeys);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_INT_HASH: {
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GHashTable *ghash = g_value_get_boxed(gvalue);
        GList *ghashkeys = g_hash_table_get_keys(ghash);
        GList *tmp = ghashkeys;
        while (tmp) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_PAIR);
            GConfValue *car = gconf_value_new(GCONF_VALUE_STRING);
            GConfValue *cdr = gconf_value_new(GCONF_VALUE_INT);
            const gchar *key = tmp->data;
            int val = GPOINTER_TO_INT(g_hash_table_lookup(ghash, key));

            gconf_value_set_string(car, key);
            gconf_value_set_int(car, val);

            gconf_value_set_car(gconflistvalue, car);
            gconf_value_set_cdr(gconflistvalue, cdr);

            gconflist = g_slist_append(gconflist, gconflistvalue);

            tmp = tmp->next;
        }
        gconf_value_set_list(*gconfvalue, gconflist);
        g_list_free(ghashkeys);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_DOUBLE_HASH: {
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GHashTable *ghash = g_value_get_boxed(gvalue);
        GList *ghashkeys = g_hash_table_get_keys(ghash);
        GList *tmp = ghashkeys;
        while (tmp) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_PAIR);
            GConfValue *car = gconf_value_new(GCONF_VALUE_STRING);
            GConfValue *cdr = gconf_value_new(GCONF_VALUE_FLOAT);
            const gchar *key = tmp->data;
            gdouble *val = g_hash_table_lookup(ghash, key);

            gconf_value_set_string(car, key);
            gconf_value_set_float(car, *val);

            gconf_value_set_car(gconflistvalue, car);
            gconf_value_set_cdr(gconflistvalue, cdr);

            gconflist = g_slist_append(gconflist, gconflistvalue);

            tmp = tmp->next;
        }
        gconf_value_set_list(*gconfvalue, gconflist);
        g_list_free(ghashkeys);
    }   break;

    case ENTANGLE_CONFIG_ENTRY_BOOL_HASH: {
        *gconfvalue = gconf_value_new(GCONF_VALUE_LIST);
        GSList *gconflist = NULL;
        GHashTable *ghash = g_value_get_boxed(gvalue);
        GList *ghashkeys = g_hash_table_get_keys(ghash);
        GList *tmp = ghashkeys;
        while (tmp) {
            GConfValue *gconflistvalue = gconf_value_new(GCONF_VALUE_PAIR);
            GConfValue *car = gconf_value_new(GCONF_VALUE_STRING);
            GConfValue *cdr = gconf_value_new(GCONF_VALUE_BOOL);
            const gchar *key = tmp->data;
            int val = GPOINTER_TO_INT(g_hash_table_lookup(ghash, key));

            gconf_value_set_string(car, key);
            gconf_value_set_bool(car, (gboolean)val);

            gconf_value_set_car(gconflistvalue, car);
            gconf_value_set_cdr(gconflistvalue, cdr);

            gconflist = g_slist_append(gconflist, gconflistvalue);

            tmp = tmp->next;
        }
        gconf_value_set_list(*gconfvalue, gconflist);
        g_list_free(ghashkeys);
    }   break;

    default:
        return FALSE;
    }

    return TRUE;
}


static gboolean entangle_config_store_gconf_write_set_entry(EntangleConfigStore *store, EntangleConfigSet *set, EntangleConfigEntry *entry, GValue *value)
{
    EntangleConfigStoreGConf *gconf = ENTANGLE_CONFIG_STORE_GCONF(store);
    EntangleConfigStoreGConfPrivate *priv = ENTANGLE_CONFIG_STORE_GCONF_GET_PRIVATE(gconf);
    gchar *key = entangle_config_store_gconf_make_key(store, set, entry);
    GConfValue *gconfvalue = NULL;
    gboolean ret = FALSE;

    ret = entangle_config_store_gvalue_to_gconfvalue(entangle_config_entry_get_datatype(entry), value, &gconfvalue);

    if (!ret)
        return FALSE;

    gconf_client_set(priv->gconf, key, gconfvalue, NULL);

    g_object_unref(gconfvalue);

    g_free(key);
    return TRUE;
}


static void entangle_config_store_gconf_interface_init(gpointer g_iface,
                                                   gpointer iface_data G_GNUC_UNUSED)
{
    EntangleConfigStoreInterface *iface = g_iface;

    iface->store_has_set_entry = entangle_config_store_gconf_has_set_entry;
    iface->store_clear_set_entry = entangle_config_store_gconf_clear_set_entry;
    iface->store_read_set_entry = entangle_config_store_gconf_read_set_entry;
    iface->store_write_set_entry = entangle_config_store_gconf_write_set_entry;
}


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  tab-width: 8
 * End:
 */
