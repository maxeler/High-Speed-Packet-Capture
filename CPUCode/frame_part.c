/*
 * frame_part.c
 *
 */

#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include "frame_part.h"

struct frame_part_s
{
	int sof;
	int eof;
	int mod;
	uint64_t data;
};

frame_part_t* frame_part_init( int sof, int eof, int mod, uint64_t* data )
{
	assert(data != NULL);

	frame_part_t* part = malloc(sizeof(frame_part_t));
	assert(part != NULL);

	part->sof = sof;
	part->eof = eof;
	part->mod = mod;
	part->data = *data;

	return part;
}

frame_part_t* frame_part_parse( uint64_t raw[2] )
{
	assert(&raw != NULL);

	uint64_t* data = &raw[0];
	int eof = (raw[1] >> 0) & 0x1;
	int sof = (raw[1] >> 1) & 0x1;
	int mod = (raw[1] >> 2) & 0x7;

	return frame_part_init(sof, eof, mod, data);
}

int frame_part_get_sof( frame_part_t* part )
{
	return part->sof;
}

int frame_part_get_eof( frame_part_t* part )
{
	return part->eof;
}

int frame_part_get_mod( frame_part_t* part )
{
	return part->mod;
}

ssize_t frame_part_get_size( frame_part_t* part )
{
	ssize_t data_size = sizeof(part->data);

	int size;
	if( part->mod == 0 )
	{ // full partial frame
		size = data_size;
	}
	else // part->mod != 0
	{
		size = data_size - part->mod;
	}

	return size;
}

const uint64_t* frame_part_get_data( frame_part_t* part )
{
	return &(part->data);
}

void frame_part_free( frame_part_t* part )
{
	assert(part != NULL);
	free(part);
}
