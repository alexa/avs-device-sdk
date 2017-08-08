cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(PlaylistParser LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("src")

if(NOT TOTEM_PLPARSER)
    message("Totem-Pl-Parser based playlist parser will not be built, and the playlist parser is disabled.")
else()
    add_subdirectory("test")
endif()