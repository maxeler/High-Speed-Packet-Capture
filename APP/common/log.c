/*
 * log.c
 */

#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include "log.h"


const char* g_log_prepend = "";
int g_log_level = 0;

int log_level_from_str( const char* str )
{
	assert(str != NULL);
	char* end;

	int level = strtol(str, &end, 10);
	if( *end != '\0' || !log_level_valid(level) )
	{ // invalid
		return -1;
	}
	else // !*end != '\0' && log_level_valid(level)
	{ // valid
		return level;
	}
}

void log_binary_append( int level, void* data, size_t size )
{
	assert(data != NULL);
	uint8_t* data8 = (uint8_t*) data;

	if( log_level_active(level) )
	{
		logf_append(level, "%zdB: ", size);

		int len = (size / sizeof(*data8)) + ((size % sizeof(*data8)) != 0);
		for( int i=0; i<len; i++ )
		{
			if( (i % 8 == 0) && (i != 0) )
			{
				logf_append(level, "%s", ".");
			}

			logf_append(level, "%02"PRIx8, data8[i]);
		}

		logf_append(level, "%s", "\n");
	}
}
