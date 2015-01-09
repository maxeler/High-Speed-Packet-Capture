/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

typedef uint64_t data_t;

int main( int argc, char** argv )
{
	int burst_size = 192;

	int bursts = 1;
	int words_per_burst = burst_size / sizeof(data_t);

	int data_len = bursts * words_per_burst;
	uint64_t data_in_len = data_len;

	// report info
	printf("words_per_burst = %d\n", words_per_burst);
	printf("data_len = %d\n", data_len);

	data_t* data_in = calloc(data_in_len, sizeof(*data_in));
	for( int i=0; i<data_in_len; i++ )
	{
		data_in[i] = i;
	}

	max_file_t *maxfile = PacketCapture_init();

	/* send */
	printf("Sending test data (%ld bytes).\n", (data_in_len * sizeof(data_t)));
	PacketCapture_capture(data_in_len, data_in);

	/* read */
	printf("Running read.\n");
	int data_out_len = data_in_len;
	data_t* data_out = calloc(data_out_len, sizeof(*data_out));
	PacketCapture_read(data_out_len, data_out);

	// check data
	for( int i=0; i<data_out_len; i++ )
	{
		printf("data_out[%d]: %"PRIu64"\n", i, data_out[i]);
		if( data_out[i] != data_in[i] )
		{
			printf("data_out[%d](%"PRIu64") != data_in[%d](%"PRIu64")\n", i, data_out[i], i, data_in[i]);
			return EXIT_FAILURE;
		}
	}

	printf("Passed.\n");

	return EXIT_SUCCESS;
}

