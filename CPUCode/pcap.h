/*
 * pcap.h
 *
 */

#ifndef PCAP_H_
#define PCAP_H_

typedef struct pcap_frame_s pcap_frame_t;

typedef struct pcap_s pcap_t;

pcap_t* pcap_create( FILE* file, int32_t  thiszone, uint32_t network, uint32_t sigfigs, uint32_t snaplen );

pcap_frame_t* pcap_frame_init( pcap_t* pcap, uint32_t ts_sec, uint32_t ts_usec );

void pcap_frame_append( pcap_frame_t* frame, const uint64_t* data, uint32_t size );

int pcap_frame_write( pcap_t* pcap, pcap_frame_t* frame );

void pcap_frame_free( pcap_frame_t* frame );

#endif /* PCAP_H_ */
