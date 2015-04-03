/*
 * args.h
 */

#ifndef ARGS_H_
#define ARGS_H_

#include <argp.h>

typedef struct
{
	int log_level;
	char* ifname;
	char* file;

} arguments_t;

extern struct argp argp;

#endif /* ARGS_H_ */
