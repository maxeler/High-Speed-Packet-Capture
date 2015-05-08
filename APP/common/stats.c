/*
 * stats.c
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#include "stats.h"
#include "log.h"


const stats_t STATS_RESET = {0, 0, 0};
static const int REPORT_STATS_INTERVAL_S = 1;

struct sstats_s
{
	pthread_mutex_t* mutex;
	stats_t stats;
	stats_t stats_shared;
};

sstats_t* sstats_init( )
{
	sstats_t* this = malloc(sizeof(*this));
	if( this == NULL )
	{
		return NULL;
	}

	this->mutex = malloc(sizeof(*this->mutex));
	if( this->mutex == NULL )
	{
		sstats_free(this);
		this = NULL;

		return NULL;
	}

	int error = pthread_mutex_init(this->mutex, NULL);
	if( error )
	{
		free(this->mutex);
		this->mutex = NULL;

		sstats_free(this);
		this = NULL;

		return NULL;
	}

	this->stats = STATS_RESET;
	this->stats_shared = STATS_RESET;

	return this;
}

void sstats_free( sstats_t* this )
{
	assert(this != NULL);

	if( this->mutex != NULL )
	{
		pthread_mutex_destroy(this->mutex);

		free(this->mutex);
		this->mutex = NULL;
	}

	free(this);
	this = NULL;
}

void sstats_inc( sstats_t* this, stats_t* stats )
{
	assert(this != NULL);
	assert(stats != NULL);

	this->stats.bytes += stats->bytes;
	this->stats.frames += stats->frames;
	this->stats.packets += stats->packets;
}

void sstats_update( sstats_t* this )
{
	assert(this != NULL);

	pthread_mutex_lock(this->mutex);
	this->stats_shared = this->stats;
	pthread_mutex_unlock(this->mutex);
}

int sstats_try_update( sstats_t* this )
{
	assert(this != NULL);

	int error = pthread_mutex_trylock(this->mutex);
	if( error )
	{ // no lock
		return 0;
	}
	else // !error
	{ // locked
		// update shared stats
		this->stats_shared = this->stats;

		pthread_mutex_unlock(this->mutex);

		return 1;
	}
}

void sstats_get( sstats_t* this, stats_t* stats )
{
	assert(this != NULL);
	assert(stats != NULL);

	pthread_mutex_lock(this->mutex);
	*stats = this->stats_shared;
	pthread_mutex_unlock(this->mutex);
}

void* report_stats( void* _sstats )
{
	assert(_sstats != NULL);

	sstats_t* sstats = (sstats_t*) _sstats;
	stats_t stats_prev = STATS_RESET;
	int first_pass = 1;

	while( 1 )
	{
		stats_t stats;
		sstats_get(sstats, &stats);

		int stats_updated = stats_prev.frames != stats.frames;
		stats_prev = stats;

		if( first_pass || stats_updated )
		{
			logf_info("Total: %zd packets (%zd frames, %zdB)\n", stats.packets, stats.frames, stats.bytes);
		}

		if( first_pass )
		{
			first_pass = 0;
		}

		int rem = REPORT_STATS_INTERVAL_S;
		while( rem != 0 )
		{
			rem = sleep(rem);
		}
	}

	return NULL;
}
