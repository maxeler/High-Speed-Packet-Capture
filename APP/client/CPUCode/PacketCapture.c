/*
 * PacketCapture.c
 */

#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"
#include "frame.h"
#include "pcap.h"
#include "capture_data.h"
#include "log.h"
#include "utils.h"


static int PCAP_TZONE = PCAP_TZONE_UTC;
static int PCAP_SIGFIGS = 0;
static int PCAP_NETWORK = PCAP_NETWORK_ETHERNET;
static int PCAP_SNAPLEN = 65535;

static int SERVER_PORT = 2511;
static int SERVERS_LEN_MAX = 3;
static int CAPTURE_DATA_SIZE = 256 / 8;

volatile sig_atomic_t g_shutdown = 0;

static void dfe_configure_interface( max_engine_t* engine, max_net_connection_t interface, const char* ip, const char* netmask );

static void init_server_capture( max_engine_t * engine,
								 max_net_connection_t dfe_if,
								 const char* dfe_ip,
								 const char* dfe_netmask,
								 const char* ipsA[],
								 int ipsA_len,
								 const char* ipsB[],
								 int ipsB_len);

static int local_read_loop( max_engine_t* engine );

int main( int argc, char** argv )
{
	// TODO: add proper arg parsing (port, ipsA, ipsB, log level, local and server capture modes)
	if( argc < 4 || argc > (3 + SERVERS_LEN_MAX) )
	{
		printf("%s: dfe-ip dfe-netmask <server-ips>\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char* dfe_ip = argv[1];
	const char* dfe_netmask = argv[2];

	const char* server_ips[SERVERS_LEN_MAX];
	int server_ips_len = 0;
	for( int i=0; i<SERVERS_LEN_MAX && (i + 3)<argc; i++ )
	{
		server_ips[i] = argv[i + 3];
		server_ips_len++;
	}

	g_log_prepend = dfe_ip;
	g_log_level = LOG_LEVEL_TRACE;
	int local_enabled = 1;

	// load bitstream onto DFE
	log_info("Loading bitstream.\n");
	max_file_t *maxfile = PacketCapture_init();
	max_engine_t * engine = max_load(maxfile, "*");

	// configure DFE
	log_info("Initializing DFE.\n");
	const char* server_if_name = max_get_constant_string(maxfile, "server_if_name");
	max_net_connection_t dfe_server_if = -1;
	int error = max_net_connection_from_name(&dfe_server_if, server_if_name);
	if( error )
	{
		fprintf(stderr, "Error: Unable to lookup max_net_connection for '%s'\n", server_if_name);
		return EXIT_FAILURE;
	}
	init_server_capture(engine, dfe_server_if, dfe_ip, dfe_netmask, server_ips, 1, server_ips + 1, server_ips_len - 1);

	if( local_enabled )
	{ // local data transfer
		log_info("Servicing DFE.\n");

		error = local_read_loop(engine);
		if( error )
		{
			fprintf(stderr, "Error: Unable to read local capture data\n");
			return EXIT_FAILURE;
		}
	}
	else
	{ // hold onto card
		pause();
	}

	// cleanup
	max_unload(engine);
	engine = NULL;
	PacketCapture_free();

	return EXIT_SUCCESS;
}

static void init_server_capture( max_engine_t * engine,
								 max_net_connection_t dfe_if,
								 const char* dfe_ip,
								 const char* dfe_netmask,
								 const char* ipsA[],
								 int ipsA_len,
								 const char* ipsB[],
								 int ipsB_len)
{
	assert(engine != NULL);
	assert(dfe_ip != NULL);
	assert(dfe_netmask != NULL);
	assert(ipsA != NULL);
	assert(ipsB != NULL);
	assert(ipsA_len > 0);
	assert(ipsB_len > 0);

	int ips_len = ipsA_len + ipsB_len;
	const char* ips[ips_len];

	int i=0;
	for( int j=0; j<ipsA_len; j++, i++ )
	{
			ips[i] = ipsA[j];
	}
	for( int j=0; j<ipsB_len; j++, i++ )
	{
			ips[i] = ipsB[j];
	}

	// init networking connection
	dfe_configure_interface(engine, dfe_if, dfe_ip, dfe_netmask);

	// create sockets
	int socks_len = ipsA_len + ipsB_len;
	max_tcp_socket_t* socks[socks_len];
	uint8_t sock_nums[socks_len];
	uint8_t* sockA_nums = sock_nums;
	uint8_t* sockB_nums = sock_nums + ipsA_len;

	for( int i=0; i<socks_len; i++ )
	{
		max_tcp_socket_t* sock = max_tcp_create_socket(engine, "serverStream");
		sock_nums[i] = max_tcp_get_socket_number(sock);
		socks[i] = sock;
	}

	// connect sockets
	log_info("Connecting to servers...\n");
	for( int i=0; i<ips_len; i++ )
	{
		const char* ip = ips[i];
		max_tcp_socket_t* sock = socks[i];

		struct in_addr ip_addr;
		inet_aton(ip, &ip_addr);

		max_tcp_connect(sock, &ip_addr, SERVER_PORT);
	}

	// wait for sockets to connect
	for( int i=0; i<ips_len; i++ )
	{
		const char* ip = ips[i];
		max_tcp_socket_t* sock = socks[i];

		logf_info("Waiting on %s...\n", ip);
		max_tcp_await_state(sock, MAX_TCP_STATE_ESTABLISHED, NULL);
	}

	log_info("Running.\n");

	// configure DFE with socket handles
	PacketCapture_enableRemoteCapture_actions_t action =
	{
		.param_socketsALen = ipsA_len,
		.param_socketsA = sockA_nums,
		.param_socketsBLen = ipsB_len,
		.param_socketsB = sockB_nums,
	};
	PacketCapture_enableRemoteCapture_run(engine, &action);
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

static int local_read_loop( max_engine_t* engine )
{
	int frames_len = 2;
	int data_size = CAPTURE_DATA_SIZE * frames_len;

	FILE* file = fopen("./capture.pcap", "w+");
	pcap_t* pcap = pcap_create(file, PCAP_TZONE, PCAP_NETWORK, PCAP_SIGFIGS, PCAP_SNAPLEN);
	assert(pcap != NULL);

	// disable pcie timeout
	max_config_set_int64(MAX_CONFIG_PCIE_TIMEOUT, 0);

	// service
	pcap_packet_t* packet = NULL;
	int sof_expected = 1;
	size_t total_packets = 0;
	while( !g_shutdown )
	{
		logf_debug("Reading %d frames (%dB)\n", frames_len, data_size);
		log_debug("\n");

		uint64_t* data = calloc(1, data_size);
		assert(data != NULL);

		PacketCapture_readCaptureData_actions_t action = {
			.param_len = frames_len,
			.outstream_toCpu = data,
		};
		PacketCapture_readCaptureData_run(engine, &action);

		// debug print data
		if( g_log_level >= LOG_LEVEL_TRACE )
		{
			size_t data_len = data_size / sizeof(*data);
			char str[BUFSIZ];
			int str_len = 0;
			str_len += snprintf((str + str_len), BUFSIZ, "read %dB: ", data_size);
			for( size_t i=0; i<data_len; i++ )
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

			assert(sof_expected == sof);
//			if( sof_expected != sof )
//			{
//				fprintf(stderr, "\n\n*** ERROR: expected %d but got %d.\n\n", sof_expected, sof);
//			}

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
				logf_info("Total packets: %ld\n", total_packets);
			}

			frame_free(frame);
			frame = NULL;
		}
	}

	// cleanup
	pcap_flush(pcap);
	fclose(file);
	file = NULL;

	return 0;
}
