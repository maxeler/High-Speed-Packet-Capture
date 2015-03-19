/*
 * frame_part.c
 *
 */

#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>

#include "frame.h"


frame_t FRAME_EMPTY = {0};

frame_t* frame_init( int sof, int eof, int mod, uint64_t data )
{
	frame_t* this = malloc(sizeof(frame_t));
	if( this == NULL )
	{
		return NULL;
	}

	this->sof = sof;
	this->eof = eof;
	this->mod = mod;
	this->data = data;

	return this;
}

void frame_free( frame_t* this )
{
	assert(this != NULL);

	free(this);
}

unsigned int frame_get_sof( frame_t* this )
{
	assert(this != NULL);

	return this->sof;
}

unsigned int frame_get_eof( frame_t* this )
{
	assert(this != NULL);

	return this->eof;
}

unsigned int frame_get_mod( frame_t* this )
{
	assert(this != NULL);

	return this->mod;
}

ssize_t frame_get_size( frame_t* this )
{
	assert(this != NULL);

	ssize_t data_size = sizeof(this->data);

	int size;
	if( this->mod == 0 )
	{ // full partial frame
		size = data_size;
	}
	else // part->mod != 0
	{
		size = this->mod;
	}

	return size;
}

const uint64_t* frame_get_data( frame_t* this )
{
	assert(this != NULL);

	return &(this->data);
}
