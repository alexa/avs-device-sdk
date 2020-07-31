cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(SpeechEncoder LANGUAGES CXX)

add_subdirectory("src")
add_subdirectory("test")

if(OPUS)
	message(STATUS "OPUS ENCODER ENABLED")
	add_subdirectory("OpusEncoderContext")
endif()
