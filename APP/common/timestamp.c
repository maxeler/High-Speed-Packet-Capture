/*
 * timestamp.c
 */
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <math.h>

#include "timestamp.h"


timestamp_t TIMESTAMP_EMPTY = {0};

timestamp_t* timestamp_init( int doubt, int valid, uint64_t value )
{
	timestamp_t* this = malloc(sizeof(timestamp_t));
	if( this == NULL )
	{
		return NULL;
	}

	this->doubt = doubt;
	this->valid = valid;
	this->value = value;

	return this;
}

void timestamp_free( timestamp_t* this )
{
	assert(this != NULL);

	free(this);
}

unsigned int timestamp_get_doubt( timestamp_t* this )
{
	assert(this != NULL);

	return this->doubt;
}

unsigned int timestamp_is_valid( timestamp_t* this )
{
	assert(this != NULL);

	return this->valid;
}

uint64_t timestamp_get_value( timestamp_t* this )
{
	assert(this != NULL);

	return this->value;
}

time_t timestamp_get_seconds( timestamp_t* this )
{
	return this->value / ((uint64_t) 1e8);
}

uint32_t timestamp_get_nanoseconds( timestamp_t* this )
{
	return (this->value % ((uint64_t) 1e8)) * 1e1;
}

suseconds_t timestamp_get_microseconds( timestamp_t* this )
{
	return timestamp_get_nanoseconds(this) / ((uint64_t) 1e3);
}
