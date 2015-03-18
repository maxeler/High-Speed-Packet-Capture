/*
 * args.h
 */

#include <stdlib.h>
#include <argp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef ARGS_H_
#define ARGS_H_

#define IPS_LEN_MAX 3

typedef enum
{
	SERVER_TYPE_A,
	SERVER_TYPE_B,
	SERVER_TYPE_UNDEF
} server_type_e;

typedef struct
{
	server_type_e type;
	struct in_addr ip;
} remote_conf_t;

typedef struct
{
	int local_enabled;
	int remote_enabled;

	int log_level;
	struct in_addr dfe_ip;
	struct in_addr dfe_netmask;
	char* local_file;

	int ipsA_len;
	int ipsB_len;
	struct in_addr ipsA[IPS_LEN_MAX];
	struct in_addr ipsB[IPS_LEN_MAX];

} arguments_t;

extern struct argp argp;

#endif /* ARGS_H_ */
