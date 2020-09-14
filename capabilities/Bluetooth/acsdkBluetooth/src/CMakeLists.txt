add_definitions("-DACSDK_LOG_MODULE=bluetooth")

add_library(
    acsdkBluetooth SHARED
    BasicDeviceConnectionRule.cpp
    Bluetooth.cpp
    BluetoothEventState.cpp
    BluetoothMediaInputTransformer.cpp
    SQLiteBluetoothStorage.cpp
)

target_include_directories(acsdkBluetooth PUBLIC
    "${acsdkBluetooth_SOURCE_DIR}/include"
)

target_link_libraries(acsdkBluetooth
    acsdkBluetoothInterfaces
    AVSCommon
    RegistrationManager
    SQLiteStorage
)

# install target
asdk_install()
