cmake_minimum_required(VERSION 3.1)

project(acsdkNotifier LANGUAGES CXX)
file(GLOB_RECURSE acsdkNotifier_SOURCES "src/*.cpp")
add_library(acsdkNotifier ${acsdkNotifier_SOURCES})
target_include_directories(acsdkNotifier
        PUBLIC "${acsdkNotifier_SOURCE_DIR}/include"
        PRIVATE "${acsdkNotifier_SOURCE_DIR}/privateInclude")
target_compile_definitions(acsdkNotifier PRIVATE ACSDK_LOG_MODULE=acsdkNotifier)
target_link_libraries(acsdkNotifier PUBLIC acsdkNotifierInterfaces)
target_link_libraries(acsdkNotifier PRIVATE AVSCommon)

# install library
asdk_install()

add_subdirectory("test")
