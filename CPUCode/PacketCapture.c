/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"
#include "frame.h"
#include "pcap.h"

static int CHUNK_SIZE = 16;
static int BURST_SIZE = 192;
static int SERVER_PORT = 2511;
static max_net_connection_t DFE_CONNECTION = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT2;
static int SERVERS_MAX = 3;

static int g_log_level;
static const char* g_log_ip;

#define logf(level, format, ...) if( g_log_level >= level ) printf("%s: "format, g_log_ip, __VA_ARGS__)
#define log(level, format) if( g_log_level >= level ) printf("%s: "format, g_log_ip)
#define log_info(format) log(1, format)
#define logf_info(format, ...) logf(1, format, __VA_ARGS__)
#define log_debug(format) log(2, format)
#define logf_debug(format, ...) logf(2, format, __VA_ARGS__)

static void dfe_configure_interface( max_engine_t* engine, max_net_connection_t interface, const char* ip, const char* netmask );

static void init_server_capture( max_engine_t * engine, max_net_connection_t dfe_connection, const char* dfe_ip, const char* dfe_netmask, const char* ips[], int ips_len );

static void local_capture_loop( max_engine_t* engine );

static void start_capture( max_engine_t* engine );

int main( int argc, char** argv )
{
	// TODO: parse properly and validate ips
	// TODO: parse log level
	// TODO: add local and server capture modes
	if( argc < 4 )
	{
		printf("%s: dfe-ip dfe-netmask server-ip0..server-ip%d\n", argv[0], (SERVERS_MAX - 1));
		return EXIT_FAILURE;
	}

	const char* dfe_ip = argv[1];
	const char* dfe_netmask = argv[2];

	const char* server_ips[SERVERS_MAX];
	int server_ips_len = 0;
	for( int i=0; i<SERVERS_MAX && (i + 3)<argc; i++ )
	{
		server_ips[i] = argv[i + 3];
		server_ips_len++;
	}

	g_log_ip = dfe_ip;
	g_log_level = 1;

	max_file_t *maxfile = PacketCapture_init();
	max_engine_t * engine = max_load(maxfile, "*");

	log_info("Initializing DFE.\n");
	init_server_capture(engine, DFE_CONNECTION, dfe_ip, dfe_netmask, server_ips, server_ips_len);

	log_info("Starting capture.\n");
	start_capture(engine);

	log_info("Servicing DFE.\n");
	local_capture_loop(engine);

	return EXIT_SUCCESS;
}

static void start_capture( max_engine_t* engine )
{
	PacketCapture_capture_actions_t capture_actions = { };
	PacketCapture_capture_run(engine, &capture_actions);
}

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

static void init_server_capture( max_engine_t * engine, max_net_connection_t dfe_connection, const char* dfe_ip, const char* dfe_netmask, const char* ips[], int ips_len )
{
	assert(engine != NULL);
	assert(dfe_ip != NULL);
	assert(dfe_netmask != NULL);
	assert(ips != NULL);
	assert(ips_len > 0);

	// init networking connection
	dfe_configure_interface(engine, dfe_connection, dfe_ip, dfe_netmask);

	// create sockets
	max_tcp_socket_t* sockets[ips_len];
	uint8_t socket_nums[ips_len];
	for( int i=0; i<ips_len; i++ )
	{
		max_tcp_socket_t* sock = max_tcp_create_socket(engine, "serverStream");
		uint8_t socket_num = max_tcp_get_socket_number(sock);

		sockets[i] = sock;
		socket_nums[i] = socket_num;
	}

	// connect sockets
	log_info("Connecting to servers...\n");
	for( int i=0; i<ips_len; i++ )
	{
		const char* ip = ips[i];
		max_tcp_socket_t* sock = sockets[i];

		struct in_addr ip_addr;
		inet_aton(ip, &ip_addr);

		max_tcp_connect(sock, &ip_addr, SERVER_PORT);
	}

	// wait for sockets to connect
	for( int i=0; i<ips_len; i++ )
	{
		const char* ip = ips[i];
		max_tcp_socket_t* sock = sockets[i];

		logf_info("Waiting on %s...\n", ip);
		max_tcp_await_state(sock, MAX_TCP_STATE_ESTABLISHED, NULL);
	}

	// configure dfe with socket handles
	PacketCapture_configServers_actions_t config_servers_action =
	{
		.param_socketsALen = 1,
		.param_socketsA = socket_nums,
		.param_socketsBLen = ips_len - 1,
		.param_socketsB = socket_nums + 1,
	};
	PacketCapture_configServers_run(engine, &config_servers_action);
}

static void local_capture_loop( max_engine_t* engine )
{
	int data_bursts = 1;
	int data_size = data_bursts * BURST_SIZE;
	int frames_len = data_size / CHUNK_SIZE;

	FILE* file = fopen("./capture.pcap", "w+");
	pcap_t* pcap = pcap_create(file, 0, 1, 0, 65535);
	assert(pcap != NULL);

	pcap_packet_t* packet = NULL;
	int sof_expected = 1;
	size_t total_packets = 0;
	while( 1 )
	{
		logf_debug("Reading %d frames (%dB)\n", frames_len, data_size);
		uint64_t* data = malloc(data_size);
		PacketCapture_read_actions_t read_action =
		{
			.param_bursts = data_bursts,
			.outstream_toCpu = data,
		};
		PacketCapture_read_run(engine, &read_action);

		// debug print data
		if( g_log_level >= 2 )
		{
			size_t data_len = data_size / sizeof(*data);
			char str[BUFSIZ];
			int str_len = 0;
			str_len += snprintf((str + str_len), BUFSIZ, "read %ldB: ", data_len);
			for( int i=0; (i * sizeof(*data))<data_len; i++ )
			{
				if( i != 0 )
				{
					str_len += snprintf((str + str_len), (BUFSIZ - str_len), ".");
				}
				str_len += snprintf((str + str_len), (BUFSIZ - str_len), "%016"PRIx64, data[i]);
			}
			snprintf((str + str_len), (BUFSIZ - str_len), "\n");
			logf_debug("%s", str);
		}

		for( int i=0; i<frames_len; i++ )
		{
			logf_debug("[frame = %d]\n", i);

			int frame_index = (2 * i);
			uint64_t* frame_data = &data[frame_index];
			logf_debug("data = 0x%016"PRIx64".%016"PRIx64"\n", frame_data[1], frame_data[0]);
			logf_debug("frame[%d][0]: 0x%"PRIx64"\n", i, frame_data[0]);
			logf_debug("frame[%d][1]: 0x%"PRIx64"\n", i, frame_data[1]);

			// parse frame part
			frame_part_t* part = frame_parse(frame_data);
			assert(part != NULL);

			logf_debug("frame[%d].data: 0x%"PRIx64"\n", i, *frame_get_data(part));
			logf_debug("frame[%d].eof: %d\n", i, frame_get_eof(part));
			logf_debug("frame[%d].sof: %d\n", i, frame_get_sof(part));
			logf_debug("frame[%d].size: %ldB\n", i, frame_get_size(part));
			log_debug("\n");

			// init frame
			int sof = frame_get_sof(part);
			int eof = frame_get_eof(part);

			assert(sof_expected == sof);

			if( sof )
			{
				packet = pcap_packet_init(pcap, time(NULL), 0);
				assert(packet != NULL);

				sof_expected = 0;
			}

			// build frame
			const uint64_t* data = frame_get_data(part);
			int size = frame_get_size(part);
			pcap_packet_append(packet, data, size);

			// write frame
			if( eof )
			{
				pcap_packet_write(pcap, packet);
				pcap_flush(pcap);

				// free
				pcap_packet_free(packet);
				packet = NULL;

				sof_expected = 1;
				total_packets++;
				logf_info("Total packet(s): %ld\n", total_packets);
			}

			frame_free(part);
			part = NULL;
		}
	}

	// TODO: cleanup
}
