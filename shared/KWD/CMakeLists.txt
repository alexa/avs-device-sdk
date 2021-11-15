cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_subdirectory("acsdkKWD")
add_subdirectory("acsdkKWDImplementations")
add_subdirectory("acsdkKWDInterfaces")

if(KWD)
    add_subdirectory("acsdkKWDProvider")
endif()
