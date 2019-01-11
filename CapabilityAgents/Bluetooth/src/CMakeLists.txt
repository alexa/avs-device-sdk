add_definitions("-DACSDK_LOG_MODULE=bluetooth")

add_library(
    Bluetooth SHARED
    Bluetooth.cpp
    BluetoothAVRCPTransformer.cpp
    SQLiteBluetoothStorage.cpp
)

set_target_properties(Bluetooth PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(Bluetooth PUBLIC
    "${Bluetooth_SOURCE_DIR}/include")

target_link_libraries(
    Bluetooth
    AVSCommon
    RegistrationManager
    SQLiteStorage
)

# install target
asdk_install()
