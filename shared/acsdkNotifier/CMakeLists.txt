cmake_minimum_required(VERSION 3.1)

project(acsdkNotifier LANGUAGES CXX)

add_library(acsdkNotifier INTERFACE)

target_include_directories(acsdkNotifier INTERFACE
    "${acsdkNotifier_SOURCE_DIR}/include"
)

target_link_libraries(acsdkNotifier INTERFACE acsdkNotifierInterfaces)

# install interface
asdk_install_interface()

add_subdirectory("test")