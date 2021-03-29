cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(acsdkMultiRoomMusic LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")

if(MRM AND MRM_STANDALONE_APP)
    add_subdirectory("MRMHandlerProxy")
    add_subdirectory("MRMApp")
elseif (MRM)
    add_subdirectory("MRMHandler")
endif()