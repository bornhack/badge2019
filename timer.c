/*
 * This file is part of badge2019.
 * Copyright 2019 Emil Renner Berthing <esmil@labitat.dk>
 *
 * badge2019 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * badge2019 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with badge2019. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdbool.h>

#include "geckonator/clock.h"
#include "geckonator/rtc.h"

#include "events.h"
#include "timer.h"

static struct timer_node timerlist;

static inline uint32_t
lessthan(uint32_t a, uint32_t b)
{
	return (a - b) & (1U << 23);
}

void
timer_add(struct timer_node *n)
{
	uint32_t timeout = n->timeout & 0xFFFFFFU;
	struct timer_node *p;

	n->timeout = timeout;

	__disable_irq();
	for (p = timerlist.prev; p != &timerlist; p = p->prev) {
		if (!lessthan(timeout, p->timeout))
			goto insert;
	}
	rtc_flag_comp0_clear();
	rtc_comp0_set(timeout);
	rtc_flag_comp0_enable();
insert:
	n->next = p->next;
	n->prev = p;
	p->next->prev = n;
	p->next = n;
	__enable_irq();

}

void
timer_remove(struct timer_node *n)
{
	struct timer_node *next;

	__disable_irq();
	next = n->next;
	if (timerlist.next == n && next != &timerlist)
		rtc_comp0_set(next->timeout);
	next->prev = n->prev;
	n->prev->next = next;
	__enable_irq();
}

void
RTC_IRQHandler(void)
{
	struct timer_node *n;

	while ((n = timerlist.next) != &timerlist) {
		struct timer_node *next;
		uint32_t now = rtc_counter();

		if (lessthan(now, n->timeout)) {
			rtc_flag_comp0_clear();
			rtc_comp0_set(n->timeout);
			return;
		}

		next = n->next;
		next->prev = &timerlist;
		timerlist.next = next;
		n->cb(n);
	}
	rtc_flag_comp0_disable();
}

void
timer_init(void)
{
	//clock_lfa_select_ulfrco();
	clock_rtc_div1();
	clock_rtc_enable();

	timerlist.next = timerlist.prev = &timerlist;

	while (clock_lf_syncbusy())
		/* wait */;

	rtc_config(RTC_ENABLE);

	/* enable rtc interrupt */
	NVIC_SetPriority(RTC_IRQn, 2);
	NVIC_EnableIRQ(RTC_IRQn);
}

struct timer_msleep {
	struct timer_node n;
	bool wait;
};

static void
timer_msleep_cb(struct timer_node *n)
{
	struct timer_msleep *s = (struct timer_msleep *)n;

	s->wait = false;
}

void
timer_msleep(uint32_t ms)
{
	struct timer_msleep s = {
		.n.timeout = timer_now() + ms,
		.n.cb = timer_msleep_cb,
		.wait = true,
	};

	timer_add(&s.n);
	while (s.wait)
		__WFI();
}

static void
ticker_cb(struct timer_node *n)
{
	struct ticker *t = (struct ticker *)n;

	event_add(t->ev);

	t->n.timeout += t->ms;
	timer_add(&t->n);
}

void
ticker_start(struct ticker *t, uint32_t ms, uint8_t ev)
{
	t->n.timeout = timer_now() + ms;
	t->n.cb = ticker_cb;
	t->ms = ms;
	t->ev = ev;
	timer_add(&t->n);
}

void
ticker_stop(struct ticker *t)
{
	timer_remove(&t->n);
}
