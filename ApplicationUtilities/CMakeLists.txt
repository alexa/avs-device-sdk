cmake_minimum_required(VERSION 3.1)
project(ApplicationUtilities LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("DefaultClient")
add_subdirectory("Resources")
add_subdirectory("SystemSoundPlayer")
add_subdirectory("SDKComponent")

if (ANDROID)
    add_subdirectory("AndroidUtilities")
endif()
