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

#include <stdio.h>

#include "geckonator/gpio.h"

#include "timer.h"
#include "events.h"
#include "buttons.h"
#include "display.h"
#include "sdcard.h"
#include "ff.h"
#include "filepicker.h"


enum events {
	EV_PUSH = 1,
};

static const struct button_config anybutton[BTN_MAX] = {
	[BTN_SUP]    = { .press = EV_PUSH, },
	[BTN_SMID]   = { .press = EV_PUSH, },
	[BTN_SDOWN]  = { .press = EV_PUSH, },
	[BTN_UP]     = { .press = EV_PUSH, },
	[BTN_DOWN]   = { .press = EV_PUSH, },
	[BTN_LEFT]   = { .press = EV_PUSH, },
	[BTN_RIGHT]  = { .press = EV_PUSH, },
	[BTN_CENTER] = { .press = EV_PUSH, },
};

void
showbmp(void)
{
	FATFS fs;
	char path[255];

	/* init sdcard */
	sd_init();

	while (1) {
		FRESULT res;

		path[0] = '\0';
		res = filepicker(&fs, path, ARRAY_SIZE(path), 0x888, 0x000);
		if (res == FR_NO_FILE)
			break;

		dp_fill(0, 0, 240, 240, 0x000);
		if (res != FR_OK) {
			sprintf(path, "Error: %u", res);
			dp_puts(24, 24, 0x800, 0x000, path);
			buttons_config(anybutton);
			event_wait();
			break;
		}

		res = dp_showbmp_at(path, 0, 0);
		if (res != FR_OK) {
			sprintf(path, "Error: %u", res);
			dp_puts(24, 24, 0x800, 0x000, path);
		}
		buttons_config(anybutton);
		event_wait();
	}

	sd_uninit();
}
