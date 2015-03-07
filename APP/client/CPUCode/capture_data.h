/*
 * capture_data.h
 */

#ifndef CAPTURE_DATA_H_
#define CAPTURE_DATA_H_

#include "timestamp.h"
#include "frame.h"


typedef struct capture_data_s capture_data_t;

capture_data_t* capture_data_parse( uint64_t data[4] );

void capture_data_free( capture_data_t* this );

timestamp_t* capture_data_get_timestamp( capture_data_t* this );

frame_t* capture_data_get_frame( capture_data_t* this );

#endif /* CAPTURE_DATA_H_ */

