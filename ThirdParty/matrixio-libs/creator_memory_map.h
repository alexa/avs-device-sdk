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

#ifndef CPP_DRIVER_CREATOR_MEMORY_MAP_H_
#define CPP_DRIVER_CREATOR_MEMORY_MAP_H_

#include <string>

namespace matrix_hal {

/* FPGA Wishbone address map */
const uint32_t kConfBaseAddress = 0x0000;
const uint32_t kUartBaseAddress = 0x0800;
const uint32_t kMicrophoneArrayBaseAddress = 0x1800;
const uint32_t kEverloopBaseAddress = 0x2000;
const uint32_t kGPIOBaseAddress = 0x2800;
const uint16_t kMCUBaseAddress = 0x3800;

/* MCU offsets map */
const uint16_t kMemoryOffsetUV = 0x00;
const uint16_t kMemoryOffsetPressure = 0x10;
const uint16_t kMemoryOffsetHumidity = 0x20;
const uint16_t kMemoryOffsetIMU = 0x30;
const uint16_t kMemoryOffsetMCU = 0x90;

};      // namespace matrix_hal
#endif  // CPP_DRIVER_CREATOR_MEMORY_MAP_H_
