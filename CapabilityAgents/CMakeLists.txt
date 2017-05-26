cmake_minimum_required(VERSION 3.0)
project(CapabilityAgents LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("AIP")
add_subdirectory("SpeechSynthesizer")
