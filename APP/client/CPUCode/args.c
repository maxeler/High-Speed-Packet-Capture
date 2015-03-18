/*
 * args.c
 */
#include <string.h>

#include "args.h"


static const char* SERVER_TYPE_A_STR = "A";
static const char* SERVER_TYPE_B_STR = "B";

static server_type_e server_type_from_str( const char* str )
{
	if( strcmp(SERVER_TYPE_A_STR, str) == 0 )
	{
		return SERVER_TYPE_A;
	}
	else if( strcmp(SERVER_TYPE_B_STR, str) == 0 )
	{
		return SERVER_TYPE_B;
	}
	else
	{
		return SERVER_TYPE_UNDEF;
	}
}

static error_t parse_opt( int key, char *arg, struct argp_state *state )
{
	arguments_t *arguments = state->input;

	switch( key )
	{
		case 'v':
		{
			arguments->log_level = atoi(arg);
			break;
		}
		case 'l':
		{
			arguments->local_enabled = 1;
			arguments->local_file = arg;
			break;
		}
		case 'r':
		{
			arguments->remote_enabled = 1;

			// requires 2 args
			if( (state->next) >= state->argc )
			{
				argp_usage(state);
			}

			struct in_addr* ips = NULL;
			int* ips_len = NULL;
			const char* type_str = arg;
			const char* ip_str = state->argv[state->next];

			server_type_e type = server_type_from_str(type_str);
			switch( type )
			{
				case SERVER_TYPE_A:
				{
					ips = arguments->ipsA;
					ips_len = &arguments->ipsA_len;
					break;
				}
				case SERVER_TYPE_B:
				{
					ips = arguments->ipsB;
					ips_len = &arguments->ipsB_len;
					break;
				}
				default:
				{
					argp_error(state, "Unknown remote server type: '%s'.", type_str);
				}
			}

			(*ips_len)++;
			if( *ips_len > IPS_LEN_MAX )
			{
				argp_error(state, "Too many servers configured for type '%s'.", type_str);
			}

			int status = inet_aton(ip_str, &ips[*ips_len - 1]);
			if( status == 0 )
			{
				argp_error(state, "Unable to parse ip address '%s'.", ip_str);
			}

			state->next = state->next++;
			break;
		}
		case ARGP_KEY_ARG:
		{
			switch( state->arg_num )
			{
				case 0:
				{
					const char* ip_str = arg;
					int status = inet_aton(ip_str, &arguments->dfe_ip);
					if( status == 0 )
					{
						argp_error(state, "Unable to parse ip address '%s'.", ip_str);
					}
					break;
				}
				case 1:
				{
					const char* ip_str = arg;
					int status = inet_aton(ip_str, &arguments->dfe_netmask);
					if( status == 0 )
					{
						argp_error(state, "Unable to parse netmask '%s'.", ip_str);
					}
					break;
				}
				default:
				{
					argp_usage(state);
				}
			}
			break;
		}
		case ARGP_KEY_END:
		{
			if( state->arg_num < 2 )
			{
				argp_usage(state);
			}

			if( arguments->local_enabled == 0 &&
				arguments->remote_enabled == 0 )
			{
				argp_error(state, "At least one type of capture mode must be configured.");
			}

			if( arguments->ipsA_len == 0
				|| arguments->ipsB_len == 0 )
			{
				argp_error(state, "At least one server must be configured for each server type.");
			}

			break;
		}
		default:
		{
			return ARGP_ERR_UNKNOWN;
		}
	}

	return 0;
}

static char doc[] = "";

static char args_doc[] = "dfe-ip dfe-netmask";

static struct argp_option options[] = {
	{
		.name = "verbose",
		.key = 'v',
		.arg = "level",
		.flags = 0,
		.doc = "",
	},
	{
		.name = "local",
		.key = 'l',
		.arg = "pcap-file",
		.flags = 0,
		.doc = "Enable local write capture mode",
	},
	{
		.name = "remote",
		.key = 'r',
		.arg = "type ip",
		.flags = 0,
		.doc = "Enable remote write capture mode",
	},

	{0}
};

struct argp argp = {options, parse_opt, args_doc, doc};

