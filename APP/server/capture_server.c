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
#include <signal.h>
#include <sys/select.h>
#include <argp.h>

#include "pcap.h"
#include "log.h"
#include "capture_data.h"
#include "args.h"


enum read_mode_e
{
	READ_MODE_HEADER,
	READ_MODE_DATA,
};


static int PCAP_TZONE = PCAP_TZONE_UTC;
static int PCAP_SIGFIGS = 0;
static int PCAP_NETWORK = PCAP_NETWORK_ETHERNET;
static int PCAP_SNAPLEN = 65535;

static const int PORT = 2511;
static const int NUM_CONECTIONS = 1;
static const int HEADER_SIZE = 1;
static const int DATA_TIMESTAMP_SIZE = 8;
static const int DATA_FRAME_SIZE_MAX = 8;
#define DATA_SIZE_MAX (DATA_TIMESTAMP_SIZE + DATA_FRAME_SIZE_MAX)

volatile sig_atomic_t g_shutdown = 0;

void handle_exit( int sig );

static void report_errno( const char* msg, int errnum );

void parse_header( capture_data_t* data, uint64_t buffer[1] );

void parse_data( capture_data_t* data, uint64_t buffer[1] );

static int handle_client( pcap_t* pcap, int conh );

static int read_data( int fd, void* buffer, ssize_t read_size );

static void print_data( uint8_t* buffer, ssize_t buffer_size );

int main( int argc, char* argv[] )
{
	int error;

	arguments_t arguments;
	arguments.capture_file = NULL;
	arguments.log_level = LOG_LEVEL_INFO;
	inet_aton("0.0.0.0", &arguments.server_ip);

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	char* server_ip_str = strdup(inet_ntoa(arguments.server_ip));
	log_prepend_set(server_ip_str);
	log_level_set(arguments.log_level);

	// init pcap
	pcap_t* pcap = pcap_init(arguments.capture_file, PCAP_TZONE, PCAP_NETWORK, PCAP_SIGFIGS, PCAP_SNAPLEN);
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
	addr.sin_port = htons(PORT);
	addr.sin_addr = arguments.server_ip;

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
	while( !g_shutdown )
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
			logf_info("Client connected from %s: %d.\n", client_ip, client_port);

			int error = handle_client(pcap, con_fd);
			if( error )
			{
				fprintf(stderr, "Error handling client\n");
			}
		}
	}

	// cleanup
	pcap_flush(pcap);
	pcap_free(pcap);
	pcap = NULL;
	fclose(arguments.capture_file);
	arguments.capture_file = NULL;
	free(server_ip_str);
	server_ip_str = NULL;

	return EXIT_SUCCESS;
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

	// register signal handler
	struct sigaction exit_action;
	exit_action.sa_handler = handle_exit;
	exit_action.sa_flags = 0;
	sigemptyset(&exit_action.sa_mask);
	sigaction(SIGINT, &exit_action, NULL);
	sigaction(SIGTERM, &exit_action, NULL);

	// service socket
	pcap_packet_t* packet = NULL;
	static int sof_expected = 1;
	while( 1 )
	{
		memset(buffer, 0, buffer_size);

		int error = read_data(con_fd, buffer, read_size);
		if( error == 2 )
		{ // shutting down
			break;
		}
		else if( error )
		{
			report_errno("Unable to read data from socket", errno);
			return 1;
		}

		if( log_level_active(LOG_LEVEL_INFO) )
		{
			print_data((uint8_t*) buffer, read_size);
		}

		// process data
		switch( read_mode )
		{
			case READ_MODE_HEADER:
			{
				// reset
				*(capture_data.timestamp) = TIMESTAMP_EMPTY;
				*(capture_data.frame) = FRAME_EMPTY;

				// process
				parse_header(&capture_data, buffer);

				// config next mode
				read_mode = READ_MODE_DATA;
				read_size = DATA_TIMESTAMP_SIZE + frame_get_size(capture_data.frame);

				// report stats
				if( frame.eof )
				{
					total_packets++;
					logf_info("Total packets: %ld\n", total_packets);
				}

				break;
			}
			case READ_MODE_DATA:
			{
				// process
				parse_data(&capture_data, buffer);

				frame_t* frame = capture_data.frame;
				timestamp_t* timestamp = capture_data.timestamp;

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

				// config next mode
				read_mode = READ_MODE_HEADER;
				read_size = HEADER_SIZE;

				break;
			}
		}
	}

	// cleanup
	pcap_flush(pcap);
	if( packet != NULL )
	{
		pcap_packet_free(packet);
		packet = NULL;
	}
	packet = NULL;
	free(buffer);
	buffer = NULL;

	return 0;
}

static int read_data( int fd, void* buffer, ssize_t read_size )
{
	uint8_t* buffer8 = (uint8_t*) buffer;
	ssize_t nbytes = 0;
	while( (nbytes < read_size) && !g_shutdown )
	{
		// wait w/ timeout for data
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);

		struct timeval timeout = {
			.tv_sec = 1,
			.tv_usec = 0,
		};

		int status = select((fd + 1), &readfds, NULL, NULL, &timeout);

		if( status == -1 )
		{ // error
			if( errno == EINTR )
			{ // interrupted by signal
				// retry
				continue;
			}
			else
			{ // fatal
				report_errno("Select failed", errno);
				return 1;
			}
		}
		else if( status == 0 )
		{ // data not available
			// retry
			continue;
		}
		else // status == 1
		{ // read ready
			nbytes += read(fd, (buffer8 + nbytes), (read_size - nbytes));

			if( nbytes == -1 )
			{ // error
				report_errno("Unable to receive data from socket", errno);
				return 1;
			}
		}
	}

	if( g_shutdown )
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

static void print_data( uint8_t* buffer, ssize_t buffer_size )
{
	char str[BUFSIZ];
	int str_len = 0;
	str_len += snprintf((str + str_len), BUFSIZ, "read %ldB: ", buffer_size);
	for( int i=0; (i * sizeof(*buffer))<buffer_size; i++ )
	{
		if( i != 0 )
		{
			str_len += snprintf((str + str_len), (BUFSIZ - str_len), ".");
		}
		str_len += snprintf((str + str_len), (BUFSIZ - str_len), "%02"PRIx8, buffer[i]);
	}
	snprintf((str + str_len), (BUFSIZ - str_len), "\n");
	logf_debug("%s", str);
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

void handle_exit( int sig )
{
	(void) sig;

	g_shutdown = 1;

	// restore original handler
	struct sigaction sigint_act;
	sigint_act.sa_handler = SIG_DFL;
	sigint_act.sa_flags = 0;
	sigemptyset(&sigint_act.sa_mask);
	sigaction(SIGINT, &sigint_act, NULL);
	sigaction(SIGTERM, &sigint_act, NULL);

	printf("Shutting down...\n");
}

static void report_errno( const char* msg, int errnum )
{
	fprintf(stderr, "%s: %s (errno=%d)\n", msg, strerror(errnum), errnum);
}
