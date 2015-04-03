/*
 * args.c
 */

#include <stdlib.h>
#include <argp.h>
#include <string.h>

#include "args.h"


static error_t parse_opt( int key, char* arg, struct argp_state* state )
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
			switch( state->arg_num )
			{
				case 0:
				{ // interface name
					arguments->ifname = arg;

					break;
				}
				case 1:
				{ // capture file
					arguments->file = arg;

					break;
				}
				default:
				{ // extra args
					argp_usage(state);

					break;
				}
			}

			break;
		}
		case ARGP_KEY_END:
		{
			// check for correct number of args
			if( state->arg_num < 2 )
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

static struct argp_option options[] =
{
	{"verbose", 'v', "level", 0, "Set verbosity level", 0},
	{0}
};

static char doc[] = "";

static char args_doc[] = "interface file.pcap";

struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};
