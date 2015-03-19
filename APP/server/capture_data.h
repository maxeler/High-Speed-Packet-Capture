/*
 * capture_data.h
 */

#ifndef CAPTURE_DATA_H_
#define CAPTURE_DATA_H_

#include "timestamp.h"
#include "frame.h"


typedef struct capture_data_s
{
	timestamp_t* timestamp;
	frame_t* frame;
} capture_data_t;

#endif /* CAPTURE_DATA_H_ */

