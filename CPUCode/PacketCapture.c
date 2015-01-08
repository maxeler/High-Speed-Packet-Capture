/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

int main( int argc, char** argv )
{
	uint64_t data_in_len = 192;

	uint32_t* data_in = calloc(data_in_len, sizeof(*data_in));
	for( int i=0; i<data_in_len; i++ )
	{
		data_in[i] = i;
	}

	max_file_t *maxfile = PacketCapture_init();

	/* send */
	int data_in_size = data_in_len * sizeof(*data_in);
	printf("Sending test data (%d bytes).\n", data_in_size);
	PacketCapture_capture(data_in_len, data_in);

	/* read */
	printf("Running read.\n");
	int data_out_len = data_in_len;
	uint32_t* data_out = calloc(data_out_len, sizeof(*data_out));
	PacketCapture_read(data_out_len, data_out);

	// check data
	for( int i=0; i<data_out_len; i++ )
	{
		printf("data_out[%d]: %"PRIu32"\n", i, data_out[i]);
		if( data_out[i] != data_in[i] )
		{
			printf("data_out[%d](%"PRIx32") != data_in[%d](%"PRIx32")\n", i, data_out[i], i, data_in[i]);
			return EXIT_FAILURE;
		}
	}

	printf("Passed.\n");

	return EXIT_SUCCESS;
}

