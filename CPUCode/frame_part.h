/*
 * frame_part.h
 *
 */

#ifndef FRAME_PART_H_
#define FRAME_PART_H_

#include <inttypes.h>

typedef struct frame_part_s frame_part_t;

frame_part_t* frame_part_init( int sof, int eof, int mod, uint64_t* data );

frame_part_t* frame_part_parse( uint64_t raw[2] );

int frame_part_get_sof( frame_part_t* part );

int frame_part_get_eof( frame_part_t* part );

int frame_part_get_mod( frame_part_t* part );

ssize_t frame_part_get_size( frame_part_t* part );

const uint64_t* frame_part_get_data( frame_part_t* part );

void frame_part_free( frame_part_t* part );

#endif /* FRAME_PART_H_ */
