add_definitions("-DACSDK_LOG_MODULE=bluetoothImplementationsBlueZ")

add_library(BluetoothImplementationsBlueZ SHARED
    BlueZA2DPSource.cpp
    BlueZAVRCPTarget.cpp
    BlueZBluetoothDevice.cpp
    BlueZBluetoothDeviceManager.cpp
    BlueZDeviceManager.cpp
    BlueZHostController.cpp
    DBusConnection.cpp
    DBusObjectBase.cpp
    DBusPropertiesProxy.cpp
    DBusProxy.cpp
    GVariantMapReader.cpp
    GVariantTupleReader.cpp
    MediaContext.cpp
    MediaEndpoint.cpp
    PairingAgent.cpp
    )

target_include_directories(BluetoothImplementationsBlueZ PUBLIC
    "${BluetoothImplementationsBlueZ_SOURCE_DIR}/include"
    "${BlueZAlsa_SOURCE_DIR}"
    "${GIO_INCLUDE_DIRS}"
    "${GIO_UNIX_INCLUDE_DIRS}")

target_link_libraries(BluetoothImplementationsBlueZ "${GIO_LDFLAGS}" "sbc" AVSCommon)

# install target
asdk_install()
