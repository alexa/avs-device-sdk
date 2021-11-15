cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(SENSORY LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

set(TARGET_KWD_LIB "SENSORY" PARENT_SCOPE)
set(KWD_ADAPTER_REGISTRATION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/acsdkKWDProvider/src/SensoryRegistration.cpp" PARENT_SCOPE)
set(KWD_COMPONENT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/acsdkKWD/src/KWDComponent.cpp" PARENT_SCOPE)

add_subdirectory("src")
add_subdirectory("test")
