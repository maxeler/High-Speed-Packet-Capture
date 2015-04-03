/*
 * args.c
 */
#include <stdio.h>
#include <argp.h>
#include <string.h>

#include "args.h"


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
		case ARGP_KEY_ARG:
		{
			int arg_num = state->arg_num;
			if( state->argc == 2 )
			{
				// skip over ip arg parsing
				arg_num++;
			}

			switch( arg_num )
			{
				case 0:
				{ // ip
					const char* ip_str = arg;

					int status = inet_aton(ip_str, &arguments->server_ip);
					if( status == 0 )
					{
						argp_error(state, "Unable to parse ip address '%s'.", ip_str);
					}

					break;
				}
				case 1:
				{ // file
					const char* file_str = arg;

					FILE* file = fopen(file_str, "w+");
					if( file == NULL )
					{
						argp_error(state, "Unable to open file '%s' (errno=%d '%s').", file_str, errno, strerror(errno));
					}
					arguments->capture_file = file;

					break;
				}
				default:
				{
					argp_usage(state);

					break;
				}
			}

			break;
		}
		case ARGP_KEY_END:
		{
			if( state->arg_num < 1 || state->arg_num > 2 )
			{
				argp_usage(state);
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

static char args_doc[] = "[ip] file.pcap";

static struct argp_option options[] =
{
	{"verbose", 'v', "level", 0, "Set log level", 0},
	{0}
};

struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};
