/*
 * frame.h
 *
 */

#ifndef FRAME_H_
#define FRAME_H_

#include <inttypes.h>


typedef struct frame_s
{
	unsigned int sof : 1;
	unsigned int eof : 1;
	unsigned int mod : 3;
	uint64_t data;
} frame_t;

extern frame_t FRAME_EMPTY;

frame_t* frame_init( int sof, int eof, int mod, uint64_t data );

frame_t* frame_parse( uint64_t raw[2] );

unsigned int frame_get_sof( frame_t* part );

unsigned int frame_get_eof( frame_t* part );

unsigned int frame_get_mod( frame_t* part );

ssize_t frame_get_size( frame_t* part );

const uint64_t* frame_get_data( frame_t* part );

void frame_free( frame_t* part );

#endif /* FRAME_H_ */
