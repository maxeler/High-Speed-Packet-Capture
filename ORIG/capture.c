/*
 * Basic packet capture program using libpcap.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <pcap/pcap.h>
#include <signal.h>
#include <inttypes.h>

#include "log.h"
#include "args.h"


const int PCAP_SNAP_LEN = 1024;

int g_exiting;

void handle_exit( )
{
	g_exiting = 1;
	fprintf(stderr, "Exiting. (Send again to exit immediately)\n");
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
}

void print_packet( const void* _data, size_t len )
{
	const uint8_t* data = (const uint8_t*) _data;

	printf("Packet (%zdB): ", len);
	for( size_t i=0; i<len; i++ )
	{
		if( i != 0 )
		{
			printf(".");
		}

		printf("%02"PRIx8, data[i]);
	}

	printf("\n");
}

int main( int argc, char** argv )
{
	int ret;
	g_exiting = 0;

	arguments_t arguments;
	arguments.file = NULL;
	arguments.ifname = NULL;
	arguments.log_level = LOG_LEVEL_INFO;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	log_level_set(arguments.log_level);
	log_prepend_set("LOCAL");

	FILE* file = fopen(arguments.file, "w+");
	if( file == NULL )
	{
		fprintf(stderr, "Unable to open capture file: '%s' (errno=%d '%s')", arguments.file, errno, strerror(errno));
		ret = EXIT_FAILURE;
		goto error0;
	}

	// open device
	logf_trace("Opening device '%s'\n", arguments.ifname);
	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t* pcap = pcap_open_live(arguments.ifname, PCAP_SNAP_LEN, 1, 0, errbuf);
	if( pcap == NULL )
	{
		fprintf(stderr, "Unable to start capture: '%s'\n", errbuf);
		ret = EXIT_FAILURE;
		goto error1;
	}

	// open capture file
	logf_trace("Opening capture file '%s'\n", arguments.file);

	pcap_dumper_t* pdump = pcap_dump_open(pcap, arguments.file);
	if( pdump == NULL )
	{
		fprintf(stderr, "Unable to create capture file: '%s'\n", pcap_geterr(pcap));
		ret = EXIT_FAILURE;
		goto error2;
	}

	// set exit handler
	signal(SIGINT, handle_exit);
	signal(SIGTERM, handle_exit);

	// read packets
	logf_info("Running capture (%s)\n", arguments.ifname);

	size_t packets_count; // note: may overflow
	while( !g_exiting )
	{
		struct pcap_pkthdr* header;
		const u_char* data;
		int status = pcap_next_ex(pcap, &header, &data);

		if( status == 0 )
		{
			log_trace("pcap_next_ex timed out.");
		}
		else if( status != 1  )
		{
			fprintf(stderr, "Unable to read packet: '%s'\n", pcap_geterr(pcap));
			ret = EXIT_FAILURE;
			goto error3;
		}

		packets_count++;

		if( log_level_active(LOG_LEVEL_TRACE) )
		{
			print_packet(data, header->caplen);
		}

		logf_info("Captured %zd packets.\n", packets_count);

		pcap_dump((u_char*) pdump, header, data);
	}

	// cleanup
	pcap_dump_flush(pdump);

error3:
	pcap_dump_close(pdump);
	pdump = NULL;

error2:
	pcap_close(pcap);
	pcap = NULL;

error1:
	fclose(file);
	file = NULL;

error0:

	return ret;
}
