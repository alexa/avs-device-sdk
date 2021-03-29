#
# Setup the Bluetooth compiler options.
#
# BlueZ is currently the only Bluetooth implementation provided with the SDK.
#
# However, if you provide your own Bluetooth implementation, if your application is using the manufactory
# you can specify ACSDKBLUETOOTHIMPLEMENTATION_LIB as a cmake argument and provide the path to your own component.
# See applications/acsdkBlueZBluetoothImplementation for the BlueZ example of this.
#
# To build with Bluetooth capabilities using the BlueZ implementation, specify:
# cmake <path-to-source> -DBLUETOOTH_ENABLED=ON -DBLUETOOTH_BLUEZ=ON

include(CMakeDependentOption)
option(BLUETOOTH_ENABLED "Enable Bluetooth" OFF)
option(BLUETOOTH_BLUEZ "Use the BlueZ implementation of Bluetooth" OFF)
cmake_dependent_option(BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS "Override PulseAudio BlueZ Endpoints with SDK ones" OFF
        "BLUETOOTH_BLUEZ" OFF)

if (BLUETOOTH_BLUEZ)
    # To maintain backwards compatibility, when BLUETOOTH_BLUEZ=ON, BLUETOOTH_ENABLED=ON by default.
    if (NOT BLUETOOTH_ENABLED)
        set(BLUETOOTH_ENABLED ON)
    endif()

    message("Creating ${PROJECT_NAME} with BlueZ implementation of Bluetooth")
    find_package(PkgConfig)
    pkg_check_modules(GIO REQUIRED gio-2.0>=2.4)
    pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0>=2.4)
    pkg_check_modules(SBC REQUIRED sbc)
    add_definitions(-DBLUETOOTH_BLUEZ)

    if(BLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS)
        pkg_check_modules(PULSEAUDIO REQUIRED libpulse>=8.0)
        add_definitions(-DBLUETOOTH_BLUEZ_PULSEAUDIO_OVERRIDE_ENDPOINTS)
    endif()
endif()

if (BLUETOOTH_ENABLED)
    add_definitions(-DBLUETOOTH_ENABLED)
endif()
