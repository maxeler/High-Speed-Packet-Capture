/*
 * capture_server.c
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
#include <pthread.h>

#include "pcap.h"
#include "log.h"
#include "capture_data.h"
#include "args.h"
#include "stats.h"


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
static const int NUM_CONNECTIONS = 1;
static const int HEADER_SIZE = 1;
static const int DATA_TIMESTAMP_SIZE = 8;
static const int DATA_FRAME_SIZE_MAX = 8;
#define DATA_SIZE_MAX (DATA_TIMESTAMP_SIZE + DATA_FRAME_SIZE_MAX)

volatile sig_atomic_t g_shutdown = 0;

static void handle_exit( int sig );

static void report_errno( const char* msg, int errnum );

static void parse_header( capture_data_t* data, uint64_t* buffer );

static void parse_data( capture_data_t* data, uint64_t* buffer );

static int handle_client( pcap_t* pcap, int conh );

static int read_data( int fd, void* buffer, size_t read_size );

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

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
		.sin_addr = arguments.server_ip,
	};

	error = bind(sock_num, (const struct sockaddr*) &addr, sizeof(addr));
	if( error )
	{
		report_errno("Unable to bind socket", errno);
		return EXIT_FAILURE;
	}

	error = listen(sock_num, NUM_CONNECTIONS);
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
		int fd = accept(sock_num, (struct sockaddr*) &client_addr, (socklen_t *) &client_addr_size);
		if( fd == -1 )
		{ // error
			report_errno("Unable to accept on socket", errno);
		}
		else // fd != -1
		{ // new connection
			char client_ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
			const int client_port = ntohs(client_addr.sin_port);
			logf_info("Client connected from %s: %d.\n", client_ip, client_port);

			int error = handle_client(pcap, fd);
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

static int handle_client( pcap_t* pcap, int fd )
{
	sstats_t* sstats = sstats_init();
	assert(sstats != NULL);

	// register signal handler
	struct sigaction exit_action;
	exit_action.sa_handler = handle_exit;
	exit_action.sa_flags = 0;
	sigemptyset(&exit_action.sa_mask);
	sigaction(SIGINT, &exit_action, NULL);
	sigaction(SIGTERM, &exit_action, NULL);

	// start stats reporting thread
	pthread_t* thread = NULL;
	if( log_level_active(LOG_LEVEL_INFO) )
	{
		thread = malloc(sizeof(*thread));
		assert(thread != NULL);

		int error = pthread_create(thread, NULL, report_stats, (void*) sstats);
		if( error )
		{
			fprintf(stderr, "Error: Unable to create thread\n");
			return 1;
		}
	}

	// service socket
	pcap_packet_t* packet = NULL;
	int sof_expected = 1;
	const int buffer_size = fmax(HEADER_SIZE, DATA_SIZE_MAX);
	uint64_t buffer[buffer_size];

	while( 1 )
	{
		int error;

		frame_t frame;
		timestamp_t timestamp;
		capture_data_t capture_data = {
			.frame = &frame,
			.timestamp = &timestamp,
		};

		// read header
		memset(buffer, 0, sizeof(buffer));
		error = read_data(fd, buffer, HEADER_SIZE);
		if( error == 2 )
		{ // shutting down
			break;
		}
		else if( error )
		{
			report_errno("Unable to header from socket", errno);
			return 1;
		}

		log_trace("header ");
		log_binary_append(LOG_LEVEL_TRACE, buffer, HEADER_SIZE);

		parse_header(&capture_data, buffer);

		// read data
		size_t data_size = DATA_TIMESTAMP_SIZE + frame_get_size(capture_data.frame);
		memset(buffer, 0, sizeof(buffer));
		error = read_data(fd, buffer, data_size);
		if( error == 2 )
		{ // shutting down
			break;
		}
		else if( error )
		{
			report_errno("Unable to read data from socket", errno);
			return 1;
		}

		log_trace("data ");
		log_binary_append(LOG_LEVEL_TRACE, buffer, data_size);

		parse_data(&capture_data, buffer);

		// process capture data
		// sof
		assert(sof_expected == frame.sof);
		if( frame.sof )
		{
			uint64_t seconds;
			uint64_t nanoseconds;

			if( timestamp_is_valid(&timestamp) )
			{
				seconds = timestamp_get_seconds(&timestamp);
				nanoseconds = timestamp_get_nanoseconds(&timestamp);
			}
			else // !timestamp_is_valid(timestamp)
			{
				logf_info("WARNING: Using system time due to invalid dfe timestamp (doubt=%d, valid=%d, value=%"PRIu64")\n",
						  timestamp_get_doubt(&timestamp),
						  timestamp_is_valid(&timestamp),
						  timestamp_get_value(&timestamp));

				seconds = time(NULL);
				nanoseconds = 0;
			}

			packet = pcap_packet_init(pcap, seconds, nanoseconds);
			assert(packet != NULL);

			sof_expected = 0;
		}

		// append
		const uint64_t* data = frame_get_data(&frame);
		int size = frame_get_size(&frame);
		pcap_packet_append(packet, data, size);

		// eof
		if( frame.eof )
		{
			pcap_packet_write(pcap, packet);
			pcap_flush(pcap);

			pcap_packet_free(packet);
			packet = NULL;

			sof_expected = 1;
		}

		// update stats
		stats_t stats = STATS_RESET;
		stats.frames = 1;
		stats.bytes = size;
		stats.packets = (frame.eof == 1) ? 1 : 0;

		sstats_inc(sstats, &stats);
		sstats_try_update(sstats);
	}

	// cleanup
	pcap_flush(pcap);

	if( packet != NULL )
	{
		pcap_packet_free(packet);
		packet = NULL;
	}

	sstats_free(sstats);
	sstats = NULL;

	return 0;
}

static int read_data( int fd, void* buffer, size_t read_size )
{
	assert(buffer != NULL);

	uint8_t* buffer8 = (uint8_t*) buffer;
	size_t nbytes = 0;

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

void parse_header( capture_data_t* data, uint64_t* buffer )
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

void parse_data( capture_data_t* data, uint64_t* buffer )
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
