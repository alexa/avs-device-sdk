/*
 * Copyright 2016 <Admobilize>
 * MATRIX Labs  [http://creator.matrix.one]
 * This file is part of MATRIX Creator HAL
 *
 * MATRIX Creator HAL is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <string>
#include <iostream>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "everloop.h"

namespace matrix_hal {

Everloop::Everloop() {}

bool Everloop::Write(const EverloopImage* led_image) {

  int fd = open("/dev/matrixio_everloop", O_WRONLY);

  char buff[4*35];
  uint32_t addr_offset = 0;
  for (const LedValue& led : led_image->leds) {

    buff[addr_offset] = led.red;
    buff[addr_offset+1] = led.green;
    buff[addr_offset+2] = led.white;
    buff[addr_offset+3] = led.blue;
    addr_offset +=4;
  }

  write(fd, buff, 35*4);
  close(fd);

  return true;
}
};  // namespace matrix_hal
