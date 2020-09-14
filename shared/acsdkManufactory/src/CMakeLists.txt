cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
add_definitions("-DACSDK_LOG_MODULE=Manufactory")

add_library(acsdkManufactory SHARED
    CookBook.cpp)

target_link_libraries(acsdkManufactory AVSCommon)

target_include_directories(acsdkManufactory PUBLIC "${acsdkManufactory_SOURCE_DIR}/include")

# install target
asdk_install()

