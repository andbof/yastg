#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "port.h"
#include "cargo.h"
#include "item.h"
#include "log.h"
#include "universe.h"

#define PORT_UPDATE_INTERVAL 10		/* in seconds, no larger than once a day */

#define SECONDS_PER_DAY (24 * 60 * 60)
#define PORT_UPDATE_FRACTION (SECONDS_PER_DAY / PORT_UPDATE_INTERVAL)

pthread_t thread;
int terminate;

pthread_condattr_t termination_attr;
pthread_cond_t termination_cond;
pthread_mutex_t termination_lock;

static void update_port(struct port *port, uint32_t iteration)
{
	struct cargo *cargo;
	long change, mod, fraction_iteration;

	pthread_rwlock_wrlock(&port->items_lock);

	list_for_each_entry(cargo, &port->items, list) {
		if (!cargo->daily_change)
			continue;

		change = cargo->daily_change / PORT_UPDATE_FRACTION;
		mod = cargo->daily_change % PORT_UPDATE_FRACTION;
		fraction_iteration = PORT_UPDATE_FRACTION / cargo->daily_change;

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

	pthread_rwlock_unlock(&port->items_lock);
}

static void update_all_ports(uint32_t iteration)
{
	struct port *port;

	list_for_each_entry(port, &univ.ports, list)
		update_port(port, iteration);
}

static void* port_update_worker(void *ptr)
{
	struct timespec next, now;
	uint32_t iteration = 0;

	if (clock_gettime(CLOCK_MONOTONIC, &next))
		goto clock_err;

	do {
		pthread_rwlock_rdlock(&univ.ports_lock);
		update_all_ports(iteration);
		pthread_rwlock_unlock(&univ.ports_lock);

		iteration++;
		if (iteration >= PORT_UPDATE_FRACTION)
			iteration = 0;

		next.tv_sec += PORT_UPDATE_INTERVAL;

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
	log_printfn(LOG_PORT_UPDATE, "clock_gettime() failed");
	return NULL;
}

int start_updating_ports(void)
{
	sigset_t old, new;

	sigfillset(&new);

	if (pthread_condattr_init(&termination_attr))
		goto err;

	if (pthread_condattr_setclock(&termination_attr, CLOCK_MONOTONIC))
		goto err_free_attr;

	if (pthread_mutex_init(&termination_lock, NULL))
		goto err_free_attr;

	if (pthread_cond_init(&termination_cond, &termination_attr))
		goto err_free_mutex;

	if (pthread_sigmask(SIG_SETMASK, &new, &old))
		goto err_free_mutex;

	if (pthread_create(&thread, NULL, port_update_worker, NULL))
		goto err_free_cond;

	if (pthread_sigmask(SIG_SETMASK, &old, NULL))
		goto err_cancel_thread;

	return 0;

err_cancel_thread:
	pthread_cancel(thread);
err_free_cond:
	pthread_cond_destroy(&termination_cond);
err_free_mutex:
	pthread_mutex_destroy(&termination_lock);
err_free_attr:
	pthread_condattr_destroy(&termination_attr);
err:
	return -1;
}

void stop_updating_ports(void)
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
