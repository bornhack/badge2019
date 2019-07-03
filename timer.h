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

#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

#include "geckonator/rtc.h"

struct timer_node;

typedef void timer_cb(struct timer_node *n);

struct timer_node {
	struct timer_node *next;
	struct timer_node *prev;
	uint32_t timeout;
	timer_cb *cb;
};

struct ticker {
	struct timer_node n;
	uint32_t ms;
	uint8_t ev;
};

static inline uint32_t timer_now(void) { return rtc_counter(); }

void timer_init(void);
void timer_add(struct timer_node *n);
void timer_remove(struct timer_node *n);

void timer_msleep(uint32_t ms);

void ticker_start(struct ticker *t, uint32_t ms, uint8_t ev);
void ticker_stop(struct ticker *t);

#endif
