/*
 * args.h
 */

#ifndef ARGS_H_
#define ARGS_H_

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef struct
{
	int log_level;

	FILE* capture_file;
	struct in_addr server_ip;

} arguments_t;

extern struct argp argp;


#endif /* ARGS_H_ */
