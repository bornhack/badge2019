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
#include <stdbool.h>

#include "ir.h"

#include "geckonator/clock.h"
#include "geckonator/gpio.h"
#include "geckonator/usart0.h"

#define IR_RX GPIO_PE11
#define IR_TX GPIO_PE10

#define IR_CLOCKDIV 319744 /* 24MHz / (16 * (1 + 319744/256)) = 1200Hz */

/*
 * 24MHz / (2 * (1 + x/256)) = 9600Hz
 * 24MHz = 9600Hz * 2 * (1 + x/256)
 * 24MHz / (2 * 9600Hz) = 1 + x/256
 * (24MHz / 19200Hz - 1) * 256 = x
 */

#if 1
#include <stdio.h>
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

void
ir_init(void)
{
	gpio_clear(IR_RX);
	gpio_clear(IR_TX);
	gpio_mode(IR_RX, GPIO_MODE_INPUT);
	gpio_mode(IR_TX, GPIO_MODE_PUSHPULL);

	clock_usart0_enable();
	/* location0:
	 * rx -> PBE11 -> IR_RX
	 * tx -> PBE10 -> IR_TX
	 */
	usart0_config(USART_CTRL_TXINV | USART_CTRL_RXINV);
	usart0_frame_8n1();
	usart0_clock_div(IR_CLOCKDIV);
	usart0_tx_enable();
	usart0_rx_enable();
	usart0_pins(USART_ROUTE_LOCATION_LOC0
			| USART_ROUTE_TXPEN
			| USART_ROUTE_RXPEN);
}

void
ir_uninit(void)
{
	usart0_rxtx_enable();
	usart0_pins(0);
	gpio_mode(IR_RX, GPIO_MODE_DISABLED);
	gpio_mode(IR_TX, GPIO_MODE_DISABLED);
	clock_usart0_enable();
}

void
ir_send(uint8_t c)
{
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdata(c);
}

int
ir_recv(void)
{
	if (!usart0_rx_valid())
		return -1;
	return usart0_rxdata();
}
