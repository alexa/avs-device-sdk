cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(AlexaPlaybackController LANGUAGES CXX)

# Define explicit short names for binary directories to shorten full path
# on Windows platform. When building for UWP some of generated artifacts
# get full path over 260 characters and this breaks the build.
add_subdirectory("acsdkAlexaPlaybackController" "APC")
add_subdirectory("acsdkAlexaPlaybackControllerInterfaces" "APCI")

