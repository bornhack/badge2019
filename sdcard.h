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

#ifndef _SDCARD_H
#define _SDCARD_H

#include <stdint.h>

void sd_init(void);
void sd_uninit(void);
uint8_t sd_cmd(const uint8_t cmd[6], uint8_t *response, unsigned int len);
uint8_t sd_wakeup(void);
uint8_t sd_status(uint8_t *status);
uint8_t sd_getcsd(uint8_t csd[16]);
uint8_t sd_getcid(uint8_t cid[16]);
uint8_t sd_getblocks(uint32_t *blocks);
uint8_t sd_readblock(uint32_t lba, uint8_t buf[512]);
uint8_t sd_writeblock(uint32_t lba, const uint8_t buf[512]);

#endif
