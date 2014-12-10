/*
 * railgun_timer.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */

#include <railgun_common.h>
#include <signal.h>
#include <time.h>

static timer_t s_timer_id = NULL;

#define SIG_RAILGUN SIGUSR1

void railgun_timer_delete() {
	if (s_timer_id != NULL) {
		timer_delete(s_timer_id);
		s_timer_id = NULL;
	}
}

void railgun_timer_init(void (*timer_handler)(int sig, siginfo_t *si, void *uc), void* param) {
	railgun_timer_delete();
	struct sigevent ev;
	struct sigaction act;

	bzero(&act, sizeof(struct sigaction));
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = timer_handler;

	sigemptyset(&act.sa_mask);

	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		perror("fail to sigaction");
		exit(-1);
	}

	bzero(&ev, sizeof(struct sigevent));
	ev.sigev_signo = SIG_RAILGUN;
	ev.sigev_notify = SIGEV_SIGNAL;
	ev.sigev_value.sival_ptr = param;

	if (timer_create(CLOCK_REALTIME, &ev, &s_timer_id) == -1) {
		perror("fail to timer_create");
		exit(-1);
	}
}

void railgun_timer_set(int time_in_ms) {
	if (s_timer_id != NULL) {
		struct itimerspec it;
		it.it_interval.tv_sec = 0;
		it.it_interval.tv_nsec = 0;
		it.it_value.tv_sec = 0;
		it.it_value.tv_nsec = time_in_ms * 1e6;
		timer_settime(s_timer_id, 0, &it, NULL);
	}
}

void railgun_timer_cancel() {
	if (s_timer_id != NULL) {
		struct itimerspec it;
		it.it_interval.tv_sec = 0;
		it.it_interval.tv_nsec = 0;
		it.it_value.tv_sec = 0;
		it.it_value.tv_nsec = 0;
		timer_settime(s_timer_id, 0, &it, NULL);
	}
}
