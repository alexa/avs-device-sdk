add_definitions("-DACSDK_LOG_MODULE=bluetooth")

add_library(
    acsdkBluetooth
    BasicDeviceConnectionRule.cpp
    BasicDeviceConnectionRulesProvider.cpp
    Bluetooth.cpp
    BluetoothComponent.cpp
    BluetoothEventState.cpp
    BluetoothMediaInputTransformer.cpp
    BluetoothNotifier.cpp
    DeviceConnectionRulesAdapter.cpp
    SQLiteBluetoothStorage.cpp
)

target_include_directories(acsdkBluetooth PUBLIC
    "${acsdkBluetooth_SOURCE_DIR}/include"
)

target_link_libraries(acsdkBluetooth
    acsdkBluetoothInterfaces
    acsdkManufactory
    acsdkNotifier
    acsdkShutdownManagerInterfaces
    AVSCommon
    RegistrationManager
    SQLiteStorage
)

# install target
asdk_install()
