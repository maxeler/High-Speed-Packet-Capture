/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"


int main( int arc, char** argv )
{
	int data_len = 192;
	uint32_t* data_in = calloc(data_len, sizeof(*data_in));
	uint32_t* data_out = calloc(data_len, sizeof(*data_out));

	for( int i=0; i<data_len; i++ )
	{
		data_in[i] = i;
	}

	max_file_t *maxfile = PacketCapture_init();
	int burst_size = max_get_burst_size(maxfile, "cmd_toMem");
	int mem_size_bursts = (data_len * sizeof(*data_in)) / burst_size;
	uint64_t tail_addr = 0;

	printf("burstSize: %d\n", burst_size);
	printf("burstCount: %d\n", (data_len * sizeof(*data_in))/burst_size);

	printf("Running capture.\n");
	PacketCapture_capture(burst_size, data_len, mem_size_bursts, tail_addr, data_in);


	printf("Running read.\n");
	PacketCapture_read(data_len, data_out);

	for( int i=0; i<data_len; i++ )
	{
		if( data_out[i] != data_in[i] )
		{
			printf("data_out[%d](%"PRIx32") != data_in[%d](%"PRIx32")\n", i, data_out[i], i, data_in[i]);
			return EXIT_FAILURE;
		}
	}

	printf("Check passed.\n");

	return EXIT_SUCCESS;
}
