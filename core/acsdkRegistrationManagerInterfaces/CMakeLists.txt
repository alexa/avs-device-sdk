cmake_minimum_required(VERSION 3.1)

project(RegistrationManagerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(RegistrationManagerInterfaces INTERFACE)

target_include_directories(RegistrationManagerInterfaces INTERFACE
        "${RegistrationManagerInterfaces_SOURCE_DIR}/include")

target_link_libraries(RegistrationManagerInterfaces INTERFACE
        acsdkNotifierInterfaces)

add_subdirectory("test")

# install interface
asdk_install_interface()
