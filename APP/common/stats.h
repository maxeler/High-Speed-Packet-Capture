/*
 * stats.h
 * A collection of common statistics related functionality.
 */

#ifndef STATS_H_
#define STATS_H_

#include <inttypes.h>

typedef struct sstats_s sstats_t;

typedef struct stats_s
{
	size_t packets;
	size_t frames;
	size_t bytes;
} stats_t;

extern const stats_t STATS_RESET;

sstats_t* sstats_init( );

void sstats_free( sstats_t* sstats );

/*
 * Increments the local statistics by stats.
 */
void sstats_inc( sstats_t* sstats, stats_t* stats );

/*
 * Updates the stats returned by get.
 * Blocking.
 */
void sstats_update( sstats_t* sstats );

/*
 * Tries to update the stats returned by get.
 * Returns whether or not the stats were updated.
 * Non-Blocking.
 */
int sstats_try_update( sstats_t* sstats );

/*
 * Returns a copy of the synchronized stats.
 */
void sstats_get( sstats_t* this, stats_t* stats );

void* report_stats( void* arg );

#endif /* STATS_H_ */
