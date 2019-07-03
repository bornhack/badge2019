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

#include "geckonator/common.h"

#include "events.h"
#include "buttons.h"
#include "font.h"
#include "display.h"
#include "ff.h"
#include "filepicker.h"

struct dirbuf {
	unsigned int offset;
	unsigned int sel;
	unsigned int end;
	unsigned int max;
	char entry[20][14];
};

enum events {
	EV_UP = 1,
	EV_DOWN,
	EV_ENTER,
	EV_EXIT,
	EV_TICK1S,
};

static const struct button_config buttons[BTN_MAX] = {
	[BTN_SUP]    = { .press   = EV_UP,    .delay = 500, .rate = 100, .repeat = EV_UP, },
	[BTN_SMID]   = { .release = EV_ENTER, .delay = 500, .longpress = EV_EXIT, },
	[BTN_SDOWN]  = { .press   = EV_DOWN,  .delay = 500, .rate = 100, .repeat = EV_DOWN, },
	[BTN_UP]     = { .press   = EV_UP,    .delay = 500, .rate = 200, .repeat = EV_UP, },
	[BTN_DOWN]   = { .press   = EV_DOWN,  .delay = 500, .rate = 200, .repeat = EV_DOWN, },
	[BTN_LEFT]   = { .press   = EV_EXIT, },
	[BTN_RIGHT]  = { .press   = EV_ENTER, },
	[BTN_CENTER] = { .press   = EV_ENTER, },
};

static FRESULT
dirbuf_fill(struct dirbuf *db, const char *path)
{
	DIR dir;
	FILINFO fi;
	FRESULT res;
	unsigned int i;

	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

	db->end = 0;
	for (i = 0; i < db->max; i++) {
		char *p;
		char *q;

		res = f_readdir(&dir, &fi);
		if (res != FR_OK)
			goto err;

		p = fi.fname;
		if (*p == '\0') {
			db->max = i;
			break;
		}

		if (i < db->offset)
			continue;

		q = db->entry[db->end];
		do {
			*q++ = *p++;
		} while (*p != '\0');
		if (fi.fattrib & AM_DIR)
			*q++ = '/';
		*q = '\0';
		db->end += 1;
		if (db->end == ARRAY_SIZE(db->entry))
			break;
	}
	return f_closedir(&dir);

err:
	f_closedir(&dir);
	return res;
}

static FRESULT
dirbuf_init(struct dirbuf *db, char *path)
{
	db->offset = 0;
	db->sel = 0;
	db->end = 0;
	db->max = -1;
	return dirbuf_fill(db, path);
}

void
dirbuf_render(struct dirbuf *db, unsigned int fg444, unsigned int bg444)
{
	unsigned int lines = 240/font.height;
	unsigned int middle = (lines - 1)/2;
	unsigned int y = 0;
	unsigned int i;

	for (i = 0; i < lines; i++) {
		if (i == middle) {
			dp_fill(24, y, 16*font.width, font.height, fg444);
			dp_puts(36, y, bg444, fg444, db->entry[db->sel - db->offset]);
		} else {
			dp_fill(36, y, 14*font.width, font.height, bg444);
			if ((db->sel - db->offset + i) >= middle &&
					(db->sel - db->offset + i - middle) < db->end)
				dp_puts(36, y, fg444, bg444,
						db->entry[db->sel - db->offset + i - middle]);
		}
		y += font.height;
	}
}

static FRESULT
dirbuf_inc(struct dirbuf *db, const char *path)
{
	unsigned int lines = 240/font.height;
	unsigned int middle = (lines - 1)/2;

	db->sel += 1;

	if (db->offset + db->end == db->max)
		return FR_OK;

	if (db->sel - db->offset + lines - middle <= db->end)
		return FR_OK;

	db->offset = db->sel - middle;
	return dirbuf_fill(db, path);
}

static FRESULT
dirbuf_dec(struct dirbuf *db, const char *path)
{
	unsigned int lines = 240/font.height;
	unsigned int middle = (lines - 1)/2;

	db->sel -= 1;

	if (db->offset == 0)
		return FR_OK;

	if (db->sel >= db->offset + middle)
		return FR_OK;

	if (db->sel + lines < ARRAY_SIZE(db->entry) + middle)
		db->offset = 0;
	else
		db->offset = db->sel + lines - ARRAY_SIZE(db->entry) - middle;
	return dirbuf_fill(db, path);
}

FRESULT
filepicker(FATFS *fs, char *buf, size_t len,
		unsigned int fg444, unsigned int bg444)
{
	struct dirbuf db;
	unsigned int end;
	FRESULT res;

	dp_fill(0, 0, 240, 240, bg444);

	res = f_mount(fs, buf, 1);
	if (res != FR_OK)
		return res;

	end = 0;
	buf[end] = '\0';

restart:
	res = dirbuf_init(&db, buf);
	if (res != FR_OK)
		return res;
	dirbuf_render(&db, fg444, bg444);
	buttons_config(buttons);

	while (1) {
		switch (event_wait()) {
		case EV_UP:
			if (db.sel == 0)
				break;
			res = dirbuf_dec(&db, buf);
			if (res != FR_OK)
				return res;
			dirbuf_render(&db, fg444, bg444);
			break;
		case EV_DOWN:
			if (db.sel + 1 == db.max)
				break;
			res = dirbuf_inc(&db, buf);
			if (res != FR_OK)
				return res;
			dirbuf_render(&db, fg444, bg444);
			break;
		case EV_ENTER:
			{
				unsigned int k = end;
				const char *p = db.entry[db.sel - db.offset];

				buf[k++] = '/';
				while (k < len && *p != '\0')
					buf[k++] = *p++;

				if (k == len)
					break;

				buf[k] = '\0';
				if (buf[k - 1] != '/')
					return FR_OK;
				end = k - 1;
				buf[end] = '\0';
				goto restart;
			}
		case EV_EXIT:
			if (end == 0)
				return FR_NO_FILE;

			do {
				end -= 1;
				if (buf[end] == '/')
					break;
			} while (end > 0);
			buf[end] = '\0';
			goto restart;
		}
	}
}
