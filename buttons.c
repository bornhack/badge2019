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

#include "geckonator/gpio.h"

#include "timer.h"
#include "events.h"
#include "buttons.h"

#define POLL_RATE 50

static const gpio_pin_t buttonpin[BTN_MAX] = {
	[BTN_SUP]    = GPIO_PC10,
	[BTN_SMID]   = GPIO_PC9,
	[BTN_SDOWN]  = GPIO_PC8,
	[BTN_UP]     = GPIO_PF2,
	[BTN_DOWN]   = GPIO_PF3,
	[BTN_LEFT]   = GPIO_PF4,
	[BTN_RIGHT]  = GPIO_PF5,
	[BTN_CENTER] = GPIO_PB11,
	/* [BTN_POWER]    = GPIO_PC4, */
};

static const struct button_config *volatile buttonconfig;
static volatile uint8_t pressed;

struct button_state {
	struct timer_node n;
	uint16_t delay_left;
};

static struct button_state buttonstate[BTN_MAX];

static void
button_cb(struct timer_node *n)
{
	struct button_state *s = (struct button_state *)n;
	enum button btn = (enum button)(s - buttonstate);
	const struct button_config *c = &buttonconfig[btn];

	if (gpio_in(buttonpin[btn])) {
		if (c->release > 0)
			event_add(c->release);
		gpio_flag_clear(buttonpin[btn]);
		gpio_flag_enable(buttonpin[btn]);
		pressed -= 1;
	} else if (s->delay_left == 0) {
		if (c->repeat > 0)
			event_add(c->repeat);
		s->n.timeout += (c->rate > 0) ? c->rate : POLL_RATE;
		timer_add(&s->n);
	} else if (s->delay_left <= POLL_RATE) {
		s->delay_left = 0;
		if (c->longpress > 0)
			event_add(c->longpress);
		else if (c->repeat > 0)
			event_add(c->repeat);
		s->n.timeout += (c->rate > 0) ? c->rate : POLL_RATE;
		timer_add(&s->n);
	} else {
		s->delay_left -= POLL_RATE;
		s->n.timeout += (s->delay_left > POLL_RATE) ? POLL_RATE : s->delay_left;
		timer_add(&s->n);
	}
}

static void
button_click(enum button btn)
{
	const struct button_config *c;
	struct button_state *s;
	uint16_t delay;

	gpio_flag_disable(buttonpin[btn]);
	pressed += 1;

	c = &buttonconfig[btn];
	delay = (c->delay > 0) ? c->delay : POLL_RATE;

	s = &buttonstate[btn];
	s->delay_left = delay;
	s->n.timeout = timer_now();
	s->n.timeout += (delay > POLL_RATE) ? POLL_RATE : delay;
	timer_add(&s->n);
	if (c->press > 0)
		event_add(c->press);
}

void
GPIO_EVEN_IRQHandler(void)
{
	uint32_t flags = gpio_flags_enabled(gpio_flags());

	if (gpio_flag(flags, GPIO_PF2))
		button_click(BTN_UP);
	/*
	if (gpio_flag(flags, GPIO_PC4))
		button_click(BTN_POWER);
	*/
	if (gpio_flag(flags, GPIO_PF4))
		button_click(BTN_LEFT);
	if (gpio_flag(flags, GPIO_PC8))
		button_click(BTN_SDOWN);
	if (gpio_flag(flags, GPIO_PE10))
		button_click(BTN_SUP);
}

void
GPIO_ODD_IRQHandler(void)
{
	uint32_t flags = gpio_flags_enabled(gpio_flags());

	if (gpio_flag(flags, GPIO_PF3))
		button_click(BTN_DOWN);
	if (gpio_flag(flags, GPIO_PF5))
		button_click(BTN_RIGHT);
	if (gpio_flag(flags, GPIO_PC9))
		button_click(BTN_SMID);
	if (gpio_flag(flags, GPIO_PB11))
		button_click(BTN_CENTER);
}

void
buttons_init(const struct button_config config[BTN_MAX])
{
	unsigned int i;

	for (i = 0; i < BTN_MAX; i++) {
		gpio_pin_t pin = buttonpin[i];

		gpio_set(pin);
		gpio_mode(pin, GPIO_MODE_INPUTPULL);

		buttonstate[i].n.cb = button_cb;

		gpio_flag_select(pin);
		gpio_flag_falling_enable(pin);
		gpio_flag_clear(pin);
		gpio_flag_enable(pin);
	}

	/* init power button */
	gpio_set(GPIO_PC4);
	gpio_mode(GPIO_PC4, GPIO_MODE_INPUTPULL);

	buttonconfig = config;

	NVIC_SetPriority(GPIO_EVEN_IRQn, 3);
	NVIC_SetPriority(GPIO_ODD_IRQn, 3);
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

void
buttons_uninit(void)
{
	unsigned int i;

	/* disable button gpio */
	for (i = 0; i < BTN_MAX; i++) {
		gpio_pin_t pin = buttonpin[i];

		gpio_flag_disable(pin);
		gpio_mode(pin, GPIO_MODE_DISABLED);
	}

	/* wait for button callbacks to complete */
	while (pressed > 0)
		__WFI();
}

void
buttons_config(const struct button_config config[BTN_MAX])
{
	while (1) {
		__disable_irq();
		if (pressed == 0)
			break;
		__enable_irq();
		__WFI();
	}
	events_clear();
	buttonconfig = config;
	__enable_irq();
}
