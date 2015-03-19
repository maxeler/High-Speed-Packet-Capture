/*
 * capture_data.h
 */

#ifndef CAPTURE_DATA_H_
#define CAPTURE_DATA_H_

#include "timestamp.h"
#include "frame.h"


typedef struct capture_data_s capture_data_t;

/*
 * Creates capture data from CPU capture data bits.
 * Data must not be NULL.
 */
capture_data_t* capture_data_parse( uint64_t data[4] );

/*
 * Frees capture data.
 * Capture_data must not be NULL.
 */
void capture_data_free( capture_data_t* capture_data );

/*
 * Returns capture data time stamp.
 * Capture_data must not be NULL.
 */
timestamp_t* capture_data_get_timestamp( capture_data_t* capture_data );

/*
 * Returns capture data frame.
 * Capture_data must not be NULL.
 */
frame_t* capture_data_get_frame( capture_data_t* capture_data );

#endif /* CAPTURE_DATA_H_ */

