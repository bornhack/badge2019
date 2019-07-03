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

#include "sdcard.h"

#include "geckonator/clock.h"
#include "geckonator/gpio.h"
#include "geckonator/usart0.h"

#define SD_CD   GPIO_PC0
#define SD_CLK  GPIO_PB13
#define SD_CS   GPIO_PB14
#define SD_MISO GPIO_PB8
#define SD_MOSI GPIO_PB7

#define SD_CLOCKDIV_INIT 7488 /* 24MHz / (2 * (1 + 7488/256)) < 400kHz */
#define SD_CLOCKDIV_RUN     0 /* 24MHz / (2 * (1 +    0/256)) =  12MHz */

#define SD_TRIES (1 << 15)

#if 0
#include <stdio.h>
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

static bool block_addressing;

void
sd_init(void)
{
	gpio_set(SD_CD);
	gpio_set(SD_CLK);
	gpio_set(SD_CS);
	gpio_set(SD_MOSI);
	gpio_set(SD_MISO);
	gpio_mode(SD_CD,   GPIO_MODE_INPUT);
	gpio_mode(SD_CLK,  GPIO_MODE_PUSHPULL);
	gpio_mode(SD_CS,   GPIO_MODE_WIREDAND);
	gpio_mode(SD_MISO, GPIO_MODE_INPUTPULL);
	gpio_mode(SD_MOSI, GPIO_MODE_PUSHPULL);

	clock_usart0_enable();
	/* location4:
	 * clk -> PB13 -> SD_CLK
	 * cs  -> PB14 -> SD_CS
	 * rx  -> PB8  -> SD_MISO
	 * tx  -> PB7  -> SD_MOSI
	 */
	usart0_config(USART_CTRL_MSBF
			| USART_CTRL_SYNC);
	usart0_frame_bits(8);
	usart0_master_enable();
	usart0_tx_enable();
	usart0_pins(USART_ROUTE_LOCATION_LOC4
			| USART_ROUTE_CLKPEN
			/*| USART_ROUTE_CSPEN */
			| USART_ROUTE_TXPEN
			| USART_ROUTE_RXPEN);
}

void
sd_uninit(void)
{
	usart0_tx_enable();
	usart0_pins(0);
	gpio_mode(SD_CD,   GPIO_MODE_DISABLED);
	gpio_mode(SD_CLK,  GPIO_MODE_DISABLED);
	gpio_mode(SD_CS,   GPIO_MODE_DISABLED);
	gpio_mode(SD_MISO, GPIO_MODE_DISABLED);
	gpio_mode(SD_MOSI, GPIO_MODE_DISABLED);
	clock_usart0_disable();
}

static uint8_t
sd__getbyte(void)
{
	while (!usart0_rx_valid())
		/* wait */;
	return usart0_rxdata();
}

static uint8_t
sd__cmd(const uint8_t cmd[6], uint8_t *response, unsigned int len)
{
	unsigned int i;
	uint8_t ret;

	usart0_txdata(0xFF);
	for (i = 0; i < 5; i++) {
		while (!usart0_tx_buffer_level())
			/* wait */;
		usart0_txdata(cmd[i]);
	}
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdatax(USART_TXDATAX_RXENAT | cmd[5]);
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdata(0xFF);
	while (!usart0_tx_buffer_level())
		/* wait */;
	for (i = SD_TRIES; i > 0; i--) {
		usart0_txdata(0xFF);
		ret = sd__getbyte();
		if (ret != 0xFF)
			break;
		//debug(".");
	}
	if (ret & 0xFE)
		goto out;
	for (; len > 0; len--) {
		usart0_txdata(0xFF);
		*response++ = sd__getbyte();
	}
out:
	usart0_rx_disable();
	return ret;
}

uint8_t
sd_cmd(const uint8_t cmd[6], uint8_t *response, unsigned int len)
{
	uint8_t ret;

	gpio_clear(SD_CS);
	ret = sd__cmd(cmd, response, len);
	while (!usart0_tx_complete())
		/* wait */;
	gpio_set(SD_CS);
	return ret;
}

uint8_t
sd_wakeup(void)
{
	unsigned int i;
	uint32_t response;
	const uint8_t cmd0[6]    = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
	const uint8_t cmd8[6]    = { 0x48, 0x00, 0x00, 0x01, 0xAA, 0x87 };
	const uint8_t cmd55[6]   = { 0x77, 0x00, 0x00, 0x00, 0x00, 0xFF };
	const uint8_t acmd41a[6] = { 0x69, 0x40, 0x00, 0x00, 0x00, 0xFF };
	const uint8_t cmd58[6]   = { 0x7A, 0x00, 0x00, 0x00, 0x00, 0xFF };
	const uint8_t acmd41b[6] = { 0x69, 0x00, 0x00, 0x00, 0x00, 0xFF };
	const uint8_t cmd1[6]    = { 0x41, 0x00, 0x00, 0x00, 0x00, 0xFF };
	const uint8_t cmd16[6]   = { 0x50, 0x00, 0x00, 0x02, 0x00, 0xFF };
	uint8_t ret;

	block_addressing = false;
	usart0_clock_div(SD_CLOCKDIV_INIT);

	gpio_clear(SD_CS);

	/* send 80 (at least 75) dummy bits */
	for (i = 10; i > 0; i--) {
		usart0_txdata(0xFF);
		while (!usart0_tx_buffer_level())
			/* wait */;
	}
	/* software reset aka go idle */
	ret = sd__cmd(cmd0, NULL, 0);
	debug("cmd0: %02x\r\n", ret);
	if (ret != 0x01)
		goto out;

	/* check voltage range or conclude it's an old card */
	ret = sd__cmd(cmd8, (uint8_t *)&response, 4);
	debug("cmd8: %02x (%lx)\r\n", ret, __builtin_bswap32(response));
	if (ret == 0x01) {
		if (response != 0xAA010000U) {
			ret = 0x80;
			goto out;
		}

		for (i = SD_TRIES; i > 0; i--) {
			ret = sd__cmd(cmd55, NULL, 0);
			if (ret != 0x01) {
				debug("cmd55: %02x\r\n", ret);
				goto out;
			}

			ret = sd__cmd(acmd41a, NULL, 0);
			if (ret == 0x00)
				break;
			if (ret != 0x01) {
				debug("acmd41: %02x\r\n", ret);
				goto out;
			}
		}
		debug("acmd41: %02x\r\n", ret);
		if (ret != 0x00) {
			ret = 0x81;
			goto out;
		}
		ret = sd__cmd(cmd58, (uint8_t *)&response, 4);
		debug("cmd58: %02x (%lx)\r\n", ret, __builtin_bswap32(response));
		if (ret != 0x00)
			goto out;
		/* CCS bit means SDHC/SDXC */
		if (response & 0x40) {
			block_addressing = true;
			goto out;
		}
	} else if (ret == 0x05) {
		for (i = SD_TRIES; i > 0; i--) {
			ret = sd__cmd(cmd55, NULL, 0);
			debug("cmd55: %02x\r\n", ret);
			if (ret & 0x04) {
				ret = 0x01;
				break;
			}
			if (ret != 0x01)
				goto out;

			ret = sd__cmd(acmd41b, NULL, 0);
			debug("acmd41: %02x\r\n", ret);
			if (ret == 0x00)
				break;
			if (ret & 0x04) {
				ret = 0x01;
				break;
			}
			if (ret != 0x01)
				goto out;
		}
		for (i = SD_TRIES; ret == 0x01 && i > 0; i--) {
			ret = sd__cmd(cmd1, NULL, 0);
			debug("cmd1: %02x\r\n", ret);
		}
		if (ret != 0x00) {
			ret = 0x82;
			goto out;
		}
	} else
		goto out;

	/* force blocksize to 512 bytes */
	ret = sd__cmd(cmd16, NULL, 0);
	debug("cmd16: %02x\r\n", ret);
out:
	while (!usart0_tx_complete())
		/* wait */;
	gpio_set(SD_CS);
	usart0_clock_div(SD_CLOCKDIV_RUN);
	return ret;
}

uint8_t
sd_status(uint8_t *status)
{
	const uint8_t cmd13[] = { 0x4D, 0x00, 0x00, 0x00, 0x00, 0xFF };
	return sd_cmd(cmd13, status, 1);
}

static uint8_t
sd__read(const uint8_t cmd[5], uint8_t *buf, unsigned int len)
{
	unsigned int i;
	uint8_t ret;

	gpio_clear(SD_CS);

	usart0_txdata(0xFF);
	for (i = 0; i < 5; i++) {
		while (!usart0_tx_buffer_level())
			/* wait */;
		usart0_txdata(cmd[i]);
	}
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdatax(USART_TXDATAX_RXENAT | 0xFF);
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdata(0xFF);
	while (!usart0_tx_buffer_level())
		/* wait */;
	for (i = SD_TRIES; i > 0; i--) {
		usart0_txdata(0xFF);
		ret = sd__getbyte();
		if (ret != 0xFF)
			break;
		//debug(".");
	}
	if (ret != 0x00) {
		debug("cmd: %02x\r\n", ret);
		goto out;
	}
	for (i = SD_TRIES; i > 0; i--) {
		usart0_txdata(0xFF);
		ret = sd__getbyte();
		if (ret != 0xFF)
			break;
		//debug(".");
	}
	if (ret != 0xFE) {
		debug("data token: %02x\r\n", ret);
		goto out;
	}
	for (; len > 0; len--) {
		usart0_txdata(0xFF);
		*buf++ = sd__getbyte();
	}
	for (i = 2; i > 0; i--) {
		usart0_txdata(0xFF);
		sd__getbyte();
	}
	ret = 0x00;
out:
	usart0_rx_disable();
	while (!usart0_tx_complete())
		/* wait */;
	gpio_set(SD_CS);
	return ret;
}

uint8_t
sd_getcsd(uint8_t csd[16])
{
	const uint8_t cmd9[5] = { 0x49, 0x00, 0x00, 0x00, 0x00 };

	debug("sd_getcsd(buf):\r\n");
	return sd__read(cmd9, csd, 16);
}

uint8_t
sd_getcid(uint8_t cid[16])
{
	const uint8_t cmd10[5] = { 0x4A, 0x00, 0x00, 0x00, 0x00 };

	debug("sd_getcid(buf):\r\n");
	return sd__read(cmd10, cid, 16);
}

uint8_t
sd_getblocks(uint32_t *blocks)
{
	uint8_t csd[16];
	uint8_t ret;
	unsigned int read_bl_len;
	unsigned int c_size;
	unsigned int c_size_mult;

	ret = sd_getcsd(csd);
	debug("sd_getcsd() = %u\r\n", ret);
	if (ret != 0x00)
		return ret;

	switch (csd[0] >> 6) {
	case 0:
		read_bl_len = csd[5] & 0x0F;
		debug("read_bl_len = %u\r\n", read_bl_len);

		c_size = csd[6] & 0x03;
		c_size <<= 8;
		c_size |= csd[7];
		c_size <<= 8;
		c_size |= csd[8];
		c_size >>= 6;
		debug("c_size = %u\r\n", c_size);

		c_size_mult = csd[9] & 0x03;
		c_size_mult <<= 8;
		c_size_mult |= csd[10];
		c_size_mult >>= 7;
		debug("c_size_mult = %u\r\n", c_size_mult);

		c_size += 1;
		c_size <<= c_size_mult + 2;
		debug("blocknr = %u\r\n", c_size);

		c_size <<= read_bl_len;
		c_size >>= 9;
		debug("blocks = %u\r\n", c_size);
		*blocks = c_size;
		break;
	case 1:
		c_size = csd[7] & 0x3F;
		c_size <<= 8;
		c_size |= csd[8];
		c_size <<= 8;
		c_size |= csd[9];
		debug("c_size = %u\r\n", c_size);

		c_size += 1;
		c_size <<= 10;
		debug("blocks = %u\r\n", c_size);
		*blocks = c_size;
		break;
	default:
		ret = 0x80;
		break;
	}
	return ret;
}

uint8_t
sd_readblock(uint32_t lba, uint8_t buf[512])
{
	uint8_t cmd17[5];

	debug("sd_readblock(%lu, buf):\r\n", lba);

	if (!block_addressing)
		lba <<= 9;

	cmd17[4] = lba & 0xFF; lba >>= 8;
	cmd17[3] = lba & 0xFF; lba >>= 8;
	cmd17[2] = lba & 0xFF; lba >>= 8;
	cmd17[1] = lba & 0xFF;
	cmd17[0] = 0x51;

	return sd__read(cmd17, buf, 512);
}

uint8_t
sd_writeblock(uint32_t lba, const uint8_t buf[512])
{
	unsigned int i;
	uint8_t cmd24[6];
	uint8_t ret;

	debug("sd_writeblock(%lu, buf):\r\n", lba);

	if (!block_addressing)
		lba <<= 9;

	cmd24[5] = 0xFF;
	cmd24[4] = lba & 0xFF; lba >>= 8;
	cmd24[3] = lba & 0xFF; lba >>= 8;
	cmd24[2] = lba & 0xFF; lba >>= 8;
	cmd24[1] = lba & 0xFF;
	cmd24[0] = 0x58;

	gpio_clear(SD_CS);

	ret = sd__cmd(cmd24, NULL, 0);
	if (ret != 0x00) {
		debug("cmd24: %02x\r\n", ret);
		goto out;
	}

	usart0_txdata(0xFF);
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdata(0xFE);
	for (i = 0; i < 512; i++) {
		while (!usart0_tx_buffer_level())
			/* wait */;
		usart0_txdata(buf[i]);
	}
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdata(0xFF);
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdatax(USART_TXDATAX_RXENAT | 0xFF);
	while (!usart0_tx_buffer_level())
		/* wait */;
	usart0_txdata(0xFF);
	ret = sd__getbyte();
	ret |= 0xE0;
	if (ret != 0xE5) {
		debug("data response: %02x\r\n", ret);
		goto out;
	}
	do {
		//debug(".");
		usart0_txdata(0xFF);
		ret = sd__getbyte();
	} while (ret != 0xFF);
	ret = 0x00;
out:
	usart0_rx_disable();
	while (!usart0_tx_complete())
		/* wait */;
	gpio_set(SD_CS);
	return ret;
}
