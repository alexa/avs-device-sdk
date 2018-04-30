cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${SQLiteStorage_SOURCE_DIR}/include")

discover_unit_tests("${INCLUDE_PATH}" SQLiteStorage ".")

