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

enum read_mode_e
{
	READ_MODE_META,
	READ_MODE_DATA,
};

int main( int argv, char* argc[] )
{
	(void) argc;
	(void) argv;

//	const char* DFE_ADDR = "5.5.5.2";
//	const int DFE_PORT = 2511;
	const char* LOCAL_ADDR = "5.5.5.1";
	const int LOCAL_PORT = 2511;
	const int NUM_CONECTIONS = 1;
	const int TCP_SIZE_MAX = 1500;
	const int META_SIZE = 1;
	const int DATA_SIZE_MAX = 8;

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

	uint8_t* buffer = malloc(TCP_SIZE_MAX);
	memset(buffer, 0, TCP_SIZE_MAX);

	int connection_fd = accept(sock_num, NULL, 0);
	printf("connection_fd= %d\n", connection_fd);

	ssize_t read_size = META_SIZE;
	enum read_mode_e read_mode = READ_MODE_META;
	while( 1 )
	{
		ssize_t nbytes = read(connection_fd, buffer, read_size);

		if( nbytes == -1 )
		{
			report_errno("Unable to receive data from socket", errno);
			return EXIT_FAILURE;
		}

		printf("read %ldB: 0x", nbytes);
		for( int i=0; (i * sizeof(*buffer))<nbytes; i++ )
		{
			printf(".%02"PRIx8, buffer[i]);
		}
		printf("\n");

		if( read_mode == READ_MODE_META )
		{
			// extract next read size
			int sof = (buffer[0] >> 0) & 0x1;
			int eof = (buffer[0] >> 1) & 0x1;
			int mod = (buffer[0] >> 2) & 0x3;

			printf("meta (sof=%d, eof=%d, mod=%d)\n", sof, eof, mod);

			read_mode = READ_MODE_DATA;
			read_size = (mod == 0) ? DATA_SIZE_MAX : mod;
		}
		else
		{
			printf("data\n");
			read_mode = READ_MODE_META;
			read_size = META_SIZE;
		}

		memset(buffer, 0, TCP_SIZE_MAX);
	}

	return 0;
}
