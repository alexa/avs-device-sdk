add_definitions("-DACSDK_LOG_MODULE=SDKComponent")

add_library(SDKComponent SDKComponent.cpp)

target_include_directories(SDKComponent PUBLIC
    "${SDKComponent_SOURCE_DIR}/include")

target_link_libraries(SDKComponent AVSCommon)

# install target
asdk_install()
