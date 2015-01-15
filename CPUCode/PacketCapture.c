/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

static int FRAME_SIZE_BITS = 70;
static int BURST_SIZE = 192;
typedef struct __packed__
{
	uint64_t upper;
	uint64_t lower;
} frame_data_t;
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
	int frames_len = (data_size * 8) / FRAME_SIZE_BITS;

	/* capture */
	// create data
	int frame_size = (2 * sizeof(uint64_t));
	int frames_size = frames_len * frame_size;

	uint64_t* frames = malloc(frames_size);
	for( int i=0; i<frames_len; i++ )
	{
		int frame_index = (2 * i);
		frames[frame_index] = i%2 ==0? 0 : 0xFFFFFFFFFFFFFFFF; // lower word
		frames[frame_index + 1] = i%2 ==0? 0 : 0xFFFFFFFFFFFFFFFF; // upper word
		printf("frames[%d] = %"PRIu64"%"PRIu64"\n", frame_index, frames[frame_index + 1], frames[frame_index]);
	}

	// send
	printf("Sending %d frames (%dB).\n", frames_len, frames_size);
	PacketCapture_capture(frames_size, frames);

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

		// shift by frame size until frame count reached
		int prev_upper_chunk_bits = 0;
		int word_bits = 64;
		int chunk_bits = 70;

		for( int i=0; i<frames_len; i++ )
		{
			printf("[frame = %d]\n", i);
			printf("prev_upper_chunk_bits = %d\n", prev_upper_chunk_bits);

			uint64_t* datum = &data[i];



			printf("datum[0]: 0x%"PRIx64"\n", datum[0]);
			printf("datum[1]: 0x%"PRIx64"\n", datum[1]);

			int lower_chunk_bits = (word_bits - prev_upper_chunk_bits);
			int upper_chunk_bits = (chunk_bits - lower_chunk_bits);
			printf("lower_chunk_bits = %d\n", lower_chunk_bits);
			printf("upper_chunk_bits = %d\n", upper_chunk_bits);

			// upper_chunk = datum[1][-upper_chunk_bits:]
			uint64_t upper_chunk = datum[1] << (word_bits - upper_chunk_bits);
			// lower_chunk = datum[0][:lower_chunk_bits]
			uint64_t lower_chunk = datum[0] >> (word_bits - lower_chunk_bits);
			// lower = upper_chunk[(word_bits - lower_chunk_bits):] # lower_chunk
			uint64_t lower = (upper_chunk << (upper_chunk_bits - (word_bits - lower_chunk_bits))) | lower_chunk;
			// upper = upper_chunk[:-(word_bits - lower_chunk_bits)]
			uint64_t upper = upper_chunk >> ((word_bits - upper_chunk_bits) + (word_bits - lower_chunk_bits));

			prev_upper_chunk_bits = upper_chunk_bits;

			printf("upper_chunk: 0x%"PRIx64"\n", upper_chunk >> (word_bits - upper_chunk_bits));
			printf("lower_chunk: 0x%"PRIx64"\n", lower_chunk);
			printf("upper: 0x%"PRIx64"\n", upper);
			printf("lower: 0x%"PRIx64"\n", lower);
//
//			uint64_t frame_data[2] = {upper, lower};
//
//			// print raw frame info
//			printf("frame[%d][0]: 0x%"PRIx64"\n", i, frame_data[0]);
//			printf("frame[%d][1]: 0x%"PRIx64"\n", i, frame_data[1]);
//			// print frame info
//			printf("frame[%d].data: 0x%"PRIx64"\n", i, frame_data[0]);
//			printf("frame[%d].eof: %"PRIu64"\n", i, (frame_data[1] >> 0) & 0x1);
//			printf("frame[%d].sof: %"PRIu64"\n", i, (frame_data[1] >> 1) & 0x1);
//			printf("frame[%d].mod: %"PRIu64"\n", i, (frame_data[1] >> 2) & 0x7);
//
			printf("\n");

		}
		return 0;
	}

	return EXIT_SUCCESS;
}
