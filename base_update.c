#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "base.h"
#include "cargo.h"
#include "item.h"
#include "log.h"
#include "universe.h"

#define BASE_UPDATE_INTERVAL 10		/* in seconds, no larger than once a day */

#define SECONDS_PER_DAY (24 * 60 * 60)
#define BASE_UPDATE_FRACTION (SECONDS_PER_DAY / BASE_UPDATE_INTERVAL)

pthread_t thread;
int terminate;

pthread_condattr_t termination_attr;
pthread_cond_t termination_cond;
pthread_mutex_t termination_lock;

static void update_base(struct base *base, uint32_t iteration)
{
	struct cargo *cargo;
	long change, mod, fraction_iteration;

	pthread_rwlock_wrlock(&base->items_lock);

	list_for_each_entry(cargo, &base->items, list) {
		if (!cargo->daily_change)
			continue;

		change = cargo->daily_change / BASE_UPDATE_FRACTION;
		mod = cargo->daily_change % BASE_UPDATE_FRACTION;
		fraction_iteration = BASE_UPDATE_FRACTION / cargo->daily_change;

		if (mod && fraction_iteration && iteration % fraction_iteration == 0) {
			if (cargo->daily_change > 0)
				change++;
			else if (cargo->daily_change < 0)
				change--;
		}

		if (change < 0 && cargo->amount < -change)
			change = -cargo->amount;
		else if (change > 0 && cargo->max - cargo->amount < change)
			change = cargo->max - cargo->amount;

		struct cargo *req;
		struct list_head *lh;
		ptrlist_for_each_entry(req, &cargo->requires, lh) {
			if (change > 0 && req->amount < change)
				change = req->amount;
			else if (change < 0 && req->amount < -change)
				change = -req->amount;
		}

		cargo->amount += change;

		if (change < 0) {
			ptrlist_for_each_entry(req, &cargo->requires, lh)
				req->amount += change;
		} else {
			ptrlist_for_each_entry(req, &cargo->requires, lh)
				req->amount -= change;
		}
	}

	pthread_rwlock_unlock(&base->items_lock);
}

static void update_all_bases(uint32_t iteration)
{
	struct base *base;

	list_for_each_entry(base, &univ.bases, list)
		update_base(base, iteration);
}

static void* base_update_worker(void *ptr)
{
	struct timespec next, now;
	uint32_t iteration = 0;

	if (clock_gettime(CLOCK_MONOTONIC, &next))
		goto clock_err;

	do {
		pthread_rwlock_rdlock(&univ.bases_lock);
		update_all_bases(iteration);
		pthread_rwlock_unlock(&univ.bases_lock);

		iteration++;
		if (iteration >= BASE_UPDATE_FRACTION)
			iteration = 0;

		next.tv_sec += BASE_UPDATE_INTERVAL;

		do {
			pthread_mutex_lock(&termination_lock);
			if (terminate) {
				pthread_mutex_unlock(&termination_lock);
				break;
			}
			pthread_cond_timedwait(&termination_cond, &termination_lock, &next);
			pthread_mutex_unlock(&termination_lock);

			if (clock_gettime(CLOCK_MONOTONIC, &now))
				goto clock_err;

		} while (!terminate && now.tv_sec < next.tv_sec);

	} while (!terminate);

	return NULL;

clock_err:
	log_printfn("base_update", "clock_gettime() failed");
	return NULL;
}

int start_updating_bases(void)
{
	int r;

	if (pthread_condattr_init(&termination_attr))
		return -1;

	if (pthread_condattr_setclock(&termination_attr, CLOCK_MONOTONIC))
		return -1;

	if (pthread_mutex_init(&termination_lock, NULL))
		return -1;

	if (pthread_cond_init(&termination_cond, &termination_attr))
		return -1;

	if (pthread_create(&thread, NULL, base_update_worker, NULL))
		return -1;

	return 0;
}

void stop_updating_bases(void)
{
	pthread_mutex_lock(&termination_lock);
	terminate = 1;
	pthread_cond_signal(&termination_cond);
	pthread_mutex_unlock(&termination_lock);

	pthread_join(thread, NULL);

	pthread_cond_destroy(&termination_cond);
	pthread_condattr_destroy(&termination_attr);
	pthread_mutex_destroy(&termination_lock);
}
