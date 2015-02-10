/*
 * capture_client.c
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <time.h>

#include "pcap.h"

static void report_errno( const char* msg, int errnum )
{
	fprintf(stderr, "%s: %s (errno=%d)\n", msg, strerror(errnum), errnum);
}

enum read_mode_e
{
	READ_MODE_HEADER,
	READ_MODE_DATA,
};

typedef struct
{
	uint64_t* data;
	ssize_t size;
	int sof;
	int eof;
} frame_t;

void process_frame( pcap_t* pcap, frame_t* frame )
{
	assert(pcap != NULL);
	assert(frame != NULL);

	static pcap_packet_t* packet = NULL;
	static int sof_expected = 1;

	// sof
	assert(sof_expected == frame->sof);
	if( frame->sof )
	{
		sof_expected = 0;

		packet = pcap_packet_init(pcap, time(NULL), 0);
		assert(packet != NULL);
	}

	printf("frame->sof: %d\n", frame->sof);
	printf("packet: %p\n", (void*) packet);

	// append
	pcap_packet_append(packet, frame->data, frame->size);

	// eof
	if( frame->eof )
	{
		pcap_packet_write(pcap, packet);
		pcap_flush(pcap);
		pcap_packet_free(packet);
		packet = NULL;

		sof_expected = 1;
	}
}

int main( int argv, char* argc[] )
{
	(void) argc;
	(void) argv;

	const char* LOCAL_ADDR = "5.5.5.1";
	const int LOCAL_PORT = 2511;
	const int NUM_CONECTIONS = 1;
	const int HEADER_SIZE = 1;
	const int DATA_SIZE_MAX = 8;
	const int READ_SIZE_MAX = 8;

	int error;

	// init pcap
	FILE* file = fopen("./capture.pcap", "w+");
	pcap_t* pcap = pcap_create(file, 0, 1, 0, 65535);
	assert(pcap != NULL);

	// create socket
	int sock_num = socket(AF_INET, SOCK_STREAM, 0);
	if( sock_num == -1 )
	{
		report_errno("Unable to create socket", errno);
		return EXIT_FAILURE;
	}

	int reuseaddr = 1;
	error = setsockopt(sock_num, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
	if( error )
	{
		report_errno("Unable to set socket options", errno);
		return EXIT_FAILURE;
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(LOCAL_PORT);
	int status = inet_aton(LOCAL_ADDR, &addr.sin_addr);
	if( status == 0 )
	{
		report_errno("inet_aton", errno);
		return EXIT_FAILURE;
	}


	error = bind(sock_num, (const struct sockaddr*) &addr, sizeof(addr));
	if( error )
	{
		report_errno("Unable to bind socket", errno);
		return EXIT_FAILURE;
	}


	error = listen(sock_num, NUM_CONECTIONS);
	if( error )
	{
		report_errno("Unable to bind socket", errno);
		return EXIT_FAILURE;
	}

	uint64_t* buffer = malloc(READ_SIZE_MAX);
	memset(buffer, 0, READ_SIZE_MAX);

	int connection_fd = accept(sock_num, NULL, 0);
	printf("connection_fd= %d\n", connection_fd);

	ssize_t read_size = HEADER_SIZE;
	enum read_mode_e read_mode = READ_MODE_HEADER;
	frame_t frame;
	while( 1 )
	{
		ssize_t nbytes = read(connection_fd, buffer, read_size);

		if( nbytes == -1 )
		{
			report_errno("Unable to receive data from socket", errno);
			return EXIT_FAILURE;
		}

		uint8_t* buffer8 = (uint8_t*) buffer;
		printf("read %ldB: 0x", nbytes);
		for( int i=0; (i * sizeof(*buffer8))<nbytes; i++ )
		{
			printf(".%02"PRIx8, buffer8[i]);
		}
		printf("\n");

		switch( read_mode )
		{
			case READ_MODE_HEADER:
			{
				// parse
				int sof = (buffer[0] >> 0) & 0x1;
				int eof = (buffer[0] >> 1) & 0x1;
				int mod = (buffer[0] >> 2) & 0x3;
				int size = (mod == 0) ? DATA_SIZE_MAX : mod;

				// build frame
				frame.eof = eof;
				frame.sof = sof;
				frame.size = size;

				printf("header (sof=%d, eof=%d, mod=%d)\n", sof, eof, mod);

				// config next mode
				read_mode = READ_MODE_DATA;
				read_size = size;

				break;
			}
			case READ_MODE_DATA:
			{
				printf("data\n");
				// build frame
				frame.data = buffer;

				// process frame
				process_frame(pcap, &frame);

				// config next mode
				read_mode = READ_MODE_HEADER;
				read_size = HEADER_SIZE;
				memset(&frame, 0, sizeof(frame));

				break;
			}
		}
		memset(buffer, 0, (READ_SIZE_MAX * sizeof(*buffer)));
	}

	return 0;
}
