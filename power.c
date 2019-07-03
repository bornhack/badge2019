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

#include "geckonator/gpio.h"
#include "geckonator/emu.h"

#include "timer.h"
#include "leds.h"
#include "buttons.h"
#include "display.h"
#include "sdcard.h"
#include "power.h"

void __noreturn
power_off(void)
{
	sd_uninit();

	dp_backlight_off();
	dp_off();
	dp_sleep_in();
	dp_uninit();

	leds_uninit();
	buttons_uninit();

	/* wait for power button to be released */
	while (!gpio_in(GPIO_PC4))
		timer_msleep(50);
	/* wait a bit more for debounce */
	timer_msleep(50);

	/* clear all wake-up requests */
	gpio_wakeup_clear();
	/* enable EM4 retention */
	gpio_retention_enable();
	/* wake up when pin goes low */
	gpio_wakeup_rising(0);
	/* enable EM4 wake-up from PC4 (POWER button) */
	gpio_wakeup_pins(GPIO_WAKEUP_PC4);

	/* do the EM4 handshake */
	emu_em4_enter();
}
