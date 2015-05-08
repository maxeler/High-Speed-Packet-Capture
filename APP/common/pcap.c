/*
 * pcap.c
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
	assert(header != NULL);
	assert(file != NULL);

	fwrite(header, sizeof(*header), 1, file);

	return 0;
}

static int frameh_write( packeth_t* header, FILE* file )
{
	assert(header != NULL);
	assert(file != NULL);

	fwrite(header, sizeof(*header), 1, file);

	return 0;
}

static int frame_data_write( const uint64_t* data, ssize_t size, FILE* file )
{
	assert(data != NULL);
	assert(file != NULL);

	fwrite(data, size, 1, file);

	return 0;
}

pcap_t* pcap_init( FILE* file, int32_t  thiszone, uint32_t network, uint32_t sigfigs, uint32_t snaplen )
{
	assert(file != NULL);

	pcap_t* this = malloc(sizeof(*this));
	if( this == NULL )
	{
		return NULL;
	}

	this->header = malloc(sizeof(*this->header));
	if( this->header == NULL )
	{
		pcap_free(this);
		this = NULL;

		return NULL;
	}

	// init this
	this->file = file;

	// init header
	globalh_t* header = this->header;
	header->version_major = VERSION_MAJOR;
	header->version_minor = VERSION_MINOR;
	header->magic_number = MAGIC_NUMBER;
	header->thiszone = thiszone;
	header->network = network;
	header->sigfigs = sigfigs;
	header->snaplen = snaplen;

	// write global header
	int error = globalh_write(header, file);
	if( error )
	{
		pcap_free(this);
		this = NULL;

		return NULL;
	}

	return this;
}

pcap_packet_t* pcap_packet_init( pcap_t* pcap, uint32_t ts_sec, uint32_t ts_nsec )
{
	assert(pcap != NULL);

	pcap_packet_t* this = malloc(sizeof(*this));
	if( this == NULL )
	{
		return NULL;
	}

	this->header = malloc(sizeof(*this->header));
	if( this->header == NULL )
	{
		pcap_packet_free(this);
		this = NULL;

		return NULL;
	}

	// init this
	this->max_len = pcap->header->snaplen;
	this->finalized = 0;

	this->data = malloc(this->max_len);
	if( this->data == NULL )
	{
		pcap_packet_free(this);
		this = NULL;

		return NULL;
	}

	// init header
	packeth_t* header = this->header;
	header->ts_sec = ts_sec;
	header->ts_nsec = ts_nsec;
	header->orig_len = 0;
	header->incl_len = 0;

	return this;
}

void pcap_packet_append( pcap_packet_t* this, const uint64_t* data, uint32_t size )
{
	assert(this != NULL);
	assert(data != NULL);
	assert(!this->finalized);

	if( size != sizeof(*data))
	{
		this->finalized = 1;
	}

	packeth_t* header = this->header;
	header->orig_len += size;

	if( header->incl_len < this->max_len )
	{ // within snaplen
		int index = header->incl_len / sizeof(*this->data);
		memcpy(&this->data[index], data, size);

		header->incl_len += size;
	}
}

int pcap_packet_write( pcap_t* this, pcap_packet_t* packet )
{
	assert(this != NULL);
	assert(packet != NULL);

	int error;

	error = frameh_write(packet->header, this->file);
	if( error )
	{
		return error;
	}

	error = frame_data_write(packet->data, packet->header->incl_len, this->file);
	if( error )
	{
		return error;
	}

	return 0;
}

int pcap_flush( pcap_t* this )
{
	assert(this != NULL);

	return fflush(this->file);
}

void pcap_packet_free( pcap_packet_t* this )
{
	assert(this != NULL);

	if( this->header != NULL )
	{
		free(this->header);
		this->header = NULL;
	}

	if( this->data != NULL )
	{
		free(this->data);
		this->data = NULL;
	}

	free(this);
	this = NULL;
}

void pcap_free( pcap_t* this )
{
	assert(this != NULL);

	if( this->header != NULL )
	{
		free(this->header);
		this->header = NULL;
	}

	free(this);
	this = NULL;
}
