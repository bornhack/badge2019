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

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ff.h"

struct dp_image565 {
	uint8_t width;
	uint8_t height;
	uint8_t data[];
};

//typedef uint32_t dp_bitstream_data_t;
typedef uint8_t dp_bitstream_data_t;
struct dp_cimage {
	uint8_t width;
	uint8_t height;
	dp_bitstream_data_t data[];
};

void dp_backlight_on(void);
void dp_backlight_off(void);
void dp_backlight_toggle(void);

void dp_reset(void);
uint8_t dp_read1(uint8_t cmd);
void dp_read(uint8_t cmd, uint8_t *buf, size_t len);
void dp_write1(uint8_t cmd);
void dp_write(uint8_t cmd, const uint8_t *buf, size_t len);
void dp_rotate(bool rotate);
void dp_sleep_in(void);
void dp_sleep_out(void);
void dp_off(void);
void dp_on(void);
void dp_init(void);
void dp_uninit(void);

void dp_fill(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int rgb444);
void dp_fill666(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int rgb);

void dp_putchar(unsigned int x, unsigned int y, unsigned int fg444, unsigned int bg444, int ch);
void dp_puts(unsigned int x, unsigned int y, unsigned int fg444, unsigned int bg444, const char *str);

void dp_image565(unsigned int x, unsigned int y, const struct dp_image565 *img);
void dp_cimage(unsigned int x, unsigned int y, const struct dp_cimage *img);

FRESULT dp_showbmp(FIL *f, unsigned int x, unsigned int y);
FRESULT dp_showbmp_at(const char *path, unsigned int x, unsigned int y);

#endif
