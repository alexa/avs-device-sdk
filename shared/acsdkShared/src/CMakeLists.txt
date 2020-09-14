add_definitions("-DACSDK_LOG_MODULE=acsdkShared")
add_library(acsdkShared SHARED
    SharedComponent.cpp)

target_include_directories(acsdkShared PUBLIC
    "${acsdkShared_SOURCE_DIR}/include")

target_link_libraries(acsdkShared
    acsdkManufactory
    AVSCommon)

# install target
asdk_install()
