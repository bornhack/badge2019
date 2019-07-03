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

#include "geckonator/common.h"

#define EVENTS_MAX 32

static uint8_t events[EVENTS_MAX];
static volatile unsigned int events_head;
static volatile unsigned int events_tail;

void
event_add(uint8_t ev)
{
	unsigned int tail;

	__disable_irq();
	tail = events_tail;
	events[tail] = ev;
	tail = (tail + 1) % EVENTS_MAX;
	if (tail != events_head)
		events_tail = tail;
	__enable_irq();
}

static uint8_t
event_pop(void)
{
	unsigned int head = events_head;
	uint8_t ret = events[head];

	events_head = (head + 1) % EVENTS_MAX;
	return ret;
}

uint8_t
event_get(void)
{
	if (events_head == events_tail)
		return 0;
	return event_pop();
}

uint8_t
event_wait(void)
{
	while (events_head == events_tail)
		__WFI();
	return event_pop();
}

void
events_clear(void)
{
	events_head = events_tail;
}
