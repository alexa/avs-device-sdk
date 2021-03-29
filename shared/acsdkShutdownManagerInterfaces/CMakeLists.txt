cmake_minimum_required(VERSION 3.1)

project(acsdkShutdownManagerInterfaces LANGUAGES CXX)

add_library(acsdkShutdownManagerInterfaces INTERFACE)

target_include_directories(acsdkShutdownManagerInterfaces INTERFACE
    "${acsdkShutdownManagerInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkShutdownManagerInterfaces INTERFACE
    acsdkNotifierInterfaces AVSCommon)

add_subdirectory("test")

# install interface
asdk_install_interface()
