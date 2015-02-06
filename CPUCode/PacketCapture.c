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
static int SERVER_PORT = 2511;

static void dfe_configure_interface( max_engine_t* engine, max_net_connection_t interface, const char* ip, const char* netmask )
{
	assert(engine != NULL);
	assert(ip != NULL);
	assert(netmask != NULL);

	struct in_addr ip_addr;
	struct in_addr netmask_addr;
	inet_aton(ip, &ip_addr);
	inet_aton(netmask, &netmask_addr);

	max_ip_config(engine, interface, &ip_addr, &netmask_addr);
}

static max_tcp_socket_t* dfe_create_sockets( max_engine_t* engine, const char* stream, const char* ip, int port )
{
	assert(engine != NULL);

	max_tcp_socket_t* socket = max_tcp_create_socket(engine, stream);

	struct in_addr ip_addr;
	inet_aton(ip, &ip_addr);

	max_tcp_connect(socket, &ip_addr, port);

	printf("Waiting for socket state: MAX_TCP_STATE_ESTABLISHED\n");
	max_tcp_await_state(socket, MAX_TCP_STATE_ESTABLISHED, NULL);

	return socket;
}

int main( int argc, char** argv )
{
	(void) argc;
	(void) argv;

	max_file_t *maxfile = PacketCapture_init();
	printf("maxload start\n");
	max_engine_t * engine = max_load(maxfile, "*");
	printf("maxload end\n");

	int data_bursts = 1;
	int data_size = data_bursts * BURST_SIZE;
	uint64_t frames_len = data_size / CHUNK_SIZE;

	FILE* file = fopen("./capture.pcap", "w+");
	pcap_t* pcap = pcap_create(file, 0, 1, 0, 65535);
	assert(pcap != NULL);

	/* init */
	dfe_configure_interface(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT2, "5.5.5.2", "255.255.255.0");
	max_tcp_socket_t* socket = dfe_create_sockets(engine, "tcpServerStream", "5.5.5.1", SERVER_PORT);
	uint8_t socket_num = max_tcp_get_socket_number(socket);

	PacketCapture_configSend_actions_t config_send_action =
	{
		.param_socket = socket_num,
	};
	PacketCapture_configSend_run(engine, &config_send_action);

	PacketCapture_capture_run(engine, NULL);

	/* read */
	pcap_frame_t* frame = NULL;
	while( 1 )
	{
		printf("Running read.\n");


		printf("Reading %"PRIu64" frames (%dB)\n", frames_len, data_size);
		uint64_t* data = malloc(data_size);
		PacketCapture_read_actions_t read_action =
		{
			.param_bursts = data_bursts,
			.outstream_toCpu = data,
		};
		PacketCapture_read_run(engine, &read_action);
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

				// free
				pcap_frame_free(frame);
				frame = NULL;
				fflush(file);
			}

			frame_part_free(part);
			part = NULL;
		}
	}

	return EXIT_SUCCESS;
}
