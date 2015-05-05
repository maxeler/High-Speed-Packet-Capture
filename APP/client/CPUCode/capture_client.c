/*
 * CaptureClient.c
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
#include <argp.h>
#include <pthread.h>

#include "Maxfiles.h"
#include "MaxSLiCInterface.h"
#include "frame.h"
#include "pcap.h"
#include "capture_data.h"
#include "log.h"
#include "utils.h"
#include "args.h"
#include "stats.h"


static int PCAP_TZONE = PCAP_TZONE_UTC;
static int PCAP_SIGFIGS = 0;
static int PCAP_NETWORK = PCAP_NETWORK_ETHERNET;
static int PCAP_SNAPLEN = 65535;

static int SERVER_PORT = 2511;
static size_t CAPTURE_DATA_SIZE = 256 / 8;
static size_t DATA_SLOTS = 512;

static const char* DFE_LOCAL_STR = "Local";

static void init_server_capture( max_engine_t * engine,
								 max_net_connection_t dfe_if,
								 struct in_addr* dfe_ip,
								 struct in_addr* dfe_netmask,
								 const struct in_addr ipsA[],
								 int ipsA_len,
								 const struct in_addr ipsB[],
								 int ipsB_len);

static int local_read_loop( max_engine_t* engine, FILE* file );

int main( int argc, char** argv )
{
	arguments_t arguments;
	arguments.log_level = LOG_LEVEL_INFO;
	arguments.local_file = NULL;
	arguments.ipsA_len = 0;
	arguments.ipsB_len = 0;
	arguments.local_enabled = 0;
	arguments.remote_enabled = 0;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	char* dfe_ip_str;
	if( arguments.remote_enabled )
	{
		dfe_ip_str = strdup(inet_ntoa(arguments.dfe_ip));
	}
	else // !arguments.remote_enabled
	{
		dfe_ip_str = strdup(DFE_LOCAL_STR);
	}

	log_prepend_set(dfe_ip_str);
	log_level_set(arguments.log_level);

	// load bitstream onto DFE
	log_info("Loading bitstream.\n");
	max_file_t *maxfile = CaptureClient_init();
	max_engine_t* engine = max_load(maxfile, "*");

	// configure DFE
	log_info("Initializing DFE.\n");

	assert(arguments.remote_enabled || arguments.local_enabled);

	if( arguments.remote_enabled )
	{ // remote data transfer
		const char* server_if_name = max_get_constant_string(maxfile, "server_if_name");
		max_net_connection_t dfe_server_if = -1;

		int error = max_net_connection_from_name(&dfe_server_if, server_if_name);
		if( error )
		{
			fprintf(stderr, "Error: Unable to lookup max_net_connection for '%s'\n", server_if_name);
			return EXIT_FAILURE;
		}

		init_server_capture(engine, dfe_server_if, &arguments.dfe_ip, &arguments.dfe_netmask, arguments.ipsA, arguments.ipsA_len, arguments.ipsB, arguments.ipsB_len);
	}

	if( arguments.local_enabled )
	{ // local data transfer
		log_info("Servicing DFE.\n");

		int error = local_read_loop(engine, arguments.local_file);
		if( error )
		{
			fprintf(stderr, "Error: Unable to read local capture data\n");
			return EXIT_FAILURE;
		}

		fclose(arguments.local_file);
		arguments.local_file = NULL;
	}
	else
	{ // hold onto card
		pause();
	}

	// cleanup
	max_unload(engine);
	engine = NULL;
	CaptureClient_free();
	free(dfe_ip_str);
	dfe_ip_str = NULL;

	return EXIT_SUCCESS;
}

static void init_server_capture( max_engine_t * engine,
								 max_net_connection_t dfe_if,
								 struct in_addr* dfe_ip,
								 struct in_addr* dfe_netmask,
								 const struct in_addr ipsA[],
								 int ipsA_len,
								 const struct in_addr ipsB[],
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
	const struct in_addr* ips[ips_len];

	int i=0;
	for( int j=0; j<ipsA_len; j++, i++ )
	{
		ips[i] = &ipsA[j];
	}
	for( int j=0; j<ipsB_len; j++, i++ )
	{
		ips[i] = &ipsB[j];
	}

	// init networking connection
	max_ip_config(engine, dfe_if, dfe_ip, dfe_netmask);

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
		max_tcp_connect(socks[i], ips[i], SERVER_PORT);
	}

	// wait for sockets to connect
	for( int i=0; i<ips_len; i++ )
	{
		logf_info("Waiting on %s...\n", inet_ntoa(*ips[i]));
		max_tcp_await_state(socks[i], MAX_TCP_STATE_ESTABLISHED, NULL);
	}

	log_info("Running.\n");

	// configure DFE with socket handles
	CaptureClient_enableRemoteCapture_actions_t action =
	{
		.param_socketsALen = ipsA_len,
		.param_socketsA = sockA_nums,
		.param_socketsBLen = ipsB_len,
		.param_socketsB = sockB_nums,
	};
	CaptureClient_enableRemoteCapture_run(engine, &action);
}

static int local_read_loop( max_engine_t* engine, FILE* file )
{
	pcap_t* pcap = pcap_init(file, PCAP_TZONE, PCAP_NETWORK, PCAP_SIGFIGS, PCAP_SNAPLEN);
	assert(pcap != NULL);

	// start stats reporting thread
	sstats_t* sstats = sstats_init();
	pthread_t* thread = NULL;
	if( log_level_active(LOG_LEVEL_INFO) )
	{
		thread = malloc(sizeof(*thread));
		assert(thread != NULL);

		int error = pthread_create(thread, NULL, report_stats, (void*) sstats);
		if( error )
		{
			fprintf(stderr, "Error: Unable to create thread\n");
			return 1;
		}
	}

	// enable local capture
	CaptureClient_enableLocalCapture_actions_t action;
	CaptureClient_enableLocalCapture_run(engine, &action);

	// service
	pcap_packet_t* packet = NULL;
	int sof_expected = 1;

	uint64_t* data_buffer;
	posix_memalign((void*) &data_buffer, CAPTURE_DATA_SIZE, (DATA_SLOTS * CAPTURE_DATA_SIZE));
	assert(data_buffer != NULL);
	max_llstream_t* stream = max_llstream_setup(engine, "toCpu", DATA_SLOTS, CAPTURE_DATA_SIZE, data_buffer);

	while( 1 )
	{
		uint64_t* data;

		// check for data
		ssize_t status = 0;
		while( status <= 0 )
		{
			status = max_llstream_read(stream, DATA_SLOTS, (void**) &data);
			if( status < 0 )
			{ // error
				fprintf(stderr, "Error: Unable to read llstream\n");
				return 1;
			}
		}
		size_t data_len = (size_t) status;

		logf_debug("Read %zd frames\n\n", data_len);

		// debug print data
		if( log_level_active(LOG_LEVEL_TRACE) )
		{
			logf_trace("read %zdB: ", (data_len * CAPTURE_DATA_SIZE));
			for( size_t i=0; i<data_len; i++ )
			{
				if( i != 0 )
				{
					logf_append_trace("%s", ".");
				}
				logf_append_trace("%016"PRIx64, data[i]);
			}
			logf_append_trace("%s", "\n");
		}

		// process
		assert(CAPTURE_DATA_SIZE % sizeof(*data) == 0);
		assert(CAPTURE_DATA_SIZE > 0);
		int index_step = (CAPTURE_DATA_SIZE / sizeof(*data));

		for( size_t i=0; i<data_len; i++ )
		{
			int index = (index_step * i);

			// debug print frame data
			logf_debug("[frame = %zd]\n", i);

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
			logf_debug("frame.size: %zd\n", frame_get_size(frame));
			logf_debug("frame.data: 0x%016"PRIx64"\n", *frame_get_data(frame));

			log_debug("\n");

			// init frame
			int sof = frame_get_sof(frame);
			int eof = frame_get_eof(frame);

			assert(sof_expected == sof);

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

			}

			// update stats
			stats_t stats = STATS_RESET;
			stats.frames = 1;
			stats.bytes = size;
			stats.packets = (eof == 1) ? 1 : 0;

			sstats_inc(sstats, &stats);
			sstats_try_update(sstats);

			// cleanup
			capture_data_free(capture_data);
			capture_data = NULL;
			max_llstream_read_discard(stream, 1);
		}
	}

	// cleanup
	pcap_flush(pcap);

	if( thread != NULL )
	{
		pthread_cancel(*thread);

		free(thread);
		thread = NULL;
	}

	sstats_free(sstats);
	sstats = NULL;

	if( packet != NULL )
	{
		pcap_packet_free(packet);
		packet = NULL;
	}

	max_llstream_release(stream);
	stream = NULL;

	free(data_buffer);
	data_buffer = NULL;

	free(data_buffer);
	data_buffer = NULL;

	return 0;
}
