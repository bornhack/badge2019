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

#include "geckonator/clock.h"
#include "geckonator/gpio.h"

#include "timer.h"
#include "leds.h"
#include "events.h"
#include "buttons.h"
#include "power.h"
#include "font.h"
#include "display.h"
#include "menu.h"

#ifdef NDEBUG
#define debug(...)
#else
#include <stdio.h>
#define debug(...) printf(__VA_ARGS__)
#endif
#define usb_debug(...)

static const struct button_config anypressed[BTN_MAX] = {
	[BTN_SUP]    = { .press = 1, },
	[BTN_SMID]   = { .press = 1, },
	[BTN_SDOWN]  = { .press = 1, },
	[BTN_UP]     = { .press = 1, },
	[BTN_DOWN]   = { .press = 1, },
	[BTN_LEFT]   = { .press = 1, },
	[BTN_RIGHT]  = { .press = 1, },
	[BTN_CENTER] = { .press = 1, },
};

extern const struct dp_cimage logo;

static void
idle(void)
{
	struct ticker tick1s;
	unsigned int count = 0;

	dp_fill(0, 0, 240, 240, 0x000);
	dp_cimage(0, 10, &logo);
	buttons_config(anypressed);

	ticker_start(&tick1s, 1000, 2);
	while (event_wait() == 2) {
		if (power_pressed()) {
			count += 1;
			if (count >= 3)
				power_off();
		} else
			count = 0;
	}
	ticker_stop(&tick1s);
}

void program(void);
void buttontest(void);
void showbmp(void);
void dumpir(void);
void snakemenu(void);

static const struct menuitem main_menu[] = {
	{ .label = "Browse program", .cb = program, },
	{ .label = "Button test",    .cb = buttontest, },
	{ .label = "Show BMP",       .cb = showbmp, },
	{ .label = "Dump IR data",   .cb = dumpir, },
	{ .label = "Snake",          .cb = snakemenu, },
};

void __noreturn
main(void)
{
	/* switch to 48MHz / 2 ushfrco as core clock */
	clock_ushfrco_48mhz_div2();
	clock_ushfrco_enable();
	while (!clock_ushfrco_ready())
		/* wait */;
	clock_hfclk_select_ushfrco();
	while (!clock_ushfrco_selected())
		/* wait */;
	clock_lfrco_enable();
	clock_hfrco_disable();

	/* disable auxfrco, only needed to program flash */
	clock_auxhfrco_disable();

	/* configure low frequency clocks */
	clock_le_enable();
	clock_lf_config(CLOCK_LFA_ULFRCO | CLOCK_LFB_DISABLED | CLOCK_LFC_DISABLED);
	while (clock_lf_syncbusy())
		/* wait */;

	timer_init();

	/* enable GPIOs */
	clock_gpio_enable();
	/* init leds */
	leds_init();
	/* init buttons */
	buttons_init(anypressed);

	/* init display */
	dp_init();
	dp_fill(0, 0, 240, 240, 0x000);
	dp_backlight_on();

	while (1) {
		idle();
		menu(main_menu, ARRAY_SIZE(main_menu), 0xCB0, 0x000);
	}
}
