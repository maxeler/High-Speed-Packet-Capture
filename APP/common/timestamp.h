/*
 * timestamp.h
 */

#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include <inttypes.h>
#include <time.h>

typedef struct timestamp_s
{
	unsigned int doubt : 1;
	unsigned int valid : 1;
	uint64_t value;
} timestamp_t;

extern timestamp_t TIMESTAMP_EMPTY;

timestamp_t* timestamp_init( int doubt, int valid, uint64_t value );

void timestamp_free( timestamp_t* this );

unsigned int timestamp_get_doubt( timestamp_t* this );

unsigned int timestamp_is_valid( timestamp_t* this );

uint64_t timestamp_get_value( timestamp_t* this );

time_t timestamp_get_seconds( timestamp_t* this );

suseconds_t timestamp_get_microseconds( timestamp_t* this );

uint32_t timestamp_get_nanoseconds( timestamp_t* this );

#endif /* TIMESTAMP_H_ */
