#include <stdlib.h>

#include "geckonator/common.h"

#include "events.h"
#include "buttons.h"
#include "font.h"
#include "display.h"
#include "menu.h"

#define FG444 0xCB0
#define BG444 0x000

struct event {
	const char **lines;
	size_t len;
};

#include "program.data"

enum events {
	EV_UP = 1,
	EV_DOWN,
	EV_EXIT,
};

static const struct button_config buttons[BTN_MAX] = {
	[BTN_SUP]    = { .press   = EV_UP,    .delay = 500, .rate = 100, .repeat = EV_UP, },
	[BTN_SMID]   = { .delay = 500, .longpress = EV_EXIT, },
	[BTN_SDOWN]  = { .press   = EV_DOWN,  .delay = 500, .rate = 100, .repeat = EV_DOWN, },
	[BTN_UP]     = { .press   = EV_UP,    .delay = 500, .rate = 200, .repeat = EV_UP, },
	[BTN_DOWN]   = { .press   = EV_DOWN,  .delay = 500, .rate = 200, .repeat = EV_DOWN, },
	[BTN_LEFT]   = { .press   = EV_EXIT, },
};

static void
render(const struct event *e)
{
	unsigned int y = 0;
	unsigned int i;

	dp_puts(2*font.width, y, BG444, FG444, e->lines[0]);
	y += font.height;
	dp_fill(2*font.width, y, 240-4*font.width, font.height, FG444);
	dp_puts(2*font.width, y, BG444, FG444, e->lines[1]);
	y += font.height;
	for (i = 2; i < 240/font.height; i++) {
		dp_fill(0, y, 240, font.height, BG444);
		if (i < e->len)
			dp_puts(0, y, FG444, BG444, e->lines[i]);
		y += font.height;
		if (y >= 240)
			break;
	}
}

static void
browse_day(const char *name, const struct event *day, size_t len)
{
	unsigned int i = 0;

	dp_fill(0, 0, 240, 2*font.height, FG444);
	render(&day[i]);
	buttons_config(buttons);
	while (1) {
		switch ((enum events)event_wait()) {
		case EV_UP:
			if (i > 0) {
				i -= 1;
				render(&day[i]);
			}
			break;
		case EV_DOWN:
			if (i + 1 < len) {
				i += 1;
				render(&day[i]);
			}
			break;
		case EV_EXIT:
			return;
		}
	}
}

static void
browse_thursday(void)
{
	browse_day("Thursday", day0, ARRAY_SIZE(day0));
}

static void
browse_friday(void)
{
	browse_day("Friday", day1, ARRAY_SIZE(day1));
}

static void
browse_saturday(void)
{
	browse_day("Saturday", day2, ARRAY_SIZE(day2));
}

static void
browse_sunday(void)
{
	browse_day("Sunday", day3, ARRAY_SIZE(day3));
}

static void
browse_monday(void)
{
	browse_day("Monday", day4, ARRAY_SIZE(day4));
}

static void
browse_tuesday(void)
{
	browse_day("Tuesday", day5, ARRAY_SIZE(day5));
}

static void
browse_wednesday(void)
{
	browse_day("Wednesday", day6, ARRAY_SIZE(day6));
}

static const struct menuitem main_menu[] = {
	{ .label = "Thursday",  .cb = browse_thursday, },
	{ .label = "Friday",    .cb = browse_friday, },
	{ .label = "Saturday",  .cb = browse_saturday, },
	{ .label = "Sunday",    .cb = browse_sunday, },
	{ .label = "Monday",    .cb = browse_monday, },
	{ .label = "Tuesday",   .cb = browse_tuesday, },
	{ .label = "Wednesday", .cb = browse_wednesday, },
};

void
program(void)
{
	menu(main_menu, ARRAY_SIZE(main_menu), FG444, BG444);
}
