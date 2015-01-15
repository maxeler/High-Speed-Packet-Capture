/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

static int CHUNK_SIZE = 16;
static int BURST_SIZE = 192;
typedef struct
{
	int sof;
	int eof;
	int mod;
	uint64_t data;
} frame_t;
static uint64_t FRAME_DATA_MASK = 0x1F;

// put 1 in bit range from from to from + width.
static uint64_t make_mask( int from, int width )
{
	assert(from + width < sizeof(uint64_t));

	static uint64_t ONES = 0xFFFFFFFFFFFFFFFF;
	int total_bits = sizeof(uint64_t) * 8;

	return (ONES << (total_bits - width)) >> (total_bits - (from + width));
}

int main( int argc, char** argv )
{

//	max_file_t *maxfile = PacketCapture_init();

	int data_bursts = 1;
	int data_size = data_bursts * BURST_SIZE;
	int frames_len = data_size / CHUNK_SIZE;

//	/* capture */
//	// create data
//	int frame_size = (2 * sizeof(uint64_t));
//	int frames_size = frames_len * frame_size;
//
//	uint64_t* frames = malloc(frames_size);
//	for( int i=0; i<frames_len; i++ )
//	{
//		// set frame data
//		int frame_index = (2 * i);
//		// frame[64:70]
//		frames[frame_index] = 0x6;
//		// frame[0:64]
//		frames[frame_index + 1] = i;
//		printf("frames[%2d] = 0x%016"PRIx64".%016"PRIx64"\n", frame_index, frames[frame_index], frames[frame_index + 1]);
//	}
//
//	// send
//	printf("Sending %d frames (%dB).\n", frames_len, frames_size);
//	PacketCapture_capture(frames_size, frames);

	/* read */
	while( 1 )
	{
		printf("Running read.\n");


		printf("Reading %d frames (%dB)\n", frames_len, data_size);
		uint64_t* data = malloc(data_size);
		PacketCapture_read(data_bursts, data);
		printf("Done reading.\n");

		int data_len = data_size / sizeof(*data);
		printf("data_len: %d\n", data_len);
		printf("data = 0x");
		for( int i=0; i<data_len; i++ )
		{
			printf("%016"PRIx64, data[i]);
		}
		printf("\n");

		frame_t* frame = malloc(sizeof(*frame));
		for( int i=0; i<frames_len; i++ )
		{
			printf("[frame = %d]\n", i);

			int frame_index = (2 * i);
			uint64_t* chunk = &data[frame_index];
			printf("data = 0x%016"PRIx64".%016"PRIx64"\n", chunk[1], chunk[0]);


			// print raw frame info
			printf("frame[%d][0]: 0x%"PRIx64"\n", i, chunk[0]);
			printf("frame[%d][1]: 0x%"PRIx64"\n", i, chunk[1]);
			// print frame info
			printf("frame[%d].data: 0x%"PRIx64"\n", i, chunk[0]);
			printf("frame[%d].eof: %"PRIu64"\n", i, (chunk[1] >> 0) & 0x1);
			printf("frame[%d].sof: %"PRIu64"\n", i, (chunk[1] >> 1) & 0x1);
			printf("frame[%d].mod: %"PRIu64"\n", i, (chunk[1] >> 2) & 0x7);
//
			printf("\n");

		}
		return 0;
	}

	return EXIT_SUCCESS;
}
