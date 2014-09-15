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

#define CONFIG_FILE "/etc/zabbix/vmbix_module.conf"

static int	item_timeout = 30;
static int CONFIG_MODULE_TIMEOUT	= 30;
static char *CONFIG_VMBIX_HOST = NULL;
unsigned short CONFIG_VMBIX_PORT = 12050;

int    zbx_module_vmbix(AGENT_REQUEST *request, AGENT_RESULT *result);
int    zbx_module_vmbix_echo(AGENT_REQUEST *request, AGENT_RESULT *result);

static ZBX_METRIC keys[] =
/* KEY               FLAG           FUNCTION                TEST PARAMETERS */
{
    {"vmbix",   CF_HAVEPARAMS, zbx_module_vmbix,  NULL},
    {"vmbix.echo",   CF_HAVEPARAMS, zbx_module_vmbix_echo,  NULL},    
    {NULL}
};

/******************************************************************************
 *                                                                            *
 * Function: get_signal_handler                                               *
 *                                                                            *
 * Purpose: process signals                                                   *
 *                                                                            *
 * Parameters: sig - signal ID                                                *
 *                                                                            *
 * Return value:                                                              *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static void	get_signal_handler(int sig)
{
	if (SIGALRM == sig)
		zbx_error("Timeout while executing operation");

	exit(FAIL);
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_set_defaults                                          *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
static void	zbx_module_set_defaults()
{

	if (NULL == CONFIG_VMBIX_HOST) 
		CONFIG_VMBIX_HOST = "127.0.0.1";
}

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
static void	zbx_module_load_config()
{
  zabbix_log(LOG_LEVEL_INFORMATION, "Loading VmBix module configuration file %s",
	      CONFIG_FILE);
	static struct cfg_line cfg[] =
	{
    {"VmBixModuleTimeout",	&CONFIG_MODULE_TIMEOUT,	TYPE_INT,	PARM_OPT,	1,	600},
		{"VmBixPort",	&CONFIG_VMBIX_PORT,	TYPE_INT,	PARM_OPT,	1,	65535},
		{"VmBixHost",	&CONFIG_VMBIX_HOST,	TYPE_STRING,	PARM_OPT,	0,	0},
    { NULL },
	};

	parse_cfg_file(CONFIG_FILE, cfg, ZBX_CFG_FILE_OPTIONAL, ZBX_CFG_STRICT);
}

/******************************************************************************
 *                                                                            *
 * Function: get_value                                                        *
 *                                                                            *
 * Purpose: connect to Zabbix agent and receive value for given key           *
 *                                                                            *
 * Parameters: host   - server name or IP address                             *
 *             port   - port number                                           *
 *             key    - item's key                                            *
 *                                                                            *
 * Return value: SUCCEED - ok, FAIL - otherwise                               *
 *             value  - retrieved value                                       *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
static int	get_value(const char *source_ip, const char *host, unsigned short port, const char *key, char **value)
{
	zbx_sock_t	s;
	int		ret;
	char		*buf, request[1024];

	assert(value);

	*value = NULL;

	if (SUCCEED == (ret = zbx_tcp_connect(&s, source_ip, host, port, GET_SENDER_TIMEOUT)))
	{
		zbx_snprintf(request, sizeof(request), "%s\n", key);

		if (SUCCEED == (ret = zbx_tcp_send(&s, request)))
		{
			if (SUCCEED == (ret = SUCCEED_OR_FAIL(zbx_tcp_recv_ext(&s, &buf, ZBX_TCP_READ_UNTIL_CLOSE, 0))))
			{
				zbx_rtrim(buf, "\r\n");
				*value = strdup(buf);
			}
		}

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
    return ZBX_MODULE_API_VERSION_ONE;
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
    srand(time(NULL));

    zbx_module_load_config();
    zbx_module_set_defaults();
    
    zabbix_log(LOG_LEVEL_DEBUG, "VmBix Timeout: %d (s)", CONFIG_MODULE_TIMEOUT);
    zabbix_log(LOG_LEVEL_DEBUG, "VmBix Host: %s", CONFIG_VMBIX_HOST);
    zabbix_log(LOG_LEVEL_DEBUG, "VmBix Port: %d", CONFIG_VMBIX_PORT);

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

/******************************************************************************
 *                                                                            *
 * Function: concat                                                           * 
 *                                                                            *
 * Purpose: Concatenate two or more strings                                   *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
char* concat(int count, ...)
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
  char *value = NULL, *host = NULL, *source_ip = NULL, *key = NULL;

  zbx_module_item_timeout(CONFIG_MODULE_TIMEOUT);

  if (request->nparam == 0 || request->nparam >= 6)
  {
       /* set optional error message */
       SET_MSG_RESULT(result, strdup("Incorrect number of parameters, expecting at least one parameter."));
       return SYSINFO_RET_FAIL;
  }

  // Construct query
  if (request->nparam == 1)
    key = concat(1, get_rparam(request, 0));
  if (request->nparam == 2)
    key = concat(4, get_rparam(request, 0), "[", get_rparam(request, 1), "]");
  if (request->nparam == 3)
    key = concat(6, get_rparam(request, 0), "[", get_rparam(request, 1), ",", get_rparam(request, 2), "]");
  if (request->nparam == 4)
    key = concat(8, get_rparam(request, 0), "[", get_rparam(request, 1), ",", get_rparam(request, 2), ",", get_rparam(request, 3), "]");
  if (request->nparam == 5)
    key = concat(10, get_rparam(request, 0), "[", get_rparam(request, 1), ",", get_rparam(request, 2), ",", get_rparam(request, 3), ",", get_rparam(request, 4), "]");

  if (NULL == key)
    SET_MSG_RESULT(result, strdup("Query is empty"));
    return SYSINFO_RET_FAIL;

  if (SUCCEED == ret)
  {
    zabbix_log(LOG_LEVEL_DEBUG, "Querying VmBix endpoint at %s:%s",
          CONFIG_VMBIX_HOST, CONFIG_VMBIX_PORT);  
    ret = get_value(source_ip, CONFIG_VMBIX_HOST, CONFIG_VMBIX_PORT, key, &value);

    if (SUCCEED == ret)
      zabbix_log(LOG_LEVEL_DEBUG, "Received reply from VmBix: %s", value);
      SET_STR_RESULT(result, strdup(value));

    zbx_free(value);
  }

  zbx_free(host);
  zbx_free(key);
  zbx_free(source_ip);

  return  SYSINFO_RET_OK;
}

/*****************************************************************************
*                                                                            *
* Function: zbx_module_vmbix_echo                                            *
*                                                                            *
* Purpose: echoes back the first parameter                                   *
*                                                                            *
* Return value: the value it was invoked with                                               *
*                                                                            *
******************************************************************************/
int    zbx_module_vmbix_echo(AGENT_REQUEST *request, AGENT_RESULT *result)
{
  const char *param;

  if (request->nparam != 1) {
    SET_MSG_RESULT(result, strdup("Invalid number of key parameters"));
    return (SYSINFO_RET_FAIL);
  }

  param = get_rparam(request, 0);

  SET_STR_RESULT(result, strdup(param));

  return (SYSINFO_RET_OK);
}
