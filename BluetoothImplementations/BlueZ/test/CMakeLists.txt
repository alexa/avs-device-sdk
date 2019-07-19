cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${BluetoothImplementationsBlueZ_INCLUDE_DIRS}"
    "${BluetoothImplementationsBlueZ_SOURCE_DIR}/test/Mock/include"
    "${GIO_INCLUDE_DIRS}"
    "${GIO_UNIX_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" BluetoothImplementationsBlueZ)
