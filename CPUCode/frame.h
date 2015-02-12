/*
 * frame.h
 *
 */

#ifndef FRAME_H_
#define FRAME_H_

#include <inttypes.h>

typedef struct frame_part_s frame_part_t;

frame_part_t* frame_init( int sof, int eof, int mod, uint64_t* data );

frame_part_t* frame_parse( uint64_t raw[2] );

int frame_get_sof( frame_part_t* part );

int frame_get_eof( frame_part_t* part );

int frame_get_mod( frame_part_t* part );

ssize_t frame_get_size( frame_part_t* part );

const uint64_t* frame_get_data( frame_part_t* part );

void frame_free( frame_part_t* part );

#endif /* FRAME_H_ */
