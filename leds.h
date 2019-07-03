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

#ifndef _LEDS_H
#define _LEDS_H

#include "geckonator/gpio.h"

#define LED_RED   GPIO_PA8
#define LED_GREEN GPIO_PA10
#define LED_BLUE  GPIO_PA9

static inline void led1_on(void) { gpio_clear(LED_RED); }
static inline void led2_on(void) { gpio_clear(LED_GREEN); }
static inline void led3_on(void) { gpio_clear(LED_BLUE); }

static inline void led1_off(void) { gpio_set(LED_RED); }
static inline void led2_off(void) { gpio_set(LED_GREEN); }
static inline void led3_off(void) { gpio_set(LED_BLUE); }

static inline void led1_toggle(void) { gpio_toggle(LED_RED); }
static inline void led2_toggle(void) { gpio_toggle(LED_GREEN); }
static inline void led3_toggle(void) { gpio_toggle(LED_BLUE); }

void leds_init(void);
void leds_uninit(void);

#endif
