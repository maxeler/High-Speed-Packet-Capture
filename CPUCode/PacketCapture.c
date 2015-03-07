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
#include "capture_data.h"
#include "log.h"

static int CHUNK_SIZE = 16;
static int BURST_SIZE = 192;
static int SERVER_PORT = 2511;
static max_net_connection_t DFE_CONNECTION = MAX_NET_CONNECTION_QSFP_TOP_10G_PORT2;
static int SERVERS_MAX = 3;
static int CAPTURE_DATA_SIZE = 256 / 8;

static int g_log_level;
static const char* g_log_ip;

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
	g_log_level = 3;

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
	int frames_len = 2;
	int data_size = CAPTURE_DATA_SIZE * frames_len;

	FILE* file = fopen("./capture.pcap", "w+");
	pcap_t* pcap = pcap_create(file, 0, 1, 0, 65535);
	assert(pcap != NULL);

	pcap_packet_t* packet = NULL;
	int sof_expected = 1;
	size_t total_packets = 0;
	while( 1 )
	{
		logf_debug("Reading %d frames (%dB)\n", frames_len, data_size);
		log_debug("\n");
		uint64_t* data = malloc(data_size);
		assert(data != NULL);
		PacketCapture_local_actions_t local_action =
		{
			.param_len = frames_len,
			.outstream_toCpu = data,
		};
		PacketCapture_local_run(engine, &local_action);

		// debug print data
		if( g_log_level >= 3 )
		{
			size_t data_len = data_size / sizeof(*data);
			char str[BUFSIZ];
			int str_len = 0;
			str_len += snprintf((str + str_len), BUFSIZ, "read %dB: ", data_size);
			for( int i=0; i<data_len; i++ )
			{
				if( i != 0 )
				{
					str_len += snprintf((str + str_len), (BUFSIZ - str_len), ".");
				}
				str_len += snprintf((str + str_len), (BUFSIZ - str_len), "%016"PRIx64, data[i]);
			}
			snprintf((str + str_len), (BUFSIZ - str_len), "\n");
			logf_trace("%s", str);
		}

		for( int i=0; i<frames_len; i++ )
		{
			logf_debug("[frame = %d]\n", i);

			int index = (4 * i);
			capture_data_t* capture_data = capture_data_parse(&data[index]);
			assert(capture_data != NULL);
			timestamp_t* timestamp = capture_data_get_timestamp(capture_data);
			frame_t* frame = capture_data_get_frame(capture_data);

			logf_debug("timestamp.doubt: %d\n", timestamp_get_doubt(timestamp));
			logf_debug("timestamp.valid: %d\n", timestamp_is_valid(timestamp));
			logf_debug("timestamp.value: %"PRIu64"\n", timestamp_get_value(timestamp));

			logf_debug("frame.sof: %d\n", frame_get_sof(frame));
			logf_debug("frame.eof: %d\n", frame_get_eof(frame));
			logf_debug("frame.mod: %d\n", frame_get_mod(frame));
			logf_debug("frame.size: %ld\n", frame_get_size(frame));
			logf_debug("frame.data: 0x%016"PRIx64"\n", *frame_get_data(frame));

			log_debug("\n");

			// init frame
			int sof = frame_get_sof(frame);
			int eof = frame_get_eof(frame);

//			assert(sof_expected == sof);
			if( sof_expected != sof )
			{
				fprintf(stderr, "\n\n*** ERROR: expected %d but got %d.\n\n", sof_expected, sof);
			}

			if( sof )
			{
				uint64_t packet_timestamp;
				if( timestamp_is_valid(timestamp) )
				{
					packet_timestamp = timestamp_get_value(timestamp);
				}
				else
				{
					logf_info("WARNING: Using system time due to invalid dfe timestamp (doubt=%d, valid=%d, value=%"PRIu64")\n",
							  timestamp_get_doubt(timestamp),
							  timestamp_is_valid(timestamp),
							  timestamp_get_value(timestamp));
					packet_timestamp = time(NULL);
				}

				packet = pcap_packet_init(pcap, timestamp_get_seconds(timestamp), timestamp_get_nanoseconds(timestamp));
				assert(packet != NULL);

				sof_expected = 0;
			}

			// build frame
			const uint64_t* data = frame_get_data(frame);
			int size = frame_get_size(frame);
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

			frame_free(frame);
			frame = NULL;
		}
	}

	// TODO: cleanup
}
