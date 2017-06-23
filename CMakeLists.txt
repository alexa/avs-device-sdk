cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Set project information
project(AlexaClientSDK VERSION 0.5.0 LANGUAGES CXX)
set(PROJECT_BRIEF "A cross-platform, modular SDK for interacting with the Alexa Voice Service")

include(build/BuildDefaults.cmake)
include(tools/Testing.cmake)

# Alexa Client SDK targets.
add_subdirectory("ThirdParty")
add_subdirectory("AVSCommon")
add_subdirectory("ACL")
add_subdirectory("AuthDelegate")
add_subdirectory("ADSL")
add_subdirectory("AFML")
add_subdirectory("ContextManager")
add_subdirectory("MediaPlayer")
add_subdirectory("KWD")
add_subdirectory("CapabilityAgents")
add_subdirectory("Integration")
add_subdirectory("doc")
