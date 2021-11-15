add_definitions("-DACSDK_LOG_MODULE=acsdkNullSystemTimeZone")
add_library(acsdkNullSystemTimeZone
        SystemTimeZoneComponent.cpp)

target_include_directories(acsdkNullSystemTimeZone PUBLIC
        "${acsdkNullSystemTimeZone_SOURCE_DIR}/include")

target_link_libraries(acsdkNullSystemTimeZone
        acsdkManufactory)

# install target
asdk_install()
