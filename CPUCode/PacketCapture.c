/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"
#include "frame_part.h"
#include "pcap.h"


static int CHUNK_SIZE = 16;
static int BURST_SIZE = 192;

int main( int argc, char** argv )
{

	(void) argc;
	(void) argv;

//	max_file_t *maxfile = PacketCapture_init();

	int data_bursts = 1;
	int data_size = data_bursts * BURST_SIZE;
	int frames_len = data_size / CHUNK_SIZE;

	FILE* file = fopen("./capture.pcap", "w+");
	pcap_t* pcap = pcap_create(file, 0, 1, 0, 65535);
	assert(pcap != NULL);

	/* read */
	pcap_frame_t* frame = NULL;
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

		for( int i=0; i<frames_len; i++ )
		{
			printf("[frame = %d]\n", i);

			int frame_index = (2 * i);
			uint64_t* frame_data = &data[frame_index];
			printf("data = 0x%016"PRIx64".%016"PRIx64"\n", frame_data[1], frame_data[0]);
			printf("frame[%d][0]: 0x%"PRIx64"\n", i, frame_data[0]);
			printf("frame[%d][1]: 0x%"PRIx64"\n", i, frame_data[1]);

			// parse frame part
			frame_part_t* part = frame_part_parse(frame_data);
			assert(part != NULL);

			printf("part[%d].data: 0x%"PRIx64"\n", i, *frame_part_get_data(part));
			printf("part[%d].eof: %d\n", i, frame_part_get_eof(part));
			printf("part[%d].sof: %d\n", i, frame_part_get_sof(part));
			printf("part[%d].size: %ldB\n", i, frame_part_get_size(part));
			printf("\n");

			// init frame
			int sof = frame_part_get_sof(part);
			int eof = frame_part_get_eof(part);
			if( sof )
			{
				if( frame != NULL )
				{
					pcap_frame_free(frame);
					frame = NULL;
				}
				frame = pcap_frame_init(pcap, time(NULL), 0);
				assert(frame != NULL);
			}

			// build frame
			const uint64_t* data = frame_part_get_data(part);
			int size = frame_part_get_size(part);
			pcap_frame_append(frame, data, size);

			// write frame
			if( eof )
			{
				pcap_frame_write(pcap, frame);
				fflush(file);
			}

			frame_part_free(part);
			part = NULL;
		}
	}

	return EXIT_SUCCESS;
}
