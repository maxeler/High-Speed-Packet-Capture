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
static int SERVER_COUNT = 2;

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

	/* init dfe */
	dfe_configure_interface(engine, MAX_NET_CONNECTION_QSFP_TOP_10G_PORT2, "5.5.5.1", "255.255.255.0");

	/* init clients */
	const char* server_addrs[] =
	{
		"5.5.5.2",
		"5.5.5.3",
	};

	// create sockets
	max_tcp_socket_t* sockets[SERVER_COUNT];
	uint8_t socket_nums[SERVER_COUNT];
	for( int i=0; i<SERVER_COUNT; i++ )
	{
		max_tcp_socket_t* sock = max_tcp_create_socket(engine, "serverStream");
		uint8_t socket_num = max_tcp_get_socket_number(sock);

		sockets[i] = sock;
		socket_nums[i] = socket_num;
	}

	// connect sockets
	for( int i=0; i<SERVER_COUNT; i++ )
	{
		const char* ip = server_addrs[i];
		max_tcp_socket_t* sock = sockets[i];

		struct in_addr ip_addr;
		inet_aton(ip, &ip_addr);

		printf("Connecting to server %s...\n", ip);
		max_tcp_connect(sock, &ip_addr, SERVER_PORT);
	}

	// wait for sockets to connect
	for( int i=0; i<SERVER_COUNT; i++ )
	{
		const char* ip = server_addrs[i];
		max_tcp_socket_t* sock = sockets[i];

		printf("Waiting for socket (%s) state: MAX_TCP_STATE_ESTABLISHED\n", ip);
		max_tcp_await_state(sock, MAX_TCP_STATE_ESTABLISHED, NULL);
	}

	PacketCapture_configServers_actions_t config_servers_action =
	{
		.param_sockets = socket_nums
	};
	PacketCapture_configServers_run(engine, &config_servers_action);

	/* init cpu */
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
