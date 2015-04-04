#include <stdlib.h>

#include "log.h"


const char* g_log_prepend = "";
int g_log_level = 0;

int log_level_from_str( const char* str )
{
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
