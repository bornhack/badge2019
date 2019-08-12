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
#include <stdint.h>
#include <stdbool.h>

#include "geckonator/clock.h"
#include "geckonator/gpio.h"
#include "geckonator/usart1.h"

#include "ff.h"
#include "font.h"
#include "display.h"

#define DP_BLK GPIO_PA1 /* backlight */
#define DP_DC  GPIO_PA2 /* D/CX */
#define DP_SDA GPIO_PC1 /* data */
#define DP_RES GPIO_PC2 /* reset */
#define DP_SCL GPIO_PC3 /* clock */

/* spi max write clock rate 1/(66ns) = 15 MHz
 * spi max read clock rate 1/(150ns) = 6.6 MHz
 */
#define DP_CLOCKDIV_WRITE   0 /* 24MHz / (2 * (1 +   0/256)) = 12MHz */
#define DP_CLOCKDIV_READ  256 /* 24MHz / (2 * (1 + 256/256)) =  6MHz */

#if 0
#include <stdio.h>
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

void dp_backlight_on(void)
{
	gpio_set(DP_BLK);
}

void dp_backlight_off(void)
{
	gpio_clear(DP_BLK);
}

void dp_backlight_toggle(void)
{
	gpio_toggle(DP_BLK);
}

void
dp_reset(void)
{
	unsigned int i;

	for (i = 240; i; i--)
		__NOP();
	gpio_clear(DP_RES);
	for (i = 240; i; i--)
		__NOP();
	gpio_set(DP_RES);
	for (i = 240; i; i--)
		__NOP();
}

uint8_t
dp_read1(uint8_t cmd)
{
	usart1_clock_div(DP_CLOCKDIV_READ);
	usart1_txdatax(cmd
			| USART_TXDATAX_RXENAT
			| USART_TXDATAX_TXTRIAT);
	gpio_toggle(DP_DC);
	usart1_txdata(0x00);
	while (!usart1_rx_valid())
		/* wait */;
	usart1_rx_disable();
	usart1_tx_tristate_disable();
	gpio_toggle(DP_DC);
	usart1_clock_div(DP_CLOCKDIV_WRITE);
	return usart1_rxdata();
}

void
dp_read(uint8_t cmd, uint8_t *buf, size_t len)
{
	usart1_clock_div(DP_CLOCKDIV_READ);
	usart1_frame_bits(9);
	usart1_txdatax((((uint16_t)cmd) << 1)
			| USART_TXDATAX_RXENAT
			| USART_TXDATAX_TXTRIAT);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);
	usart1_frame_bits(8);
	usart1_txdata(0x00);
	while (--len) {
		usart1_txdata(0x00);
		while (!usart1_rx_valid())
			/* wait */;
		*buf++ = usart1_rxdata();
	}
	while (!usart1_rx_valid())
		/* wait */;
	usart1_rx_disable();
	usart1_tx_tristate_disable();
	usart1_clock_div(DP_CLOCKDIV_WRITE);
	*buf++ = usart1_rxdata();
}

void
dp_write1(uint8_t cmd)
{
	usart1_txdata(cmd);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);
}

void
dp_write(uint8_t cmd, const uint8_t *buf, size_t len)
{
	const uint8_t *end = buf + len;

	usart1_txdata(cmd);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);
	while (1) {
		usart1_txdata(*buf++);
		if (buf >= end)
			break;
		while (!usart1_tx_buffer_level())
			/* wait */;
	}
	while (!usart1_tx_complete())
		/* wait */;
}

void
dp_sleep_in(void)
{
	dp_write1(0x10);
}

void
dp_sleep_out(void)
{
	dp_write1(0x11);
}

void
dp_off(void)
{
	dp_write1(0x28);
}

void
dp_on(void)
{
	dp_write1(0x29);
}

static void
dp_mode444(void)
{
	const uint8_t v = 0x03;
	dp_write(0x3a, &v, 1);
}

static void
dp_mode565(void)
{
	const uint8_t v = 0x05;
	dp_write(0x3a, &v, 1);
}

static void
dp_mode666(void)
{
	const uint8_t v = 0x06;
	dp_write(0x3a, &v, 1);
}

void
dp_rotate(bool rotate)
{
	if (rotate) {
		{
			/* Memory Data Access Control MY=1 MX=1 */
			const uint8_t v = 0xC0;
			dp_write(0x36, &v, 1);
		}
		{
			/* Vertial Scroll Start Address of RAM: 80 */
			const uint8_t data[] = { 0, 80 };
			dp_write(0x37, data, ARRAY_SIZE(data));
		}
	} else {
		{
			/* Memory Data Access Control MY=0 MX=0 */
			const uint8_t v = 0x00;
			dp_write(0x36, &v, 1);
		}
		{
			/* Vertial Scroll Start Address of RAM: 0 */
			const uint8_t data[] = { 0, 0 };
			dp_write(0x37, data, ARRAY_SIZE(data));
		}
	}
}

void
dp_init(void)
{
	unsigned int i;

	gpio_clear(DP_BLK);
	gpio_clear(DP_RES);
	gpio_set(DP_DC);
	gpio_set(DP_SCL);
	gpio_clear(DP_SDA);
	gpio_mode(DP_BLK, GPIO_MODE_PUSHPULL);
	gpio_mode(DP_RES, GPIO_MODE_PUSHPULL);
	gpio_mode(DP_DC,  GPIO_MODE_PUSHPULL);
	gpio_mode(DP_SCL, GPIO_MODE_PUSHPULL);
	gpio_mode(DP_SDA, GPIO_MODE_PUSHPULL);

	clock_usart1_enable();
	/* location5:
	 * clk -> PC3 -> DP_SCL (data latched on rising edge)
	 * cs  -> PC0 (not connected)
	 * rx  \  with USART_CTRL_LOOPBK enabled
	 * tx  -> PC1 -> DP_SDA
	 */
	usart1_config(USART_CTRL_MSBF
			| USART_CTRL_CLKPHA
			| USART_CTRL_CLKPOL
			| USART_CTRL_LOOPBK
			| USART_CTRL_SYNC);
	usart1_clock_div(DP_CLOCKDIV_WRITE);
	usart1_frame_bits(8);
	usart1_master_enable();
	usart1_tx_enable();
	usart1_pins(USART_ROUTE_LOCATION_LOC5
			| USART_ROUTE_CLKPEN
			| USART_ROUTE_TXPEN);

	/* release reset */
	for (i = 240; i; i--)
		__NOP();
	gpio_set(DP_RES);
	for (i = 240; i; i--)
		__NOP();

	dp_sleep_out();
	{ /* Partial Area from line 0 to 240 */
		const uint8_t data[] = { 0, 0, 0, 240 };

		dp_write(0x30, data, ARRAY_SIZE(data));
	}
	dp_write1(0x12); /* Partial Display Mode On */
	dp_write1(0x21); /* Inverse On */
	dp_on();
	dp_mode444();
	dp_rotate(true);
}

void
dp_uninit(void)
{
	usart1_rxtx_disable();
	usart1_pins(0);

	dp_backlight_off();
	gpio_mode(DP_RES, GPIO_MODE_DISABLED);
	gpio_mode(DP_DC,  GPIO_MODE_DISABLED);
	gpio_mode(DP_SCL, GPIO_MODE_DISABLED);
	gpio_mode(DP_SDA, GPIO_MODE_DISABLED);
}

static void
dp__setbox(unsigned int xs, unsigned int xe, unsigned int ys, unsigned int ye)
{
	uint8_t buf[4];

	buf[0] = (xs >> 8) & 0xff;
	buf[1] = xs & 0xff;
	buf[2] = (xe >> 8) & 0xff;
	buf[3] = xe & 0xff;
	dp_write(0x2a, buf, 4);

	buf[0] = (ys >> 8) & 0xff;
	buf[1] = ys & 0xff;
	buf[2] = (ye >> 8) & 0xff;
	buf[3] = ye & 0xff;
	dp_write(0x2b, buf, 4);
}

void
dp_fill(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int rgb444)
{
	uint8_t buf[3] = {
		(rgb444 >> 4) & 0xff,
		((rgb444 << 4) & 0xff) | ((rgb444 >> 8) & 0xff),
		rgb444 & 0xff,
	};
	unsigned int i;

	dp__setbox(x, x+w-1, y, y+h-1);

	usart1_txdata(0x2c);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);
	for (i = (w * h + 1)/2; i; i--) {
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(buf[0]);
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(buf[1]);
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(buf[2]);
	}
	while (!usart1_tx_complete())
		/* wait */;
}

void
dp_fill666(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int rgb)
{
	unsigned int i;

	dp_mode666();
	dp__setbox(x, x+w-1, y, y+h-1);

	usart1_txdata(0x2c);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);
	for (i = w * h; i; i--) {
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata((rgb >> 16) & 0xff);
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata((rgb >> 8) & 0xff);
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(rgb & 0xff);
	}
	while (!usart1_tx_complete())
		/* wait */;
	dp_mode444();
}

void
dp_putchar(unsigned int x, unsigned int y, unsigned int fg444, unsigned int bg444, int ch)
{
	unsigned int idx = 0;
	uint8_t mask;
	unsigned int i;

	if (ch >= 32 && ch <= 126)
		idx = ch - 31;

	idx *= font.width * font.height;
	mask = 1U << (idx % 8);
	idx /= 8;

	dp__setbox(x, x+font.width-1, y, y+font.height-1);

	usart1_txdata(0x2c);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);

	for (i = (font.width * font.height + 1)/2; i; i--) {
		unsigned int p1, p2;
		uint8_t v1, v2, v3;

		if (font.data[idx] & mask)
			p1 = fg444;
		else
			p1 = bg444;
		mask <<= 1;
		if (mask == 0) {
			idx += 1;
			mask = 1;
		}
		v1 = p1 >> 4;
		v2 = p1 << 4;

		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(v1);

		if (font.data[idx] & mask)
			p2 = fg444;
		else
			p2 = bg444;
		mask <<= 1;
		if (mask == 0) {
			idx += 1;
			mask = 1;
		}
		v2 |= p2 >> 8;
		v3 = p2;

		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(v2);
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(v3);
	}
	while (!usart1_tx_complete())
		/* wait */;
}

void
dp_puts(unsigned int x, unsigned int y, unsigned int fg444, unsigned int bg444, const char *str)
{
	while (1) {
		int ch = *str++;

		if (ch == '\0')
			break;
		if ((x + font.width) > 240)
			break;
		dp_putchar(x, y, fg444, bg444, ch);
		x += font.width;
	}
}

void
dp_image565(unsigned int x, unsigned int y, const struct dp_image565 *img)
{
	dp_mode565();
	dp__setbox(x, x + img->width - 1, y, y + img->height - 1);

	usart1_txdata(0x2c);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);

	{
		const uint8_t *p, *end = &img->data[2 * img->width * img->height];

		for (p = &img->data[0]; p < end; p += 2) {
			while (!usart1_tx_buffer_level())
				/* wait */;
			usart1_txdata(p[1]);

			while (!usart1_tx_buffer_level())
				/* wait */;
			usart1_txdata(p[0]);
		}
	}
	while (!usart1_tx_complete())
		/* wait */;
	dp_mode444();
}

struct dp_bitstream {
	const dp_bitstream_data_t *p;
	dp_bitstream_data_t mask;
};

static void
dp_bitstream_init(struct dp_bitstream *bs, const dp_bitstream_data_t *p)
{
	bs->p = p;
	bs->mask = ((dp_bitstream_data_t)1) << (8*sizeof(dp_bitstream_data_t) - 1);
}

static dp_bitstream_data_t
dp_bitstream_pop(struct dp_bitstream *bs)
{
	dp_bitstream_data_t ret = *bs->p & bs->mask;
	bs->mask >>= 1;
	if (bs->mask == 0)
		dp_bitstream_init(bs, bs->p + 1);

	return ret;
}

static unsigned int
dp_bitstream_get(struct dp_bitstream *bs)
{
	unsigned int ret;

	if (!dp_bitstream_pop(bs))
		return 0;

	ret = 1;
	while (dp_bitstream_pop(bs)) {
		ret <<= 1;
		if (dp_bitstream_pop(bs))
			ret |= 1;
	}

	return ret;
}

static int
dp_bitstream_gets(struct dp_bitstream *bs)
{
	int ret = 0;

	while (dp_bitstream_pop(bs))
		ret++;

	if (dp_bitstream_pop(bs))
		return -ret;

	return ret + 1;
}

void
dp_cimage(unsigned int x, unsigned int y, const struct dp_cimage *img)
{
	struct dp_bitstream bs;
	unsigned int len = ((unsigned int)img->width) * ((unsigned int)img->height);
	unsigned int run = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	dp_bitstream_init(&bs, img->data);

	dp_mode666();
	dp__setbox(x, x + img->width - 1, y, y + img->height - 1);

	usart1_txdata(0x2c);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);

	for (; len; len--) {
		if (run == 0)
			r = ((int)r) + 4*dp_bitstream_gets(&bs);
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(r);

		if (run == 0)
			g = ((int)g) + 4*dp_bitstream_gets(&bs);
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(g);

		if (run == 0) {
			b = ((int)b) + 4*dp_bitstream_gets(&bs);
			run = dp_bitstream_get(&bs);
		} else
			run--;
		while (!usart1_tx_buffer_level())
			/* wait */;
		usart1_txdata(b);
	}
	while (!usart1_tx_complete())
		/* wait */;
	dp_mode444();
}

FRESULT
dp_showbmp(FIL *f, unsigned int x, unsigned int y)
{
	uint8_t buf[720];
	unsigned int bfSize;
	unsigned int bfOffBits;
	unsigned int biSize;
	unsigned int biWidth;
	int biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	unsigned int biCompression;
	unsigned int read;
	int linesize;
	int pos;
	FRESULT res;

	res = f_read(f, buf, 18, &read);
	if (res != FR_OK) {
		debug("f_read(f, 18) = %u\r\n", res);
		return res;
	}
	if (read < 18) {
		debug("f_read(f, 18): short read\r\n");
		return FR_INVALID_PARAMETER;
	}

	if (buf[0] != 'B' || buf[1] != 'M') {
		debug("error reading bitmap: not a BMP file\r\n");
		return FR_INVALID_PARAMETER;
	}

	bfSize = (((unsigned int)buf[2]) <<  0)
	       | (((unsigned int)buf[3]) <<  8)
	       | (((unsigned int)buf[4]) << 16)
	       | (((unsigned int)buf[5]) << 24);

	debug("bfSize = %u\r\n", bfSize);

	bfOffBits = (((unsigned int)buf[10]) <<  0)
	          | (((unsigned int)buf[11]) <<  8)
	          | (((unsigned int)buf[12]) << 16)
	          | (((unsigned int)buf[13]) << 24);

	debug("bfOffBits = %u\r\n", bfOffBits);

	biSize = (((unsigned int)buf[14]) <<  0)
	       | (((unsigned int)buf[15]) <<  8)
	       | (((unsigned int)buf[16]) << 16)
	       | (((unsigned int)buf[17]) << 24);
	debug("biSize = %u\r\n", biSize);
	if (biSize < 40) {
		debug("error reading bitmap: old OS/2 header not supported\r\n");
		return FR_INVALID_PARAMETER;
	}
	if (12 + biSize >= bfOffBits) {
		debug("error reading bitmap: bfOffBits points into header\r\n");
		return FR_INVALID_PARAMETER;
	}

	res = f_read(f, buf, 16, &read);
	if (res != FR_OK) {
		debug("f_read(f, 16) = %u\r\n", res);
		return res;
	}
	if (read < 16) {
		debug("f_read(f, 16): short read\r\n");
		return FR_INVALID_PARAMETER;
	}

	biWidth = (((unsigned int)buf[0]) <<  0)
	        | (((unsigned int)buf[1]) <<  8)
	        | (((unsigned int)buf[2]) << 16)
	        | (((unsigned int)buf[3]) << 24);
	biHeight = (((unsigned int)buf[4]) <<  0)
	         | (((unsigned int)buf[5]) <<  8)
	         | (((unsigned int)buf[6]) << 16)
	         | (((unsigned int)buf[7]) << 24);
	debug("biWidth x biHeight = %u x %d\r\n", biWidth, biHeight);

	biPlanes = (((uint16_t)buf[8]) <<  0)
	         | (((uint16_t)buf[9]) <<  8);
	if (biPlanes != 1) {
		debug("error reading bitmap: biPlanes != 1\r\n");
		return FR_INVALID_PARAMETER;
	}

	biBitCount = (((uint16_t)buf[10]) <<  0)
	           | (((uint16_t)buf[11]) <<  8);
	debug("biBitCount = %u\r\n", biBitCount);
	if (biBitCount != 24) {
		debug("error reading bitmap: only 24bit bitmaps supported\r\n");
		return FR_INVALID_PARAMETER;
	}

	biCompression = (((unsigned int)buf[12]) <<  0)
	              | (((unsigned int)buf[13]) <<  8)
	              | (((unsigned int)buf[14]) << 16)
	              | (((unsigned int)buf[15]) << 24);
	debug("biCompression = %u\r\n", biCompression);
	if (biCompression != 0) {
		debug("error reading bitmap: only uncompressed bitmaps supported\r\n");
		return FR_INVALID_PARAMETER;
	}

	linesize = 3 * biWidth;
	if (linesize & 0x3U) {
		linesize &= ~0x3U;
		linesize += 4;
	}
	if (biHeight >= 0) {
		read = bfOffBits + linesize * biHeight;
		pos = read - linesize;
		linesize = -linesize;
	} else {
		biHeight = -biHeight;
		read = bfOffBits + linesize * biHeight;
		pos = bfOffBits;
	}
	if (read > bfSize) {
		debug("error reading bitmap: file too short\r\n");
		return FR_INVALID_PARAMETER;
	}

	dp_mode666();
	dp__setbox(x, x + biWidth - 1, y, y + biHeight - 1);

	usart1_txdata(0x2c);
	gpio_toggle(DP_DC);
	while (!usart1_tx_complete())
		/* wait */;
	gpio_toggle(DP_DC);

	for (; biHeight > 0; biHeight--) {
		unsigned int bytes = 3 * biWidth;
		uint8_t *p, *end;

		res = f_lseek(f, pos);
		if (res != FR_OK) {
			debug("f_lseek(f, %d) = %u\r\n", pos, res);
			goto out;
		}

		res = f_read(f, buf, bytes, &read);
		if (res != FR_OK) {
			debug("f_read(f, %u) = %u\r\n", bytes, res);
			goto out;
		}
		if (read < bytes) {
			debug("f_read(f, %u): short read\r\n", bytes);
			res = FR_INVALID_PARAMETER;
			goto out;
		}

		p = buf;
		end = buf + bytes;
		for (; p < end; p += 3) {
			while (!usart1_tx_buffer_level())
				/* wait */;
			usart1_txdata(p[2]);
			while (!usart1_tx_buffer_level())
				/* wait */;
			usart1_txdata(p[1]);
			while (!usart1_tx_buffer_level())
				/* wait */;
			usart1_txdata(p[0]);
		}
		pos += linesize;
	}
out:
	while (!usart1_tx_complete())
		/* wait */;
	dp_mode444();
	return res;
}

FRESULT
dp_showbmp_at(const char *path, unsigned int x, unsigned int y)
{
	FIL f;
	FRESULT res;

	res = f_open(&f, path, FA_READ);
	if (res != FR_OK) {
		debug("f_open(f, \"%s\", FA_READ) = %u\r\n", path, res);
		goto err;
	}
	res = dp_showbmp(&f, x, y);
	if (res != FR_OK)
		goto err;

	return f_close(&f);
err:
	f_close(&f);
	return res;
}
