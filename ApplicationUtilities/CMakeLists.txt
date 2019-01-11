cmake_minimum_required(VERSION 3.1)
project(ApplicationUtilities VERSION ${PROJECT_VERSION} LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("DefaultClient")
add_subdirectory("Resources")

if (ANDROID)
    add_subdirectory("AndroidUtilities")
endif()
