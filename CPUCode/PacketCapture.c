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
	int data_len = 64;
	uint32_t* data_in = calloc(data_len, sizeof(*data_in));
	uint32_t* data_out = calloc(data_len, sizeof(*data_in));

	for( int i=0; i<data_len; i++ )
	{
		data_in[i] = i;
		data_out[i] = 0;
	}

	PacketCapture(data_len, data_in, data_out);

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
