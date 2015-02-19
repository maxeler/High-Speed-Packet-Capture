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

#include "pcap.h"
#include "log.h"

const int NUM_CONECTIONS = 1;
const int HEADER_SIZE = 1;
const int DATA_SIZE_MAX = 8;
const int READ_SIZE_MAX = 8;

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

void process_frame( pcap_t* pcap, frame_t* frame );

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
	g_log_level = 1;

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
	uint64_t* buffer = malloc(READ_SIZE_MAX);
	memset(buffer, 0, READ_SIZE_MAX);

	size_t total_packets = 0; // note: may overflow
	size_t read_size = HEADER_SIZE;
	enum read_mode_e read_mode = READ_MODE_HEADER;
	frame_t frame;

	// TODO: keep alive

	while( 1 )
	{
		// read until expected bytes received
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
				// parse
				int sof = (buffer[0] >> 0) & 0x1;
				int eof = (buffer[0] >> 1) & 0x1;
				int mod = (buffer[0] >> 2) & 0x7;
				int size = (mod == 0) ? DATA_SIZE_MAX : mod;

				// build frame
				frame.eof = eof;
				frame.sof = sof;
				frame.size = size;

				logf_debug("header (sof=%d, eof=%d, mod=%d)\n", sof, eof, mod);

				// config next mode
				read_mode = READ_MODE_DATA;
				read_size = size;

				// packet count
				if( eof )
				{
					total_packets++;
					logf_info("Total packet(s): %ld\n", total_packets);
				}

				break;
			}
			case READ_MODE_DATA:
			{
				logf_debug("data (%ldB)\n", frame.size);

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
		packet = pcap_packet_init(pcap, time(NULL), 0);
		assert(packet != NULL);

		sof_expected = 0;
	}

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
