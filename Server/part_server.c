/*
 * part_server.c
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

static void report_errno( const char* msg, int errnum )
{
	fprintf(stderr, "%s: %s (errno=%d)\n", msg, strerror(errnum), errnum);
}


int main( int argv, char* argc[] )
{
	(void) argc;
	(void) argv;

//	const char* DFE_ADDR = "5.5.5.2";
//	const int DFE_PORT = 2511;
	const char* LOCAL_ADDR = "5.5.5.1";
	const int LOCAL_PORT = 2511;
	const int NUM_CONECTIONS = 1;

	int error;

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

	uint64_t buffer[2];

	int connection_fd = accept(sock_num, NULL, 0);
	printf("connection_fd= %d\n", connection_fd);

	while( 1 )
	{
		ssize_t nbytes = read(connection_fd, buffer, sizeof(buffer));

		if( nbytes == -1 )
		{
			report_errno("Unable to receive data from socket", errno);
			return EXIT_FAILURE;
		}

		if( nbytes != sizeof(buffer) )
		{
			fprintf(stderr, "Unexpected data\n");
			return EXIT_FAILURE;
		}

		uint8_t eof = (buffer[1] >> 0) & 0x1;
		uint8_t sof = (buffer[1] >> 1) & 0x1;
		uint8_t mod = (buffer[1] >> 2) & 0x7;
		uint64_t data = buffer[0];

		printf("read %ldB: 0x%016"PRIx64".%016"PRIx64"\n", nbytes, buffer[1], buffer[0]);
		printf("[sof=%"PRIu8", eof=%"PRIu8", mod=%"PRIu8", data=0x%016"PRIx64"]\n", sof, eof, mod, data);

		buffer[0] = 0;
		buffer[1] = 0;
	}

	return 0;
}
