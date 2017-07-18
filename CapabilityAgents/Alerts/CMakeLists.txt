cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(Alerts LANGUAGES CXX)

include(../../build/BuildDefaults.cmake)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
find_package(PkgConfig)
pkg_check_modules(SQLITE REQUIRED sqlite3)

add_subdirectory("src")
