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

#ifndef _BUTTONS_H
#define _BUTTONS_H

#include <stdint.h>

enum button {
	BTN_SUP,
	BTN_SMID,
	BTN_SDOWN,
	BTN_UP,
	BTN_DOWN,
	BTN_LEFT,
	BTN_RIGHT,
	BTN_CENTER,
	BTN_MAX,
};

struct button_config {
	uint16_t delay;
	uint16_t rate;
	uint8_t press;
	uint8_t release;
	uint8_t longpress;
	uint8_t repeat;
};

void buttons_init(const struct button_config config[BTN_MAX]);
void buttons_uninit(void);
void buttons_config(const struct button_config config[BTN_MAX]);

#endif
