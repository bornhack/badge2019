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

#include "events.h"
#include "buttons.h"
#include "font.h"
#include "display.h"
#include "ir.h"

static const struct button_config buttons[BTN_MAX] = {
	[BTN_SMID]   = { .release = 2, .delay = 500, .longpress = 1, },
	[BTN_LEFT]   = { .press = 1, },
	[BTN_RIGHT]  = { .press = 2, },
	[BTN_CENTER] = { .press = 2, },
};

static void
rerender(uint8_t lines[10][20], unsigned int j)
{
	unsigned int y;

	for (y = 0; y < 240; y += font.height) {
		j += 1;
		if (j == 10)
			j = 0;

		dp_fill(0, y, 240, font.height, 0x000);
		dp_puts(0, y, 0xAAA, 0x000, (char *)lines[j]);
	}
}

void
dumpir(void)
{
	uint8_t lines[10][20];
	unsigned int i = 0;
	unsigned int j;

	dp_fill(0, 0, 240, 240, 0x00);
	ir_init();

	for (j = 0; j < 10; j++)
		lines[j][0] = '\0';

	buttons_config(buttons);

	j = 0;
	while (1) {
		int ch;
		const char *p;

		switch (event_get()) {
		case 1:
			ir_uninit();
			return;
		case 2:
			for (p = "Hello World!\n"; *p != '\0'; p++)
				ir_send((uint8_t)*p);
			break;
		}

		ch = ir_recv();
		if (ch < 0)
			continue;

		switch (ch) {
		case '\n':
			lines[j][i] = '\0';
			j += 1;
			if (j == 10)
				j = 0;
			i = 0;
			lines[j][i] = '\0';
			rerender(lines, j);
			break;
		default:
			lines[j][i] = ch;
			if (i == 19) {
				j += 1;
				if (j == 10)
					j = 0;
				i = 0;
				lines[j][i] = '\0';
				rerender(lines, j);
			} else {
				dp_putchar(i*font.width, 9*font.height, 0xAAA, 0x000, ch);
				i += 1;
			}
			break;
		}
	}
}
