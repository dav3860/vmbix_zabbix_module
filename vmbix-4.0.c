/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
** This program reuses some code from Zabbix SIA & DAISUKE Ikeda
** Copyright (C) 2014 dav3860
** Copyright (C) 2014 DAISUKE Ikeda
** Copyright (C) 2001-2014 Zabbix SIA
**
**/

#include "common.h"
#include "threads.h"
#include "comms.h"
#include "cfg.h"
#include "log.h"
#include "zbxgetopt.h"
#include "sysinc.h"
#include "module.h"
#include "../../libs/zbxcrypto/tls.h"

#ifndef _WINDOWS
#	include "zbxnix.h"
#endif

#define VMBIX_MODULE_VERSION "1.3"
#define VMBIX_HOST "127.0.0.1"
#define VMBIX_PORT 12050
#define CONFIG_FILE "/etc/zabbix/vmbix_module.conf"

/* the variable keeps timeout setting for item processing */
static int  item_timeout                  = 30;
unsigned int	configured_tls_connect_mode = ZBX_TCP_SEC_UNENCRYPTED;

/* module SHOULD define internal functions as static and use a naming pattern different from Zabbix internal */
/* symbols (zbx_*) and loadable module API functions (zbx_module_*) to avoid conflicts                       */
static int    CONFIG_MODULE_TIMEOUT =                 30;

static int    zbx_module_vmbix(AGENT_REQUEST *request, AGENT_RESULT *result);
static int    zbx_module_vmbix_ping(AGENT_REQUEST *request, AGENT_RESULT *result);

static ZBX_METRIC keys[] =
/* KEY          FLAG           FUNCTION               TEST PARAMETERS */
{
  {"vmbix",      CF_HAVEPARAMS, zbx_module_vmbix,      NULL},
  {"vmbix.ping", CF_HAVEPARAMS, zbx_module_vmbix_ping, NULL},
  {NULL}
};

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_load_config                                           *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - module initialization failed               *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
static void zbx_module_load_config()
{
  zabbix_log(LOG_LEVEL_INFORMATION, "Loading VmBix module configuration file %s", CONFIG_FILE);
  static struct cfg_line cfg[] =
  {
    {"VmBixModuleTimeout", &CONFIG_MODULE_TIMEOUT, TYPE_INT,    PARM_OPT, 1, 600},   
    { NULL },
  };

  parse_cfg_file(CONFIG_FILE, cfg, ZBX_CFG_FILE_OPTIONAL, ZBX_CFG_STRICT);
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_get_value                                             *
 *                                                                            *
 * Purpose: connect with Zabbix agent protocol, receive and print value       *
 *                                                                            *
 * Parameters: con_string - vCenter or ESX connection string, format :        *
 *                                               tcp://hostname:port          *
 *             source_ip - source IP address                                  *
 *             key  - item's key                                              *
 *                                                                            *
 ******************************************************************************/
static int      zbx_module_get_value(const char *source_ip, const char *host, unsigned short port, const char *key, char **value)
{
    zbx_socket_t    s;
    int             ret;
    ssize_t         bytes_received = -1;
    char            *request;
    
    // assert(value);
    
    // *value = NULL;
    
    if (SUCCEED == (ret = zbx_tcp_connect(&s, source_ip, host, port, GET_SENDER_TIMEOUT,
            configured_tls_connect_mode, NULL, NULL)))
    {
        request = zbx_dsprintf(NULL, "%s\n", key);
        
        if (SUCCEED == (ret = zbx_tcp_send(&s, request)))
        {
            // zbx_tcp_recv_exts only accepts two arguments in zabbix 4.0
            if (0 < (bytes_received = zbx_tcp_recv_ext(&s, ZBX_TCP_READ_UNTIL_CLOSE)))
            {
                if (0 != strcmp(s.buffer, ZBX_NOTSUPPORTED) || sizeof(ZBX_NOTSUPPORTED) >= s.read_bytes)
                {
                    zbx_rtrim(s.buffer, "\r\n");
                    *value = strdup(s.buffer);
                }
        
            }
        }
        
        zbx_free(request);
        zbx_tcp_close(&s);
    }

    return ret;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_api_version                                           *
*                                                                            *
* Purpose: returns version number of the module interface                    *
*                                                                            *
* Return value: ZBX_MODULE_API_VERSION_ONE - the only version supported by   *
*               Zabbix currently                                             *
*                                                                            *
******************************************************************************/
int    zbx_module_api_version()
{
    return ZBX_MODULE_API_VERSION;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_item_timeout                                          *
*                                                                            *
* Purpose: set timeout value for processing of items                         *
*                                                                            *
* Parameters: timeout - timeout in seconds, 0 - no timeout set               *
*                                                                            *
******************************************************************************/
void    zbx_module_item_timeout(int timeout)
{
    item_timeout = timeout;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_item_list                                             *
*                                                                            *
* Purpose: returns list of item keys supported by the module                 *
*                                                                            *
* Return value: list of item keys                                            *
*                                                                            *
******************************************************************************/
ZBX_METRIC    *zbx_module_item_list()
{
    return keys;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_init                                                  *
*                                                                            *
* Purpose: the function is called on agent startup                           *
*          It should be used to call any initialization routines             *
*                                                                            *
* Return value: ZBX_MODULE_OK - success                                      *
*               ZBX_MODULE_FAIL - module initialization failed               *
*                                                                            *
* Comment: the module won't be loaded in case of ZBX_MODULE_FAIL             *
*                                                                            *
******************************************************************************/
int    zbx_module_init()
{
    zabbix_log(LOG_LEVEL_INFORMATION, "VmBix module version %s", VMBIX_MODULE_VERSION);

    srand(time(NULL));

    zbx_module_load_config();

    zabbix_log(LOG_LEVEL_DEBUG, "VmBix  Timeout: %d (s)",               CONFIG_MODULE_TIMEOUT);   
    zabbix_log(LOG_LEVEL_DEBUG, "Zabbix Version: %s", ZABBIX_VERSION);

    return ZBX_MODULE_OK;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_uninit                                                *
*                                                                            *
* Purpose: the function is called on agent shutdown                          *
*          It should be used to cleanup used resources if there are any      *
*                                                                            *
* Return value: ZBX_MODULE_OK - success                                      *
*               ZBX_MODULE_FAIL - function failed                            *
*                                                                            *
******************************************************************************/
int    zbx_module_uninit()
{
    return ZBX_MODULE_OK;
}

int param_is_empty(char *param_to_check) {
    return (param_to_check[0] == '\0') ? 1: 0;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_concat                                                           *
 *                                                                            *
 * Purpose: Concatenate two or more strings                                   *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
char* zbx_module_concat(int count, ...)
{
    va_list ap;
    int i;

    // Find required length to store merged string
    int len = 1; // room for NULL
    va_start(ap, count);
    for(i=0 ; i<count ; i++)
        len += strlen(va_arg(ap, char*));
    va_end(ap);

    // Allocate memory to concat strings
    char *merged = calloc(sizeof(char),len);
    int null_pos = 0;

    // Actually concatenate strings
    va_start(ap, count);
    for(i=0 ; i<count ; i++)
    {
        char *s = va_arg(ap, char*);
        zbx_strlcpy(merged + null_pos, s, 256);
        null_pos += strlen(s);
    }
    va_end(ap);

    return merged;
}

/******************************************************************************
*                                                                            *
* Function: zbx_module_vmbix                                                 *
*                                                                            *
* Purpose: queries VMBix                                                     *
*                                                                            *
* Return value: list of values                                               *
*                                                                            *
******************************************************************************/
int    zbx_module_vmbix(AGENT_REQUEST *request, AGENT_RESULT *result)
{
  int  ret = SUCCEED;
  char *value = NULL, *source_ip = NULL, *key = NULL, *conn = NULL;
  char host[100] = VMBIX_HOST;
  int  port = VMBIX_PORT;

  zbx_module_item_timeout(CONFIG_MODULE_TIMEOUT);

  if (request->nparam == 0 || request->nparam >= 7)
  {
    /* set optional error message */
    SET_MSG_RESULT(result, strdup("Incorrect number of parameters, expecting at least one parameter."));
    return SYSINFO_RET_FAIL;
  }

  if (strstr(get_rparam(request, 0), "tcp://") != NULL) { // Connection string was provided
    conn = get_rparam(request, 0);
    memmove(conn, conn+6, strlen(conn));     
    sscanf(conn, "%99[^:]:%99d[^\n]", host, &port);
   
    // Construct query
    if (request->nparam == 2)
      key = zbx_module_concat(1, get_rparam(request, 1));
    if (request->nparam == 3)
      key = zbx_module_concat(4, get_rparam(request, 1), "[", get_rparam(request, 2), "]");
    if (request->nparam == 4)
      key = zbx_module_concat(6, get_rparam(request, 1), "[", get_rparam(request, 2), ",", get_rparam(request, 3), "]");
    if (request->nparam == 5)
      key = zbx_module_concat(8, get_rparam(request, 1), "[", get_rparam(request, 2), ",", get_rparam(request, 3), ",", get_rparam(request, 4), "]");
    if (request->nparam == 6)
      key = zbx_module_concat(10, get_rparam(request, 1), "[", get_rparam(request, 2), ",", get_rparam(request, 3), ",", get_rparam(request, 4), ",", get_rparam(request, 5), "]");

  } else {
    // Construct query
    if (request->nparam == 1)
      key = zbx_module_concat(1, get_rparam(request, 0));
    if (request->nparam == 2)
      key = zbx_module_concat(4, get_rparam(request, 0), "[", get_rparam(request, 1), "]");
    if (request->nparam == 3)
      key = zbx_module_concat(6, get_rparam(request, 0), "[", get_rparam(request, 1), ",", get_rparam(request, 2), "]");
    if (request->nparam == 4)
      key = zbx_module_concat(8, get_rparam(request, 0), "[", get_rparam(request, 1), ",", get_rparam(request, 2), ",", get_rparam(request, 3), "]");
    if (request->nparam == 5)
      key = zbx_module_concat(10, get_rparam(request, 0), "[", get_rparam(request, 1), ",", get_rparam(request, 2), ",", get_rparam(request, 3), ",", get_rparam(request, 4), "]"); 
  }
  zabbix_log(LOG_LEVEL_DEBUG, "Using VmBix host %s:%d", host, port);
  
  if (SUCCEED == ret)
  {
    ret = zbx_module_get_value(source_ip, host, port, key, &value);
    
    if (SUCCEED == ret && value != NULL) {
      zabbix_log(LOG_LEVEL_DEBUG, "Received reply from VmBix. Query: %s, result: %s", strdup(key), strdup(value));
      SET_STR_RESULT(result, strdup(value));
    }
 
    zbx_free(value);
  }

  zbx_free(conn);
  zbx_free(key);
  zbx_free(source_ip);

  return  SYSINFO_RET_OK;
}

/*****************************************************************************
*                                                                            *
* Function: zbx_module_vmbix_ping                                            *
*                                                                            *
* Purpose: echoes back the first parameter                                   *
*                                                                            *
* Return value: the value it was invoked with                                               *
*                                                                            *
******************************************************************************/
int    zbx_module_vmbix_ping(AGENT_REQUEST *request, AGENT_RESULT *result)
{
  char *param;

  if (1 != request->nparam)
  {
    SET_MSG_RESULT(result, strdup("Invalid number of key parameters"));
    return (SYSINFO_RET_FAIL);
  }

  param = get_rparam(request, 0);

  SET_STR_RESULT(result, strdup(param));

  return (SYSINFO_RET_OK);
}
