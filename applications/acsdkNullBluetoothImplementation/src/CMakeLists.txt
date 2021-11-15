add_definitions("-DACSDK_LOG_MODULE=BluetoothImplementation")

add_library(acsdkNullBluetoothImplementation
        BluetoothImplementationComponent.cpp)

target_include_directories(acsdkNullBluetoothImplementation PUBLIC
        "${acsdkNullBluetoothImplementation_SOURCE_DIR}/include")

target_link_libraries(acsdkNullBluetoothImplementation
        acsdkManufactory
        AVSCommon)

# install target
asdk_install()
