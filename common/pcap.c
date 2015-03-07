/*
 * pcap.c
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>

#include "pcap.h"


static const uint16_t VERSION_MAJOR = 2;
static const uint16_t VERSION_MINOR = 4;
static const uint32_t MAGIC_NUMBER = 0xa1b23c4d;

typedef struct __attribute__((__packed__)) packeth_s
{
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_nsec;        /* timestamp nanoseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
} packeth_t;

struct pcap_packet_s
{
	packeth_t* header;
	uint64_t* data;
	uint32_t max_len;
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

static int frameh_write( packeth_t* header, FILE* file )
{
	fwrite(header, sizeof(*header), 1, file);
	return 0;
}

static int frame_data_write( const uint64_t* data, ssize_t size, FILE* file )
{
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

pcap_packet_t* pcap_packet_init( pcap_t* pcap, uint32_t ts_sec, uint32_t ts_nsec )
{
	packeth_t* header = malloc(sizeof(*header));
	assert(header != NULL);

	header->ts_sec = ts_sec;
	header->ts_nsec = ts_nsec;
	header->orig_len = 0;
	header->incl_len = 0;

	pcap_packet_t* packet = malloc(sizeof(*packet));
	assert(packet != NULL);
	packet->header = header;
	packet->max_len = pcap->header->snaplen;
	packet->data = malloc(packet->max_len);
	assert(packet->data != NULL);
	packet->finalized = 0;

	return packet;
}

void pcap_packet_append( pcap_packet_t* packet, const uint64_t* data, uint32_t size )
{
	assert(packet != NULL);
	assert(data != NULL);
	assert(!packet->finalized);

	if( size != sizeof(*data))
	{
		packet->finalized = 1;
	}

	packeth_t* header = packet->header;
	header->orig_len += size;

	if( header->incl_len < packet->max_len )
	{ // within snaplen
		int index = header->incl_len / sizeof(*(packet->data));
		memcpy(&packet->data[index], data, size);

		header->incl_len += size;
	}
}

int pcap_packet_write( pcap_t* pcap, pcap_packet_t* packet )
{
	int error;

	error = frameh_write(packet->header, pcap->file);
	if( error ) return error;

	error = frame_data_write(packet->data, packet->header->incl_len, pcap->file);
	if( error ) return error;

	return 0;
}

int pcap_flush( pcap_t* pcap )
{
	return fflush(pcap->file);
}

void pcap_packet_free( pcap_packet_t* packet )
{
	assert(packet != NULL);

	if( packet->header != NULL )
	{
		free(packet->header);
	}

	free(packet->data);
}
