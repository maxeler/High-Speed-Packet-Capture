/*
 * pcap.h
 *
 */

#ifndef PCAP_H_
#define PCAP_H_

#include <stdio.h>
#include <inttypes.h>


#define PCAP_NETWORK_ETHERNET   1
#define PCAP_TZONE_UTC          0

typedef struct pcap_packet_s pcap_packet_t;

typedef struct pcap_s pcap_t;

pcap_t* pcap_create( FILE* file, int32_t  thiszone, uint32_t network, uint32_t sigfigs, uint32_t snaplen );

pcap_packet_t* pcap_packet_init( pcap_t* pcap, uint32_t ts_sec, uint32_t ts_nsec );

void pcap_packet_append( pcap_packet_t* packet, const uint64_t* data, uint32_t size );

int pcap_packet_write( pcap_t* pcap, pcap_packet_t* packet );

int pcap_flush( pcap_t* pcap );

void pcap_packet_free( pcap_packet_t* packet );

#endif /* PCAP_H_ */
