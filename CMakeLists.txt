cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(AlexaClientSDK LANGUAGES CXX)

include(build/BuildDefaults.cmake)
include(tools/Testing.cmake)

# Alexa Client SDK targets.
add_subdirectory("AuthDelegate")
add_subdirectory("ACL")
add_subdirectory("Integration")
add_subdirectory("AVSUtils")
