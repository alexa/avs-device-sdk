cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(IPCServerSampleApplication LANGUAGES CXX)

# enable testing - needs to be included from this top-level directory
include(tools/Testing.cmake)

set(ASDK_LIBRARY_DIRS "${CMAKE_PREFIX_PATH}/lib")
set(ASDK_INCLUDE_DIRS "${ASDK_INCLUDE_DIRS}" "${CMAKE_PREFIX_PATH}/include")
set(ASDK_LDFLAGS "${ASDK_LDFLAGS}" "-L${CMAKE_PREFIX_PATH}/lib")

get_filename_component(AVS_CMAKE_BUILD ../../cmakeBuild ABSOLUTE)

add_subdirectory(Communication)
add_subdirectory(LibIPCServerSampleApp)

if (METRICS_EXTENSION)
    add_subdirectory("Metrics")
endif ()



include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)
include(${AVS_CMAKE_BUILD}/cmake/PrepareInstall.cmake)

add_subdirectory(src)