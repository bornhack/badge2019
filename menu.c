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

#include <stdlib.h>

#include "font.h"
#include "timer.h"
#include "buttons.h"
#include "events.h"
#include "display.h"
#include "power.h"
#include "menu.h"

enum events {
	EV_UP = 1,
	EV_DOWN,
	EV_ENTER,
	EV_EXIT,
	EV_TICK1S,
};

static const struct button_config menubuttons[BTN_MAX] = {
	[BTN_SUP]    = { .press   = EV_UP,    .delay = 500, .rate = 100, .repeat = EV_UP, },
	[BTN_SMID]   = { .release = EV_ENTER, .delay = 500, .longpress = EV_EXIT, },
	[BTN_SDOWN]  = { .press   = EV_DOWN,  .delay = 500, .rate = 100, .repeat = EV_DOWN, },
	[BTN_UP]     = { .press   = EV_UP,    .delay = 500, .rate = 200, .repeat = EV_UP, },
	[BTN_DOWN]   = { .press   = EV_DOWN,  .delay = 500, .rate = 200, .repeat = EV_DOWN, },
	[BTN_LEFT]   = { .press   = EV_EXIT, },
	[BTN_RIGHT]  = { .press   = EV_ENTER, },
	[BTN_CENTER] = { .press   = EV_ENTER, },
};

static void
menu_render(unsigned int fg444, unsigned int bg444,
		const struct menuitem *menu, size_t len, unsigned int sel)
{
	unsigned int lines = 240/font.height;
	unsigned int middle = (lines - 1)/2;
	unsigned int i;
	unsigned int y = 2;

	for (i = 0; i < lines; i++) {
		if (i == middle) {
			dp_fill(10, y, 220, font.height, fg444);
			dp_puts(20, y, bg444, fg444, menu[sel].label);
		} else {
			dp_fill(20, y, 200, font.height, bg444);
			if ((i + sel) >= middle && (i + sel - middle) < len)
				dp_puts(20, y, fg444, bg444,
						menu[i + sel - middle].label);
		}
		y += font.height;
	}
}

void
menu(const struct menuitem *menu, size_t len,
		unsigned int fg444, unsigned int bg444)
{
	struct ticker tick1s;
	unsigned int count;
	unsigned int i = 0;

restart:
	count = 0;
	dp_fill(0, 0, 240, 240, bg444);
	menu_render(fg444, bg444, menu, len, i);
	buttons_config(menubuttons);
	ticker_start(&tick1s, 1000, EV_TICK1S);

	while (1) {
		switch ((enum events)event_wait()) {
		case EV_UP:
			if (i > 0) {
				i -= 1;
				menu_render(fg444, bg444, menu, len, i);
			}
			break;
		case EV_DOWN:
			if ((i + 1) < len) {
				i += 1;
				menu_render(fg444, bg444, menu, len, i);
			}
			break;
		case EV_ENTER:
			if (menu[i].cb) {
				ticker_stop(&tick1s);
				menu[i].cb();
				goto restart;
			}
			break;
		case EV_EXIT:
			ticker_stop(&tick1s);
			return;
		case EV_TICK1S:
			if (power_pressed()) {
				count += 1;
				if (count >= 3)
					power_off();
			} else
				count = 0;
			break;
		}
	}
}
