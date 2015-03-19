/*
 * utils.c
 *
 */

#include <string.h>

#include "utils.h"


int max_net_connection_from_name( max_net_connection_t* connection, const char* name )
{
	int error = 0;

	// top
	if( strcmp("QSFP_TOP_10G_PORT1", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT1;
	}
	else if( strcmp("QSFP_TOP_10G_PORT2", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT2;
	}
	else if( strcmp("QSFP_TOP_10G_PORT3", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT3;
	}
	else if( strcmp("QSFP_TOP_10G_PORT4", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT4;
	}
	// mid
	else if( strcmp("QSFP_MID_10G_PORT1", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_MID_10G_PORT1;
	}
	else if( strcmp("QSFP_MID_10G_PORT2", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_MID_10G_PORT2;
	}
	else if( strcmp("QSFP_MID_10G_PORT3", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_MID_10G_PORT3;
	}
	else if( strcmp("QSFP_MID_10G_PORT4", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_MID_10G_PORT4;
	}
	// bot
	else if( strcmp("QSFP_BOT_10G_PORT1", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT1;
	}
	else if( strcmp("QSFP_BOT_10G_PORT2", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT2;
	}
	else if( strcmp("QSFP_BOT_10G_PORT3", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT3;
	}
	else if( strcmp("QSFP_BOT_10G_PORT4", name) == 0 )
	{
		*connection = MAX_NET_CONNECTION_QSFP_BOT_10G_PORT4;
	}
	else
	{
		error = 1;
	}

	return error;
}
