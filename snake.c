/*
 * This file is part of badge2019.
 * Copyright 2019 Niels Kristensen <niels@kristensen.io>
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

#include "leds.h"
#include "timer.h"
#include "events.h"
#include "buttons.h"
#include "display.h"
#include "power.h"

enum events {
  EV_UP = 1,
  EV_DOWN,
  EV_LEFT,
  EV_RIGHT,
};

static const struct button_config snake_buttons[BTN_MAX] = {
  [BTN_UP]     = { .press   = EV_UP, },
  [BTN_DOWN]   = { .press   = EV_DOWN, },
  [BTN_LEFT]   = { .press   = EV_LEFT, },
  [BTN_RIGHT]  = { .press   = EV_RIGHT, },
};

struct pos {
  char x,
       y;
};

static struct pos getRandomPos(){
  short x, y;
  struct pos point;

  x = rand() % 44 * 5 + 10;
  y = rand() % 44 * 5 + 10;

  point.x = x;
  point.y = y;
  return point;
}

static void lost(unsigned short len){
  short xpos, printchar;

  dp_fill(0, 0, 240, 240, 0x000);
  dp_puts(30, 20, 0x00F, 0x00, "Your score:");
  xpos = 220;

  if (xpos == 220 && len == 0) {
    dp_putchar(xpos, 50, 0xCB0, 0x000, 48);
  } else {	
    while (len != 0) {
      printchar = len % 10 + 48;
      dp_putchar(xpos, 50, 0xCB0, 0x000, printchar);
      len /= 10;
      xpos -= 15;
    }
  }
  timer_msleep(5000);
}

void
snake(void)
{
  struct pos point;
  struct pos path[256];

  unsigned short i = 0, k = 0, last = 0, skip = 0, len = 5;

  // clear and print border
  dp_fill(0, 0, 240, 240, 0x000);
  dp_fill(5, 5, 5, 235, 0xCB0);
  dp_fill(235, 5, 5, 235, 0xCB0);
  dp_fill(10, 5, 230, 5, 0xCB0);
  dp_fill(10, 235, 230, 5, 0xCB0);

  buttons_config(snake_buttons);

  // start direction
  char direction = 'd';

  // start position
  unsigned short currentX = 30,
           currentY = 30;

  // first point
  point.x = 200;
  point.y = 200;
  dp_fill(point.x, point.y, 5, 5, 0xF00);

  // '4' Guaranteed to be randomly -- chosen by fair dice
  srand(4);

  i = 0;
  while (1) {

  switch ((enum events)event_get()) {
    case EV_UP:
      if (direction != 'd')
        direction = 'u';
      break;
    case EV_DOWN:
      if (direction != 'u')
        direction = 'd';
      break;
    case EV_LEFT:
      if (direction != 'r')
        direction = 'l';
      break;
    case EV_RIGHT:
      if (direction != 'l')
        direction = 'r';
      break;
    }

    if (direction == 'd'){
      currentY += 5;
    } else if (direction == 'u'){
      currentY -= 5;
    } else if (direction == 'l'){
      currentX -= 5;
    } else if (direction == 'r'){
      currentX += 5;
    }

    // Did we hit ourself?
    unsigned int a, b, c;
    if (i > 1) {
      b = i - 2;
      c = i - len - 1;
      for(a = c; a <= b; a++)
      {
        if(currentX == path[a].x && currentY == path[a].y){
          lost(len - 5);
          return;
        }
      }
    }

    dp_fill(currentX, currentY, 5, 5, 0xCB0);

    path[i].x = currentX;
    path[i].y = currentY;
    i = (i + 1) % ARRAY_SIZE(path);

    if (k > 5 && !skip) {
      dp_fill(path[last].x, path[last].y, 5, 5, 0x000);
      last = (last + 1) % ARRAY_SIZE(path);
    } else {
      skip = 0;
      k++;
    }

    // Did we eat a point?
    if (currentX == point.x && currentY == point.y){
      skip = 1;
      len++;
      // generate new point
      point = getRandomPos();
    }

    dp_fill(point.x, point.y, 5, 5, 0xF00);

    // Did we hit the wall?
    if (currentY == 235 || currentY == 5 || currentX == 5 || currentX == 235){
      lost(len - 5);
      return;
    }

    timer_msleep(100);
  }
}
