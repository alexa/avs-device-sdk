cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkAlexaLiveViewController LANGUAGES CXX)
 
if (RTCSC)
    add_subdirectory("AlexaLiveViewController")
    add_subdirectory("AlexaLiveViewControllerInterfaces")
endif()
