#
# Set up Android specific configurations for the sample app.
#
# To build with Android, run the following command,
#     cmake <path-to-source>
#       -DANDROID=ON
#       -DANDROID_ABI=<android-abi-version>
#       ... Other android configurations.
#
# If ANDROID is ON then all android components are enabled by default. To disable only one component, set its option
# to OFF. E.g.:
#     cmake <path-to-source>
#       -DANDROID=ON
#       -DANDROID_LOGGER=OFF

include(CMakeDependentOption)
option(ANDROID "Enable Android build." OFF)
cmake_dependent_option(ANDROID_LOGGER "Use Android logging mechanism instead of console output." ON
        "ANDROID" OFF)
cmake_dependent_option(ANDROID_MICROPHONE "Use Android microphone to record audio." ON
        "ANDROID" OFF)
cmake_dependent_option(ANDROID_MEDIA_PLAYER "Use Android media player." ON
        "ANDROID" OFF)
cmake_dependent_option(ANDROID_DEVICE_INSTALL_PREFIX "Set the path on the device where the SDK will be installed." OFF
        "ANDROID" OFF)

# This option is only used for memory debug and is OFF by default.
cmake_dependent_option(ANDROID_ADDRESS_SANITIZER "Enable Android address sanitizer on debug build." OFF
        "ANDROID" OFF)

if (ANDROID)
    add_definitions("-DANDROID")

    if (ANDROID_LOGGER)
        add_definitions("-DANDROID_LOGGER")
    endif()

    if (ANDROID_MICROPHONE)
        add_definitions("-DANDROID_MICROPHONE")
    endif()
    
    if (ANDROID_MEDIA_PLAYER)
        if (NOT FFMPEG_INCLUDE_DIR OR NOT FFMPEG_LIB_PATH)
            message(FATAL_ERROR "Cannot build Android Media Player without FFmpeg support.")
        endif()
        add_definitions("-DANDROID_MEDIA_PLAYER")
    endif()

    if (ANDROID_ADDRESS_SANITIZER)
        # Turn on android's address sanitizer
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
        set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    endif()

    if (ANDROID_DEVICE_INSTALL_PREFIX)
        find_program(ADB_PATH adb)
        if (ADB_PATH)
            set(ANDROID_TEST_AVAILABLE ON)
        else()
            message("WARNING: adb: command not found."
            " To run unit tests, you'll need to upload the tests to the device.")
        endif()
    else ()
        message("WARNING: Unknown android device installation path."
            " To run unit tests, you'll need to upload the tests to the device.")
    endif()
endif()
