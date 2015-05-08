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

extern const timestamp_t TIMESTAMP_EMPTY;

timestamp_t* timestamp_init( int doubt, int valid, uint64_t value );

void timestamp_free( timestamp_t* timestamp );

unsigned int timestamp_get_doubt( timestamp_t* timestamp );

unsigned int timestamp_is_valid( timestamp_t* timestamp );

uint64_t timestamp_get_value( timestamp_t* timestamp );

time_t timestamp_get_seconds( timestamp_t* timestamp );

suseconds_t timestamp_get_microseconds( timestamp_t* timestamp );

uint32_t timestamp_get_nanoseconds( timestamp_t* timestamp );

#endif /* TIMESTAMP_H_ */
