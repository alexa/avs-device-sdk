add_definitions("-DACSDK_LOG_MODULE=BluetoothImplementation")

add_library(acsdkBlueZBluetoothImplementation
        BluetoothImplementationComponent.cpp)

target_include_directories(acsdkBlueZBluetoothImplementation PUBLIC
        "${acsdkBlueZBluetoothImplementation_SOURCE_DIR}/include")

target_link_libraries(acsdkBlueZBluetoothImplementation
        acsdkManufactory
        AVSCommon
        BluetoothImplementationsBlueZ)

# install target
asdk_install()
