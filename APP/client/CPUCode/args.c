/*
 * args.c
 */

#include <string.h>
#include <stdio.h>

#include "args.h"
#include "log.h"


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
			int level = log_level_from_str(arg);
			if( level == -1 )
			{
				argp_error(state, "Invalid level '%s'\n", arg);
			}

			arguments->log_level = level;

			break;
		}
		case 'l':
		{
			const char* file_str = arg;

			arguments->local_enabled = 1;

			FILE* file = fopen(file_str, "w+");
			if( file == NULL )
			{
				argp_error(state, "Unable to open file '%s' (errno=%d '%s').", file_str, errno, strerror(errno));
			}
			arguments->local_file = file;

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

					break;
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
				{ // dfe_ip
					const char* ip_str = arg;

					int status = inet_aton(ip_str, &arguments->dfe_ip);
					if( status == 0 )
					{
						argp_error(state, "Unable to parse ip address '%s'.", ip_str);
					}

					break;
				}
				case 1:
				{ // dfe_netmask
					const char* ip_str = arg;

					int status = inet_aton(ip_str, &arguments->dfe_netmask);
					if( status == 0 )
					{
						argp_error(state, "Unable to parse ip address '%s'.", ip_str);
					}

					break;
				}
				default:
				{ // unknown
					argp_usage(state);

					break;
				}
			}

			break;
		}
		case ARGP_KEY_END:
		{
			if( arguments->local_enabled == 0 &&
				arguments->remote_enabled == 0 )
			{
				argp_error(state, "At least one type of capture mode must be configured.");
			}

			if( arguments->remote_enabled )
			{
				if( state->arg_num < 2 )
				{
					argp_usage(state);
				}

				if( arguments->ipsA_len == 0 || arguments->ipsB_len == 0 )
				{
					argp_error(state, "At least one server must be configured for each server type.");
				}
			}

			break;
		}
		default:
		{
			return ARGP_ERR_UNKNOWN;

			break;
		}
	}

	return 0;
}

static char doc[] = "";

static char args_doc[] = "[dfe-ip dfe-netmask]";

static struct argp_option options[] =
{
	{"verbose", 'v', "level", 0, "Set log level", 0},
	{"local", 'l', "pcap-file", 0, "Enable local write mode", 0},
	{"remote", 'r', "type ip", 0, "Enable remote write mode", 0},
	{0}
};

struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

