/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"

// todo: use mem_info
typedef struct mem_info
{
	uint64_t tail;
	uint64_t head;
	uint64_t max;
};

void increment_tail(  uint64_t size_max, uint64_t* tail, uint64_t amount )
{
	*tail = (*tail + amount) % size_max;
}

uint64_t get_unread_mem_len( uint64_t size_max, uint64_t tail, uint64_t head )
{
	if( head == tail )
	{
		return 0;
	}
	else if( head > tail )
	{
		return head - tail;
	}
	else // head < tail
	{
		return (size_max - tail) + head;
	}
}

int main( int arc, char** argv )
{
	int mem_len_max = 8;
	uint64_t data_in_len = 192;

	uint32_t* data_in = calloc(data_in_len, sizeof(*data_in));
	for( int i=0; i<data_in_len; i++ )
	{
		data_in[i] = i;
	}

	uint64_t tail_addr = 0;
	uint64_t head_addr;

	max_file_t *maxfile = PacketCapture_init();
	int burst_size = max_get_burst_size(maxfile, "cmd_toMem");
	int word_size = burst_size / sizeof(*data_in);

	/* send */
	int data_in_size = data_in_len * sizeof(*data_in);
	int data_in_burst_len = data_in_size / burst_size;

	printf("Sending test data (%d bursts).\n", data_in_burst_len);
	PacketCapture_capture(burst_size, data_in_len, mem_len_max, tail_addr, data_in);

	/* read */
	// get head
	PacketCapture_getHead(&head_addr);
	printf("head=%" PRIu64 "\n", head_addr);

	// calculate unread size
	uint64_t unread_burst_len = get_unread_mem_len(mem_len_max, tail_addr, head_addr);
	printf("unread_burst_len: %"PRIu64"\n", unread_burst_len);
	uint64_t data_out_len = unread_burst_len * word_size;
	uint32_t* data_out = calloc(data_out_len, sizeof(*data_out));

	// read
	printf("Running read.\n");
	PacketCapture_read(burst_size, data_out_len, tail_addr, data_out);

	// update tail
	increment_tail(mem_len_max, &tail_addr, unread_burst_len);
	PacketCapture_setTail(tail_addr);
	printf("tail=%" PRIu64 "\n", tail_addr);

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

