/*
 * pcap.c
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#include "pcap.h"


static const uint16_t VERSION_MAJOR = 2;
static const uint16_t VERSION_MINOR = 4;
static const uint32_t MAGIC_NUMBER = 0xa1b23c4d;

typedef struct __attribute__((__packed__)) frameh_s
{
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
} frameh_t;

struct pcap_frame_s
{
	frameh_t* header;
	uint64_t* data;
	uint32_t max_size;
	int finalized;
};

typedef struct __attribute__((__packed__)) globalh_s
{
	uint32_t magic_number;   /* magic number */
	uint16_t version_major;  /* major version number */
	uint16_t version_minor;  /* minor version number */
	int32_t  thiszone;       /* GMT to local correction */
	uint32_t sigfigs;        /* accuracy of timestamps */
	uint32_t snaplen;        /* max length of captured packets, in octets */
	uint32_t network;        /* data link type */
} globalh_t;

struct pcap_s
{
	FILE* file;
	globalh_t* header;
};

static int globalh_write( globalh_t* header, FILE* file )
{
	fwrite(header, sizeof(*header), 1, file);
	return 0;
}

static int frameh_write( frameh_t* header, FILE* file )
{
	fwrite(header, sizeof(*header), 1, file);
	return 0;
}

static int frame_data_write( const uint64_t* data, ssize_t size, FILE* file )
{
	// todo: impl
	printf("SIZE: %ldB\n", size);
	int words = ceil(size/8.0);
	for( int i=0; i<words; i++ )
	{
		printf("DATA[%2d]: 0x%016"PRIx64"\n", i, data[i]);
	}
	fwrite(data, size, 1, file);

	return 0;
}

pcap_t* pcap_init( FILE* file, globalh_t* header )
{
	assert(file != NULL);

	pcap_t* pcap = malloc(sizeof(*pcap));
	assert(pcap != NULL);

	pcap->header = header;
	pcap->file = file;

	return pcap;
}

pcap_t* pcap_create( FILE* file, int32_t  thiszone, uint32_t network, uint32_t sigfigs, uint32_t snaplen )
{
	assert(file != NULL);

	// global header
	globalh_t* header = malloc(sizeof(*header));
	assert(header != NULL);

	header->version_major = VERSION_MAJOR;
	header->version_minor = VERSION_MINOR;
	header->magic_number = MAGIC_NUMBER;
	header->thiszone = thiszone;
	header->network = network;
	header->sigfigs = sigfigs;
	header->snaplen = snaplen;

	// pcap
	pcap_t* pcap = pcap_init(file, header);

	// write global header
	int error = globalh_write(header, file);
	if( error )
	{
		return NULL;
	}

	return pcap;
}

pcap_frame_t* pcap_frame_init( pcap_t* pcap, uint32_t ts_sec, uint32_t ts_usec )
{
	frameh_t* header = malloc(sizeof(*header));
	assert(header != NULL);

	header->ts_sec = ts_sec;
	header->ts_usec = ts_usec;
	header->orig_len = 0;
	header->incl_len = 0;

	pcap_frame_t* frame = malloc(sizeof(*frame));
	frame->header = header;
	frame->max_size = pcap->header->snaplen;
	frame->data = malloc(frame->max_size);
	frame->finalized = 0;
	assert(frame->data != NULL);

	return frame;
}

void pcap_frame_append( pcap_frame_t* frame, const uint64_t* data, uint32_t size )
{
	assert(frame != NULL);
	assert(data != NULL);
	assert(!frame->finalized);

	if( size != sizeof(*data))
	{
		frame->finalized = 1;
	}

	frameh_t* header = frame->header;
	header->orig_len += size;

	if( header->incl_len < frame->max_size )
	{ // within snaplen
		int index = header->incl_len % sizeof(*(frame->data));
		frame->data[index] = *data;

		header->incl_len += size;
	}
}

int pcap_frame_write( pcap_t* pcap, pcap_frame_t* frame )
{
	int error;

	error = frameh_write(frame->header, pcap->file);
	if( error ) return error;

	error = frame_data_write(frame->data, frame->header->incl_len, pcap->file);
	if( error ) return error;

	return 0;
}

void pcap_frame_free( pcap_frame_t* frame )
{
	assert(frame != NULL);

	if( frame->header != NULL )
	{
		free(frame->header);
	}

	free(frame->data);
}
