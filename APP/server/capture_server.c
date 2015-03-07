/*
 * capture_server.c
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
#include <stdarg.h>
#include <math.h>

#include "pcap.h"
#include "log.h"
#include "capture_data.h"


static const int NUM_CONECTIONS = 1;
static const int HEADER_SIZE = 1;
static const int DATA_TIMESTAMP_SIZE = 8;
static const int DATA_FRAME_SIZE_MAX = 8;
#define DATA_SIZE_MAX (DATA_TIMESTAMP_SIZE + DATA_FRAME_SIZE_MAX)

static void report_errno( const char* msg, int errnum )
{
	fprintf(stderr, "%s: %s (errno=%d)\n", msg, strerror(errnum), errnum);
}

enum read_mode_e
{
	READ_MODE_HEADER,
	READ_MODE_DATA,
};

void parse_header( capture_data_t* data, uint64_t buffer[1] );

void parse_data( capture_data_t* data, uint64_t buffer[1] );

void process_capture_data( pcap_t* pcap, capture_data_t* data );

static int handle_client( pcap_t* pcap, int conh );

int main( int argc, char* argv[] )
{
	int error;

	// parse args
	// TODO: parse safely/properly
	if( argc != 4 )
	{
		fprintf(stderr, "%s: ip port file.pcap", argv[0]);
		return EXIT_FAILURE;
	}

	const char* ip = argv[1];
	const int port = strtol(argv[2], NULL, 10);
	const char* filePath = argv[3];

	g_log_ip = ip;
	g_log_level = 3;

	// init pcap
	FILE* file = fopen(filePath, "w+");
	pcap_t* pcap = pcap_create(file, 0, 1, 0, 65535);
	assert(pcap != NULL);

	// setup socket
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
	addr.sin_port = htons(port);
	int status = inet_aton(ip, &addr.sin_addr);
	if( status == 0 )
	{
		fprintf(stderr, "inet_aton: invalid address (%s)\n", ip);
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
		report_errno("Unable to listen on socket", errno);
		return EXIT_FAILURE;
	}

	// accept a client
	while( 1 )
	{
		log_info("Waiting for client...\n");

		struct sockaddr_in client_addr = {0};
		size_t client_addr_size = sizeof(client_addr);
		int con_fd = accept(sock_num, (struct sockaddr*) &client_addr, (socklen_t *) &client_addr_size);

		if( con_fd == -1 )
		{ // error
			report_errno("Unable to accept on socket", errno);
		}
		else
		{ // new connection
			char client_ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
			const int client_port = ntohs(client_addr.sin_port);
			logf_info("Client connected %s:%d.\n", client_ip, client_port);

			int error = handle_client(pcap, con_fd);
			if( error )
			{
				fprintf(stderr, "Error handling client\n");
			}
		}
	}

	// TODO: cleanup
}

static int handle_client( pcap_t* pcap, int con_fd )
{
	int buffer_size = fmax(HEADER_SIZE, DATA_SIZE_MAX);
	uint64_t* buffer = malloc(buffer_size);

	size_t total_packets = 0; // note: may overflow
	size_t read_size = HEADER_SIZE;
	enum read_mode_e read_mode = READ_MODE_HEADER;
	frame_t frame;
	timestamp_t timestamp;
	capture_data_t capture_data = {
		.frame = &frame,
		.timestamp = &timestamp,
	};

	// TODO: keep alive

	while( 1 )
	{
		// read until expected bytes received
		memset(buffer, 0, buffer_size);
		ssize_t nbytes = 0;
		while( nbytes < read_size )
		{
			nbytes += read(con_fd, (buffer + nbytes), (read_size - nbytes));

			if( nbytes == -1 )
			{
				report_errno("Unable to receive data from socket", errno);
				return 1;
			}
		}

		// debug print data
		if( g_log_level >= 2 )
		{
			uint8_t* buffer8 = (uint8_t*) buffer;
			char str[BUFSIZ];
			int str_len = 0;
			str_len += snprintf((str + str_len), BUFSIZ, "read %ldB: ", nbytes);
			for( int i=0; (i * sizeof(*buffer8))<nbytes; i++ )
			{
				if( i != 0 )
				{
					str_len += snprintf((str + str_len), (BUFSIZ - str_len), ".");
				}
				str_len += snprintf((str + str_len), (BUFSIZ - str_len), "%02"PRIx8, buffer8[i]);
			}
			snprintf((str + str_len), (BUFSIZ - str_len), "\n");
			logf_debug("%s", str);
		}

		// process data
		switch( read_mode )
		{
			case READ_MODE_HEADER:
			{
				// clear
				*(capture_data.timestamp) = TIMESTAMP_EMPTY;
				*(capture_data.frame) = FRAME_EMPTY;

				parse_header(&capture_data, buffer);

				// config next mode
				read_mode = READ_MODE_DATA;
				read_size = DATA_TIMESTAMP_SIZE + frame_get_size(capture_data.frame);

				// packet count
				if( frame.eof )
				{
					total_packets++;
					logf_info("Total packets: %ld\n", total_packets);
				}

				break;
			}
			case READ_MODE_DATA:
			{
				parse_data(&capture_data, buffer);
				process_capture_data(pcap, &capture_data);

				// config next mode
				read_mode = READ_MODE_HEADER;
				read_size = HEADER_SIZE;

				break;
			}
		}
	}

	return 0;
}

void parse_header( capture_data_t* data, uint64_t buffer[1] )
{
	assert(data != NULL);
	assert(buffer != NULL);

	timestamp_t* timestamp = data->timestamp;
	frame_t* frame = data->frame;

	timestamp->doubt = (buffer[0] >> 0) & 0x1;
	timestamp->valid = (buffer[0] >> 1) & 0x1;
	frame->sof = (buffer[0] >> 2) & 0x1;
	frame->eof = (buffer[0] >> 3) & 0x1;
	frame->mod = (buffer[0] >> 4) & 0x7;

	logf_debug("header (timestamp(doubt=%d, valid=%d), frame(sof=%d, eof=%d, mod=%d))\n",
			    timestamp->doubt,
			    timestamp->valid,
				frame->sof,
				frame->eof,
				frame->mod);
}

void parse_data( capture_data_t* data, uint64_t buffer[1] )
{
	assert(data != NULL);
	assert(buffer != NULL);

	timestamp_t* timestamp = data->timestamp;
	frame_t* frame = data->frame;

	timestamp->value = buffer[0];
	frame->data = buffer[1];

	logf_debug("data (timestamp(value=%"PRIu64", frame(data=0x%"PRIx64"))\n",
				timestamp->value,
				frame->data);
}

void process_capture_data( pcap_t* pcap, capture_data_t* data )
{
	assert(pcap != NULL);
	assert(data != NULL);

	static pcap_packet_t* packet = NULL;
	static int sof_expected = 1;

	frame_t* frame = data->frame;
	timestamp_t* timestamp = data->timestamp;

	// sof
	assert(sof_expected == frame->sof);
	if( frame->sof )
	{
		packet = pcap_packet_init(pcap, timestamp_get_seconds(timestamp), timestamp_get_nanoseconds(timestamp));
		assert(packet != NULL);

		sof_expected = 0;
	}

	// append
	pcap_packet_append(packet, frame_get_data(frame), frame_get_size(frame));

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
