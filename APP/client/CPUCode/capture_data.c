/*
 * capture_data.c
 *
 */
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include "capture_data.h"


struct capture_data_s
{
	timestamp_t* timestamp;
	frame_t* frame;
};


/*
 * Create time stamp from CPU capture data bits.
 * Data must not be NULL.
 */
static timestamp_t* parse_timestamp( uint64_t* data )
{
	assert(data != NULL);

	uint64_t value = data[0];
	int doubt = data[2] >> 0 & 0x1;
	int valid = data[2] >> 1 & 0x1;

	return timestamp_init(doubt, valid, value);
}

/*
 * Create frame from CPU capture data bits.
 * Data must not be NULL.
 */
static frame_t* parse_frame( uint64_t* data )
{
	assert(data != NULL);

	int sof = data[2] >> 2 & 0x1;
	int eof = data[2] >> 3 & 0x1;
	int mod = data[2] >> 4 & 0x7;
	uint64_t frame_data = data[1];

	return frame_init(sof, eof, mod, frame_data);
}

capture_data_t* capture_data_parse( uint64_t data[4] )
{
	assert(data != NULL);

	capture_data_t* this = malloc(sizeof(capture_data_t));
	if( this == NULL )
	{
		return NULL;
	}

	this->timestamp = parse_timestamp(data);
	this->frame = parse_frame(data);
	if( this->timestamp == NULL || this->frame == NULL )
	{
		return NULL;
	}

	return this;
}

void capture_data_free( capture_data_t* this )
{
	assert(this != NULL);

	timestamp_free(this->timestamp);
	frame_free(this->frame);
}

timestamp_t* capture_data_get_timestamp( capture_data_t* this )
{
	return this->timestamp;
}

frame_t* capture_data_get_frame( capture_data_t* this )
{
	return this->frame;
}


