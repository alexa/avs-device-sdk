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
#include <iostream>
#include <string>

#include "cpp/driver/creator_memory_map.h"
#include "cpp/driver/everloop.h"

namespace matrix_hal {

Everloop::Everloop() {}

bool Everloop::Write(const EverloopImage* led_image) {
  if (!wishbone_) return false;

  uint16_t wb_data_buffer;
  char* data_buffer = reinterpret_cast<char*>(&wb_data_buffer);

  uint32_t addr_offset = 0;
  for (const LedValue& led : led_image->leds) {
    data_buffer[0] = led.green;
    data_buffer[1] = led.red;
    wishbone_->SpiWrite16(kEverloopBaseAddress + addr_offset, wb_data_buffer);

    data_buffer[0] = led.blue;
    data_buffer[1] = led.white;
    wishbone_->SpiWrite16(kEverloopBaseAddress + addr_offset + 1,
                          wb_data_buffer);

    addr_offset = addr_offset + 2;
  }
  return true;
}
};  // namespace matrix_hal
